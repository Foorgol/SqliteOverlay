#include <tuple>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include <Sloppy/DateTime/DateAndTime.h>
#include <Sloppy/Crypto/Crypto.h>

#include "TableCreator.h"
#include "Transaction.h"
#include "UserMngr.h"

namespace SqliteOverlay
{
  namespace UserMngr
  {

    ErrCode UserMngr::createUser(const string& name, const string& pw, int minPwLen, int pwExiration__secs, bool createAsLocked, int saltLen, int hashCycles) const
    {
      // check user and password for formal validity
      string n = boost::trim_copy(name);
      if (n.empty() || (n.size() > MaxUserNameLen)) return ErrCode::InvalidName;

      string p = boost::trim_copy(pw);
      if ((p.size() < minPwLen) || (p.size() > MaxPwLen) || (minPwLen <=0)) return ErrCode::InvalidPassword;

      // make sure the login name is still available
      if (tab->getMatchCountForColumnValue(US_LoginName, n) > 0) return ErrCode::InvalidName;

      // hash the password
      auto hashResult = Sloppy::Crypto::hashPassword(p, saltLen, hashCycles);
      const string salt = hashResult.first;
      const string hashedPw = hashResult.second;
      if ((salt.empty()) || (hashedPw.empty())) return ErrCode::UnspecifiedError;

      // combine hash cycles, salt and hash using '_' as the delimiter
      const string saltAndHash = to_string(hashCycles) + "_" + salt + "_" + hashedPw;

      // create a new database entry for the user and the password
      auto tr = db->startTransaction();
      if (tr == nullptr) return ErrCode::DatabaseError;

      // the entry in the user table
      ColumnValueClause cvc;
      cvc.addStringCol(US_LoginName, n);
      Sloppy::DateTime::UTCTimestamp now{};
      cvc.addDateTimeCol(US_CreationTime, &now);
      cvc.addIntCol(US_LoginFailCount, 0);
      cvc.addDateTimeCol(US_LastAuthSuccessTime, &now);
      if (createAsLocked)
      {
        cvc.addIntCol(US_State, static_cast<int>(UserState::Locked));
      } else {
        cvc.addIntCol(US_State, static_cast<int>(UserState::Active));
      }

      int dbErr;
      int newId = tab->insertRow(cvc, &dbErr);
      if ((newId < 1) || (dbErr != SQLITE_DONE))
      {
        tr->rollback();
        return ErrCode::DatabaseError;
      }

      // the entry in the password table
      cvc.clear();
      cvc.addIntCol(U2P_UserRef, newId);
      cvc.addStringCol(U2P_Password, saltAndHash);
      cvc.addDateTimeCol(U2P_CreationTime, &now);
      if (pwExiration__secs > 0)
      {
        UTCTimestamp expireTime{now};
        expireTime.applyOffset(pwExiration__secs);
        cvc.addDateTimeCol(U2P_ExpirationTime, &expireTime);
      }
      newId = pwTab->insertRow(cvc, &dbErr);
      if ((newId < 1) || (dbErr != SQLITE_DONE))
      {
        tr->rollback();
        return ErrCode::DatabaseError;
      }

      return tr->commit() ? ErrCode::Success : ErrCode::DatabaseError;
    }

    //----------------------------------------------------------------------------

    ErrCode UserMngr::setEmail(const string& name, const string& email) const
    {
      string e = boost::trim_copy(email);
      if (e.empty() || (e.size() > MaxEmailLen)) return ErrCode::UnspecifiedError;

      //
      // FIX: check for a correct email format
      //

      int id = getIdForUser(name);
      if (id < 1) return ErrCode::InvalidName;

      TabRow r = tab->operator [](id);
      ;

      return r.update(US_Email, e) ? ErrCode::Success : ErrCode::DatabaseError;
    }

    //----------------------------------------------------------------------------

