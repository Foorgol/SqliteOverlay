#include <tuple>

#include <boost/algorithm/string.hpp>

#include <Sloppy/DateTime/DateAndTime.h>
#include <Sloppy/Crypto/Crypto.h>

#include "TableCreator.h"
#include "Transaction.h"
#include "UserMngr.h"

namespace RankingApp
{
  namespace UserMngr
  {

    ErrCode UserMngr::createUser(const string& name, const string& pw, int minPwLen, int pwExiration__secs, int saltLen, int hashCycles) const
    {
      // check user and password for formal validity
      string n = boost::trim_copy(name);
      if (n.empty() || (n.size() > MaxUserNameLen)) return ErrCode::InvalidName;

      string p = boost::trim_copy(pw);
      if ((p.size() < minPwLen) || (p.size() > MaxPwLen)) return ErrCode::InvalidPasswort;

      // make sure the login name is still available
      if (tab->getMatchCountForColumnValue(US_LoginName, n) > 0) return ErrCode::InvalidName;

      // hash the password
      auto hashResult = Sloppy::Crypto::hashPassword(pw, saltLen, hashCycles);
      const string salt = hashResult.first;
      const string hashedPw = hashResult.second;
      if ((salt.empty()) || (hashedPw.empty())) return ErrCode::UnspecifiedError;

      // combine hash cycles, salt and hash using '_' as the delimiter
      const string saltAndHash = to_string(hashCycles) + "_" + salt + "_" + hashedPw;

      // create a new database entry
      Sloppy::DateTime::UTCTimestamp now{};
      ColumnValueClause cvc;
      cvc.addStringCol(US_LoginName, n);
      cvc.addStringCol(US_Password, saltAndHash);
      cvc.addDateTimeCol(US_CreationTime, &now);
      cvc.addDateTimeCol(US_LastPwChangeTime, &now);
      if (pwExiration__secs > 0)
      {
        time_t t = now.getRawTime();
        UTCTimestamp expireTime{t + pwExiration__secs};
        cvc.addDateTimeCol(US_PwExpirationTime, &expireTime);
      }
      cvc.addIntCol(US_LoginFailCount, 0);
      cvc.addDateTimeCol(US_LastAuthSuccessTime, &now);
      cvc.addIntCol(US_State, static_cast<int>(UserState::Active));
      int dbErr;
      int newId = tab->insertRow(cvc, &dbErr);
      if ((newId < 1) || (dbErr != SQLITE_OK)) return ErrCode::DatabaseError;

      return ErrCode::Success;
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
      r.update(US_Email, e);

      return ErrCode::Success;
    }

    //----------------------------------------------------------------------------

    ErrCode UserMngr::updatePassword(const string& name, const string& oldPw, const string& newPw, int minPwLen, int pwExiration__secs, int saltLen, int hashCycles) const
    {
      // make sure the user name is valid
      int id = getIdForUser(name);
      if (id < 1) return ErrCode::InvalidName;

      // check password constraints
      string p = boost::trim_copy(newPw);
      if ((p.size() < minPwLen) || (p.size() > MaxPwLen)) return ErrCode::InvalidPasswort;

      // check the correctness of the old password
      if (!(checkPasswort(id, oldPw))) return ErrCode::NotAuthenticated;

      // authentication okay, hash and prepare the new PW
      auto hashResult = Sloppy::Crypto::hashPassword(newPw, saltLen, hashCycles);
      const string salt = hashResult.first;
      const string hashedPw = hashResult.second;
      if ((salt.empty()) || (hashedPw.empty())) return ErrCode::UnspecifiedError;
      const string saltAndHash = to_string(hashCycles) + "_" + salt + "_" + hashedPw;

      // okay, perform the update
      Sloppy::DateTime::UTCTimestamp now{};
      ColumnValueClause cvc;
      cvc.addStringCol(US_Password, saltAndHash);
      if (pwExiration__secs > 0)
      {
        time_t t = now.getRawTime();
        UTCTimestamp expireTime{t + pwExiration__secs};
        cvc.addDateTimeCol(US_PwExpirationTime, &expireTime);
      } else {
        cvc.addNullCol(US_PwExpirationTime);   // remove any old expiration time
      }
      cvc.addIntCol(US_LoginFailCount, 0);  // reset the failure counter
      cvc.addDateTimeCol(US_LastPwChangeTime, &now);
      cvc.addDateTimeCol(US_LastAuthSuccessTime, &now);
      TabRow r = tab->operator [](id);
      int dbErr;
      r.update(cvc, &dbErr);

      if (dbErr != SQLITE_OK) return ErrCode::DatabaseError;

      return ErrCode::Success;
    }

    //----------------------------------------------------------------------------