    ErrCode UserMngr::updatePassword(const string& name, const string& oldPw, const string& newPw, int historyCheckDepth, int minPwLen,
                                     int pwExiration__secs, int saltLen, int hashCycles) const
    {
      // make sure the user name is valid
      int uid = getIdForUser(name);
      if (uid < 1) return ErrCode::InvalidName;

      // check password constraints
      string p = boost::trim_copy(newPw);
      if ((p.size() < minPwLen) || (p.size() > MaxPwLen) || (minPwLen <=0)) return ErrCode::InvalidPassword;

      // check the correctness of the old password
      if (!(checkPasswort(uid, oldPw))) return ErrCode::NotAuthenticated;

      // we need these later...
      int lastCycleCount = -1;
      string lastSalt;
      string lastHashedNewPassword;

      // go back in history and make sure that old PWs are not reused
      if (p == oldPw) return ErrCode::InvalidPassword;
      if (historyCheckDepth > 1)
      {
        WhereClause w;
        w.addIntCol(U2P_UserRef, uid);
        w.setOrderColumn_Desc(U2P_CreationTime);   // get the newest entries first
        w.setOrderColumn_Desc("id");   // get the newest entries first
        auto it = pwTab->getRowsByWhereClause(w);

        // skip the first entry (has already been checked)
        if (!(it.isEnd())) ++it;

        // walk through old hashes and check for "collisions"
        int cnt = 1;
        while ((!(it.isEnd())) && (cnt < historyCheckDepth))
        {
          TabRow r = *it;

          Sloppy::StringList sl;
          Sloppy::stringSplitter(sl, r[U2P_Password], "_");
          if (sl.size() < 3) return ErrCode::UnspecifiedError;  // shouldn't happen

          int nCycles = stoi(sl[0]);
          const string salt = sl[1];
          const string oldHash = sl[2];

          if ((nCycles != lastCycleCount) || (salt != lastSalt))
          {
            // hash the new password with the hashing parameters of the old one
            lastHashedNewPassword = Sloppy::Crypto::hashPassword(p, salt, nCycles);
            lastCycleCount = nCycles;
            lastSalt = salt;
          }

          // compare new and old hash
          if (lastHashedNewPassword == oldHash) return ErrCode::InvalidPassword;

          ++cnt;
          ++it;
        }
      }

      // authentication and pw history are okay, hash and prepare the new PW
      //
      // re-use old hashing parameters, if possible, in order to reduce
      // multiple hashing operations when checking the PW history later on
      if ((hashCycles != lastCycleCount) || (lastSalt.length() != saltLen))
      {
        auto hashResult = Sloppy::Crypto::hashPassword(p, saltLen, hashCycles);
        lastSalt = hashResult.first;
        lastHashedNewPassword = hashResult.second;
        if ((lastSalt.empty()) || (lastHashedNewPassword.empty())) return ErrCode::UnspecifiedError;
      }
      const string saltAndHash = to_string(hashCycles) + "_" + lastSalt + "_" + lastHashedNewPassword;

      // okay, perform the update
      //
      // we need two modifications in the PW table: disabling the old entry and adding a new entry.
      // additionally, we need to update a few entries in the user table
      auto tr = db->startTransaction();
      if (tr == nullptr) return ErrCode::DatabaseError;

      // disable the old entry
      WhereClause w;
      w.addIntCol(U2P_UserRef, uid);
      w.addNullCol(U2P_DisablingTime);
      TabRow oldPwRow = pwTab->getSingleRowByWhereClause(w);  // this row MUST exist
      UTCTimestamp now;
      int dbErr;
      oldPwRow.update(U2P_DisablingTime, now, &dbErr);
      if (dbErr != SQLITE_DONE)
      {
        tr->rollback();
        return ErrCode::DatabaseError;
      }

      // create the new entry
      ColumnValueClause cvc;
      cvc.addIntCol(U2P_UserRef, uid);
      cvc.addStringCol(U2P_Password, saltAndHash);
      cvc.addDateTimeCol(U2P_CreationTime, &now);
      if (pwExiration__secs > 0)
      {
        UTCTimestamp expireTime{now};
        expireTime.applyOffset(pwExiration__secs);
        cvc.addDateTimeCol(U2P_ExpirationTime, &expireTime);
      }
      int newId = pwTab->insertRow(cvc, &dbErr);
      if ((newId < 1) || (dbErr != SQLITE_DONE))
      {
        tr->rollback();
        return ErrCode::DatabaseError;
      }

      // update the user table
      TabRow userRow = tab->operator [](uid);
      cvc.clear();
      cvc.addIntCol(US_LoginFailCount, 0);  // reset the failure counter
      cvc.addDateTimeCol(US_LastAuthSuccessTime, &now);
      userRow.update(cvc, &dbErr);
      if (dbErr != SQLITE_DONE)
      {
        tr->rollback();
        return ErrCode::DatabaseError;
      }

      return tr->commit() ? ErrCode::Success : ErrCode::DatabaseError;
    }

    //----------------------------------------------------------------------------

    ErrCode UserMngr::deleteUser(const string& name) const
    {
      // make sure the user name is valid
      int id = getIdForUser(name);
      if (id < 1) return ErrCode::InvalidName;

      // delete all passwords, role assignments, sessions and the user itself
      int dbErr;
      auto t = db->startTransaction();
      vector<pair<DbTab*, string>> tabsAndCols = {{pwTab, U2P_UserRef}, {roleTab, U2R_UserRef},
                                                  {sessionTab, U2S_UserRef}, {tab, "id"}};
      for (const auto& tc : tabsAndCols)
      {
        int dbErr;
        (tc.first)->deleteRowsByColumnValue(tc.second, id, &dbErr);
        if (dbErr != SQLITE_DONE)
        {
          t->rollback();
          return ErrCode::DatabaseError;
        }
      }

      return t->commit() ? ErrCode::Success : ErrCode::DatabaseError;
    }

    //----------------------------------------------------------------------------

    bool UserMngr::lockUser(const string& name) const
    {
      // make sure the user name is valid
      int id = getIdForUser(name);
      if (id < 1) return false;

      TabRow r = tab->operator [](id);
      r.update(US_State, static_cast<int>(UserState::Locked));

      // terminate all open sessions for this user
      terminateAllUserSessions(name);

      return true;
    }

    //----------------------------------------------------------------------------

    bool UserMngr::unlockUser(const string& name) const
    {
      // make sure the user name is valid
      int id = getIdForUser(name);
      if (id < 1) return false;

      TabRow r = tab->operator [](id);
      r.update(US_State, static_cast<int>(UserState::Active));

      return true;
    }

    //----------------------------------------------------------------------------

    unique_ptr<UserData> UserMngr::getUserData(const string& userName) const
    {
      int id = getIdForUser(userName);
      if (id < 1) return nullptr;
      return getUserData(id);
    }

    //----------------------------------------------------------------------------

    unique_ptr<UserData> UserMngr::getUserData(int id) const
    {
      try
      {
        TabRow userRow = tab->operator [](id);

        WhereClause w;
        w.addIntCol(U2P_UserRef, id);
        w.addNullCol(U2P_DisablingTime);
        TabRow pwRow = pwTab->getSingleRowByWhereClause(w);

        unique_ptr<UserData> result = make_unique<UserData>();
        result->uid = id;
        result->loginName = userRow[US_LoginName];
        auto email = userRow.getString2(US_Email);
        result->email = (email->isNull()) ? "" : email->get();
        result->userCreationTime = userRow.getUTCTime(US_CreationTime);
        auto expTime = pwRow.getUTCTime2(U2P_ExpirationTime);
        result->pwExpirationTime = (expTime->isNull()) ? nullptr : make_unique<Sloppy::DateTime::UTCTimestamp>(expTime->get());
        result->lastPwChange = pwRow.getUTCTime(U2P_CreationTime);
        result->lastAuthSuccess = userRow.getUTCTime(US_LastAuthSuccessTime);
        auto failTime = userRow.getUTCTime2(US_LastAuthFailTime);
        result->lastAuthFail = (failTime->isNull()) ? nullptr : make_unique<Sloppy::DateTime::UTCTimestamp>(failTime->get());
        result->failCount = userRow.getInt(US_LoginFailCount);
        result->state = static_cast<UserState>(userRow.getInt(US_State));

        return result;
      }
      catch (...) {}

      return nullptr;
    }

    //----------------------------------------------------------------------------

    bool UserMngr::hasUser(const string& name)
    {
      return (getIdForUser(name) > 0);
    }