    ErrCode UserMngr::deleteUser(const string& name) const
    {
      // make sure the user name is valid
      int id = getIdForUser(name);
      if (id < 1) return ErrCode::InvalidName;

      // delete all role assignments, sessions and the user itself
      int dbErr;
      auto t = db->startTransaction();
      roleTab->deleteRowsByColumnValue(U2R_UserRef, id, &dbErr);
      if (dbErr != SQLITE_OK)
      {
        t->rollback();
        return ErrCode::DatabaseError;
      }
      sessionTab->deleteRowsByColumnValue(U2S_UserRef, id, &dbErr);
      if (dbErr != SQLITE_OK)
      {
        t->rollback();
        return ErrCode::DatabaseError;
      }
      tab->deleteRowsByColumnValue("id", id, &dbErr);
      if (dbErr != SQLITE_OK)
      {
        t->rollback();
        return ErrCode::DatabaseError;
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
        TabRow r = tab->operator [](id);

        unique_ptr<UserData> result = make_unique<UserData>();
        result->loginName = r[US_LoginName];
        auto email = r.getString2(US_Email);
        result->email = (email->isNull()) ? "" : email->get();
        result->userCreationTime = Sloppy::DateTime::UTCTimestamp{r.getInt(US_CreationTime)};
        auto expTime = r.getInt2(US_PwExpirationTime);
        result->pwExpirationTime = (expTime->isNull()) ? nullptr : make_unique<Sloppy::DateTime::UTCTimestamp>(expTime->get());
        result->lastPwChange = Sloppy::DateTime::UTCTimestamp{r.getInt(US_LastPwChangeTime)};
        result->lastAuthSuccess = Sloppy::DateTime::UTCTimestamp{r.getInt(US_LastAuthSuccessTime)};
        auto failTime = r.getInt2(US_LastAuthFailTime);
        result->lastAuthFail = (failTime->isNull()) ? nullptr : make_unique<Sloppy::DateTime::UTCTimestamp>(failTime->get());
        result->failCount = r.getInt(US_LoginFailCount);
        result->state = static_cast<UserState>(r.getInt(US_State));

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
      if ((ai.result != AuthResult::Authenticated) || (cookieLen < 0) || (cookieLen > MaxSessionCookiLen) || (sessionExpiration__secs < 0))
      {
        result.cookie = "";
        result.user = nullptr;
        const Sloppy::DateTime::UTCTimestamp dummy{0};
        result.sessionStart = dummy;
        result.sessionExpiration = dummy;
        result.lastRefresh = dummy;
        result.status = SessionStatus::Unknown;
        result.refreshCount = -1;

        return result;
      }

      // the provided credentials are okay, so we can initiate a session
      const Sloppy::DateTime::UTCTimestamp now;
      result.cookie = Sloppy::Crypto::getRandomAlphanumString(cookieLen);
      result.user = getUserData(name);
      result.sessionStart = now;
      result.sessionExpiration = Sloppy::DateTime::UTCTimestamp{now.getRawTime() + sessionExpiration__secs};
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
      if ((newId < 1) || (dbErr != SQLITE_OK))
      {
        // database error! Reset everything to error values
        result.cookie = "";
        result.user = nullptr;
        const Sloppy::DateTime::UTCTimestamp dummy{0};
        result.sessionStart = dummy;
        result.sessionExpiration = dummy;
        result.lastRefresh = dummy;
        result.status = SessionStatus::Unknown;
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

        const Sloppy::DateTime::UTCTimestamp dummy{0};
        result.sessionStart = dummy;
        result.sessionExpiration = dummy;
        result.lastRefresh = dummy;
        result.refreshCount = -1;
        result.status = SessionStatus::Unknown;

        return result;
      }

      // get the cookie row
      TabRow r = sessionTab->getSingleRowByColumnValue(U2S_SessionCookie, cookie);

      // get the associated user name
      int userId = r.getInt(U2S_UserRef);
      result.user = getUserData(userId);

      // copy some constant data fields
      result.cookie = cookie;
      result.sessionStart = Sloppy::DateTime::UTCTimestamp{r.getInt(U2S_SessionStartTime)};
      result.refreshCount = r.getInt(U2S_RefreshCount);

      // calculate the validity
      const Sloppy::DateTime::UTCTimestamp lastRefresh{r.getInt(U2S_LastRefreshTime)};
      result.lastRefresh = lastRefresh;
      const int duration = r.getInt(U2S_SessionLength_Secs);
      const Sloppy::DateTime::UTCTimestamp exTime{lastRefresh.getRawTime() + duration};
      result.sessionExpiration = exTime;

      const Sloppy::DateTime::UTCTimestamp now;
      if (now > exTime)
      {
        // the cookie's validity has expired
        result.status = SessionStatus::Expired;
        return result;
      }

      // the session has been successfully validated. Shall we do a refresh
      if (refreshValidity)
      {
        ++result.refreshCount;
        result.lastRefresh = now;
        ColumnValueClause cvc;
        cvc.addIntCol(U2S_RefreshCount, result.refreshCount);
        cvc.addDateTimeCol(U2S_LastRefreshTime, &now);
        r.update(cvc);  // no check for errors here; if something fails, the session simply expires earlier

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

    void UserMngr::initTabs(const string& tp)
    {
      TableCreator tc{db};
      tc.addVarchar(US_LoginName, MaxUserNameLen, true, CONFLICT_CLAUSE::FAIL, true, CONFLICT_CLAUSE::FAIL);
      tc.addVarchar(US_Password, 10 + 64, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addVarchar(US_Email, MaxEmailLen);
      tc.addInt(US_CreationTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_LastPwChangeTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_PwExpirationTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_LastAuthSuccessTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_LastAuthFailTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_LoginFailCount, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(US_State, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.createTableAndResetCreator(tp + Tab_User_Ext);

      tc.addForeignKey(U2R_UserRef, tp + Tab_User_Ext, CONSISTENCY_ACTION::CASCADE);
      tc.addInt(U2R_Role, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2R_CreationTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.createTableAndResetCreator(tp + Tab_User2Role_Ext);

      tc.addForeignKey(U2S_UserRef, tp + Tab_User_Ext, CONSISTENCY_ACTION::CASCADE);
      tc.addVarchar(U2S_SessionCookie, MaxSessionCookiLen, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_SessionStartTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_SessionLength_Secs, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_LastRefreshTime, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.addInt(U2S_RefreshCount, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
      tc.createTableAndResetCreator(tp + Tab_User2Session_Ext);
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

    bool UserMngr::checkPasswort(int userId, const string& providedPw) const
    {
      if (userId < 1) return false;

      string saltAndHash;
      try
      {
        TabRow r = tab->getSingleRowByColumnValue("id", userId);
        saltAndHash = r[US_Password];
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

  }

}