    //----------------------------------------------------------------------------

    AuthInfo UserMngr::authenticateUser(const string& name, const string& pw) const
    {
      AuthInfo result;

      int id = getIdForUser(name);
      if (id < 1)
      {
        result.result = AuthResult::UnknownUser;
        result.user = nullptr;
        return result;   // all other data fields are undefined
      }

      result.user = getUserData(id);
      if (result.user == nullptr)
      {
        result.result = AuthResult::UnknownUser;
        return result;
      }

      // check the user's state
      if (result.user->state == UserState::Locked)
      {
        result.result = AuthResult::Locked;
        return result;
      }

      // check password expiration, if applicable
      Sloppy::DateTime::UTCTimestamp now{};
      if (result.user->pwExpirationTime != nullptr)
      {
        if (now > *(result.user->pwExpirationTime))
        {
          result.result = AuthResult::PasswordExpired;
          return result;
        }
      }

      // check the passwork itself
      TabRow r = tab->operator [](id);
      if (!(checkPasswort(id, pw)))
      {
        // update the failure counters
        ColumnValueClause cvc;
        result.user->lastAuthFail = make_unique<Sloppy::DateTime::UTCTimestamp>(now);
        cvc.addDateTimeCol(US_LastAuthFailTime, &now);
        result.user->failCount += 1;
        cvc.addIntCol(US_LoginFailCount, result.user->failCount);
        r.update(cvc);   // FIX: we're not checking for DB errors here!

        result.result = AuthResult::WrongPassword;
        return result;
      }

      // ok, if we're at this point, everthing is fine
      result.user->lastAuthSuccess = now;
      r.update(US_LastAuthSuccessTime, now);
      r.update(US_LoginFailCount, 0);
      result.result = AuthResult::Authenticated;

      return result;
    }

    //----------------------------------------------------------------------------

    SessionInfo UserMngr::startSession(const string& name, const string& pw, int sessionExpiration__secs, int cookieLen) const
    {
      SessionInfo result;
      result.refreshCount = 0;

      // try to authenticate the user
      const AuthInfo ai = authenticateUser(name, pw);
      if ((ai.result != AuthResult::Authenticated) || (cookieLen <= 0) || (cookieLen > MaxSessionCookiLen) || (sessionExpiration__secs < 0))
      {
        result.cookie = "";
        result.user = nullptr;
        const Sloppy::DateTime::UTCTimestamp dummy{static_cast<time_t>(0)};
        result.sessionStart = dummy;
        result.sessionExpiration = dummy;
        result.lastRefresh = dummy;
        result.status = SessionStatus::Invalid;
        result.refreshCount = -1;

        return result;
      }

      //
      // the provided credentials are okay, so we can initiate a session
      //

      // create a unique cookie
      while (result.cookie.empty())
      {
        result.cookie = Sloppy::Crypto::getRandomAlphanumString(cookieLen);
        if (sessionTab->getMatchCountForColumnValue(U2S_SessionCookie, result.cookie) > 0)
        {
          result.cookie.clear();
        }
      }

      // fill-in other data fields
      result.user = getUserData(name);
      const Sloppy::DateTime::UTCTimestamp now;
      result.sessionStart = now;
      result.sessionExpiration = now;
      result.sessionExpiration.applyOffset(sessionExpiration__secs);
      result.lastRefresh = now;

      // update the database
      ColumnValueClause cvc;
      cvc.addIntCol(U2S_UserRef, getIdForUser(name));
      cvc.addStringCol(U2S_SessionCookie, result.cookie);
      cvc.addDateTimeCol(U2S_SessionStartTime, &now);
      cvc.addIntCol(U2S_SessionLength_Secs, sessionExpiration__secs);
      cvc.addDateTimeCol(U2S_LastRefreshTime, &now);
      cvc.addIntCol(U2S_RefreshCount, 0);
      int dbErr;
      int newId = sessionTab->insertRow(cvc, &dbErr);
      if ((newId < 1) || (dbErr != SQLITE_DONE))
      {
        // database error! Reset everything to error values
        result.cookie = "";
        result.user = nullptr;
        const Sloppy::DateTime::UTCTimestamp dummy{static_cast<time_t>(0)};
        result.sessionStart = dummy;
        result.sessionExpiration = dummy;
        result.lastRefresh = dummy;
        result.status = SessionStatus::Invalid;
        result.refreshCount = -1;

        return result;
      }

      // now we're really done
      result.status = SessionStatus::Created;

      return result;
    }

    //----------------------------------------------------------------------------

    SessionInfo UserMngr::validateSession(const string& cookie, bool refreshValidity) const
    {
      SessionInfo result;

      // check if the cookie exists
      if (sessionTab->getMatchCountForColumnValue(U2S_SessionCookie, cookie) < 1)
      {
        result.cookie = "";
        result.user = nullptr;

        const Sloppy::DateTime::UTCTimestamp dummy{static_cast<time_t>(0)};
        result.sessionStart = dummy;
        result.sessionExpiration = dummy;
        result.lastRefresh = dummy;
        result.refreshCount = -1;
        result.status = SessionStatus::Invalid;

        return result;
      }

      // get the cookie row
      TabRow r = sessionTab->getSingleRowByColumnValue(U2S_SessionCookie, cookie);

      // get the associated user name
      int userId = r.getInt(U2S_UserRef);
      result.user = getUserData(userId);

      // copy some constant data fields
      result.cookie = cookie;
      result.sessionStart = r.getUTCTime(U2S_SessionStartTime);
      result.refreshCount = r.getInt(U2S_RefreshCount);

      // calculate the validity
      const Sloppy::DateTime::UTCTimestamp lastRefresh{r.getUTCTime(U2S_LastRefreshTime)};
      result.lastRefresh = lastRefresh;
      const int duration = r.getInt(U2S_SessionLength_Secs);
      Sloppy::DateTime::UTCTimestamp exTime{lastRefresh};
      exTime.applyOffset(duration);
      result.sessionExpiration = exTime;

      const Sloppy::DateTime::UTCTimestamp now;
      if (now > exTime)
      {
        // the cookie's validity has expired
        result.status = SessionStatus::Expired;
        return result;
      }

      // the session has been successfully validated. Shall we do a refresh?
      if (refreshValidity)
      {
        ++result.refreshCount;
        result.lastRefresh = now;
        ColumnValueClause cvc;
        cvc.addIntCol(U2S_RefreshCount, result.refreshCount);
        cvc.addDateTimeCol(U2S_LastRefreshTime, &now);
        r.update(cvc);  // no check for errors here; if something fails, the session simply expires earlier

        UTCTimestamp newExpTime{now};
        newExpTime.applyOffset(duration);
        result.sessionExpiration = newExpTime;

        result.status = SessionStatus::Refreshed;
      } else {
        result.status = SessionStatus::Valid;
      }

      return result;
    }

    //----------------------------------------------------------------------------

    ErrCode UserMngr::terminateSession(const string& cookie) const
    {
      // check if the cookie exists
      if (sessionTab->getMatchCountForColumnValue(U2S_SessionCookie, cookie) < 1)
      {
        return ErrCode::InvalidName;
      }

      // delete it
      int dbErr;
      sessionTab->deleteRowsByColumnValue(U2S_SessionCookie, cookie, &dbErr);

      return (dbErr == SQLITE_DONE) ? ErrCode::Success : ErrCode::DatabaseError;
    }

    //----------------------------------------------------------------------------

    ErrCode UserMngr::terminateAllUserSessions(const string& name) const
    {
      // make sure the user name is valid
      int id = getIdForUser(name);
      if (id < 1) return ErrCode::InvalidName;

      // delete all cookies of this user
      int dbErr;
      sessionTab->deleteRowsByColumnValue(U2S_UserRef, id, &dbErr);

      return (dbErr == SQLITE_DONE) ? ErrCode::Success : ErrCode::DatabaseError;
    }

    //----------------------------------------------------------------------------

    bool UserMngr::terminateAllSessions() const
    {
      int dbErr;
      int rowsAffected = sessionTab->clear(&dbErr);

      return ((rowsAffected >= 0) && (dbErr == SQLITE_DONE));
    }

    //----------------------------------------------------------------------------

    bool UserMngr::cleanupExpiredSessions() const
    {
      if (sessionTab->length() < 1) return true;

      auto tr = db->startTransaction();
      if (tr == nullptr) return false;

      UTCTimestamp now;
      auto it = sessionTab->getAllRows();
      while (!(it.isEnd()))
      {
        TabRow r = *it;

        // calculate the expiration time of the sessions
        const Sloppy::DateTime::UTCTimestamp lastRefresh{r.getUTCTime(U2S_LastRefreshTime)};
        const int duration = r.getInt(U2S_SessionLength_Secs);
        Sloppy::DateTime::UTCTimestamp exTime{lastRefresh};
        exTime.applyOffset(duration);

        // delete the row if the session is expired
        if (now > exTime)
        {
          int dbErr;
          int rowsAffected = sessionTab->deleteRowsByColumnValue("id", r.getId(), &dbErr);

          if ((rowsAffected != 1) || (dbErr != SQLITE_DONE))
          {
            tr->rollback();
            return false;
          }
        }

        ++it;
      }

      return tr->commit();
    }

    //----------------------------------------------------------------------------

    void UserMngr::initTabs(const string& tp)
    {
      TableCreator tc{db};
      tc.addVarchar(US_LoginName, MaxUserNameLen, true, CONFLICT_CLAUSE::FAIL, true, CONFLICT_CLAUSE::FAIL);
      tc.addVarchar(US_Email, MaxEmailLen);
      tc.addInt(US_CreationTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_LastAuthSuccessTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_LastAuthFailTime, false, CONFLICT_CLAUSE::__NOT_SET);
      tc.addInt(US_LoginFailCount, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_State, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.createTableAndResetCreator(tp + Tab_User_Ext);
      tab = db->getTab(tp + Tab_User_Ext);

      tc.addForeignKey(U2P_UserRef, tp + Tab_User_Ext, CONSISTENCY_ACTION::CASCADE);
      tc.addVarchar(U2P_Password, 10 + 2 + MaxSaltLen + 64, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2P_CreationTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2P_ExpirationTime, false, CONFLICT_CLAUSE::__NOT_SET);
      tc.addInt(U2P_DisablingTime, false, CONFLICT_CLAUSE::__NOT_SET);
      tc.createTableAndResetCreator(tp + Tab_User2Password_Ext);
      pwTab = db->getTab(tp + Tab_User2Password_Ext);

      tc.addForeignKey(U2R_UserRef, tp + Tab_User_Ext, CONSISTENCY_ACTION::CASCADE);
      tc.addInt(U2R_Role, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2R_CreationTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.createTableAndResetCreator(tp + Tab_User2Role_Ext);
      roleTab = db->getTab(tp + Tab_User2Role_Ext);

      tc.addForeignKey(U2S_UserRef, tp + Tab_User_Ext, CONSISTENCY_ACTION::CASCADE);
      tc.addVarchar(U2S_SessionCookie, MaxSessionCookiLen, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_SessionStartTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_SessionLength_Secs, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_LastRefreshTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_RefreshCount, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.createTableAndResetCreator(tp + Tab_User2Session_Ext);
      sessionTab = db->getTab(tp + Tab_User2Session_Ext);
    }

    //----------------------------------------------------------------------------

    int UserMngr::getIdForUser(const string& name) const
    {
      try
      {
        TabRow r = tab->getSingleRowByColumnValue(US_LoginName, name);
        return r.getId();
      }
      catch (...) {}

      return -1;
    }

    //----------------------------------------------------------------------------

    vector<UserData> UserMngr::getAllUsers() const
    {
      vector<UserData> result;
      auto it = tab->getAllRows();
      while (!(it.isEnd()))
      {
        TabRow r = *it;
        auto ud = getUserData(r.getId());
        if (ud != nullptr) result.push_back(*ud);
        ++it;
      }

      return result;
    }

    //----------------------------------------------------------------------------

    bool UserMngr::checkPasswort(int userId, const string& providedPw) const
    {
      if (userId < 1) return false;

      string saltAndHash;
      try
      {
        WhereClause w;
        w.addIntCol(U2P_UserRef, userId);
        w.addNullCol(U2P_DisablingTime);
        TabRow pwRow = pwTab->getSingleRowByWhereClause(w);

        saltAndHash = pwRow[U2P_Password];
      }
      catch (...)
      {
        return false;
      }

      Sloppy::StringList sl;
      Sloppy::stringSplitter(sl, saltAndHash, "_");
      if (sl.size() < 3) return false;

      int nCycles = stoi(sl[0]);

      return Sloppy::Crypto::checkPassword(providedPw, sl[2], sl[1], nCycles);
    }

    //----------------------------------------------------------------------------

    bool UserMngr::assignRole(const string& name, int roleId) const
    {
      int uid = getIdForUser(name);
      if (uid < 1) return false;

      if (hasRole(uid, roleId)) return true;

      ColumnValueClause cvc;
      cvc.addIntCol(U2R_UserRef, uid);
      cvc.addIntCol(U2R_Role, roleId);
      UTCTimestamp now;
      cvc.addDateTimeCol(U2R_CreationTime, &now);
      int dbErr;
      int newRowId = roleTab->insertRow(cvc, &dbErr);

      return ((newRowId > 0) && (dbErr == SQLITE_DONE));
    }

    //----------------------------------------------------------------------------

    bool UserMngr::removeRole(const string& name, int roleId) const
    {
      int uid = getIdForUser(name);
      if (uid < 1) return false;

      if (!(hasRole(uid, roleId))) return true;

      WhereClause w;
      w.addIntCol(U2R_UserRef, uid);
      w.addIntCol(U2R_Role, roleId);
      int dbErr;
      int delRowCount = roleTab->deleteRowsByWhereClause(w, &dbErr);

      return ((delRowCount > 0) && (dbErr == SQLITE_DONE));
    }

    //----------------------------------------------------------------------------

    bool UserMngr::hasRole(const string& name, int roleId) const
    {
      return hasRole(getIdForUser(name), roleId);
    }

    //----------------------------------------------------------------------------

    bool UserMngr::hasRole(int uid, int roleId) const
    {
      WhereClause w;
      w.addIntCol(U2R_UserRef, uid);
      w.addIntCol(U2R_Role, roleId);
      return (roleTab->getMatchCountForWhereClause(w) > 0);
    }

    //----------------------------------------------------------------------------

    UserData::UserData(const UserData& src)
    {
      // copy the plain data fields
      loginName = src.loginName;
      email = src.email;
      userCreationTime = src.userCreationTime;
      lastPwChange = src.lastPwChange;
      lastAuthSuccess = src.lastAuthSuccess;
      failCount = src.failCount;
      state = src.state;
      uid = src.uid;

      // handle the unique_ptr fields
      if (src.pwExpirationTime != nullptr)
      {
        time_t raw = src.pwExpirationTime->getRawTime();
        pwExpirationTime = make_unique<UTCTimestamp>(raw);
      } else {
        pwExpirationTime = nullptr;
      }
      if (src.lastAuthFail != nullptr)
      {
        time_t raw = src.lastAuthFail->getRawTime();
        lastAuthFail = make_unique<UTCTimestamp>(raw);
      } else {
        lastAuthFail = nullptr;
      }
    }

    //----------------------------------------------------------------------------

    UserData& UserData::operator=(const UserData& src)
    {
      // copy the plain data fields
      loginName = src.loginName;
      email = src.email;
      userCreationTime = src.userCreationTime;
      lastPwChange = src.lastPwChange;
      lastAuthSuccess = src.lastAuthSuccess;
      failCount = src.failCount;
      state = src.state;
      uid = src.uid;

      // handle the unique_ptr fields
      if (src.pwExpirationTime != nullptr)
      {
        time_t raw = src.pwExpirationTime->getRawTime();
        pwExpirationTime = make_unique<UTCTimestamp>(raw);
      } else {
        pwExpirationTime = nullptr;
      }
      if (src.lastAuthFail != nullptr)
      {
        time_t raw = src.lastAuthFail->getRawTime();
        lastAuthFail = make_unique<UTCTimestamp>(raw);
      } else {
        lastAuthFail = nullptr;
      }

      return *this;
    }

    //----------------------------------------------------------------------------

  }

}
