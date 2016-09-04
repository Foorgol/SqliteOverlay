#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "UserMngr.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, UserMngr_createTables)
{
  auto db = getScenario01();

  string prefix = "um";
  string tUser = prefix + UserMngr::Tab_User_Ext;
  string tSession = prefix + UserMngr::Tab_User2Session_Ext;
  string tRole = prefix + UserMngr::Tab_User2Role_Ext;

  ASSERT_TRUE(db->getTab(tUser) == nullptr);
  ASSERT_TRUE(db->getTab(tSession) == nullptr);
  ASSERT_TRUE(db->getTab(tRole) == nullptr);
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_TRUE(db->getTab(tUser) != nullptr);
  ASSERT_TRUE(db->getTab(tSession) != nullptr);
  ASSERT_TRUE(db->getTab(tRole) != nullptr);

}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_createUser)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};

  ASSERT_FALSE(um.hasUser("u"));
  UserMngr::ErrCode e = um.createUser("u", "abc123", 3);
  ASSERT_EQ(UserMngr::ErrCode::Success, e);
  ASSERT_TRUE(um.hasUser("u"));

  // check database entries
  string tUser = prefix + UserMngr::Tab_User_Ext;
  DbTab* t = db->getTab(tUser);
  TabRow r = t->operator [](1);
  ASSERT_EQ("u", r[UserMngr::US_LoginName]);
  ASSERT_TRUE(r[UserMngr::US_Password].size() > 0);
  ASSERT_EQ("0", r[UserMngr::US_LoginFailCount]);
  ASSERT_EQ(static_cast<int>(UserMngr::UserState::Active), r.getInt(UserMngr::US_State));

  // check timestamps for "now + 1 sec" for the unlikely case
  // that one second elapsed since the user creation
  time_t now = time(0);
  for (const string& s : {UserMngr::US_CreationTime, UserMngr::US_LastPwChangeTime, UserMngr::US_LastAuthSuccessTime})
  {
    time_t timestamp = r.getInt(s);
    ASSERT_TRUE(timestamp >= now);
    ASSERT_TRUE(timestamp <= (now + 1));
  }

  // check NULL columns
  for (const string& s : {UserMngr::US_Email, UserMngr::US_PwExpirationTime, UserMngr::US_LastAuthFailTime})
  {
    auto x = r.getString2(s);
    ASSERT_TRUE(x->isNull());
  }

  // create user with expiring password
  e = um.createUser("u2", "abc123", 3, 10);
  ASSERT_EQ(UserMngr::ErrCode::Success, e);
  r = t->operator [](2);
  time_t timestamp = r.getInt(UserMngr::US_PwExpirationTime);
  ASSERT_TRUE(timestamp >= (now + 10));
  ASSERT_TRUE(timestamp <= (now + 11));

  // create invalid entries
  ASSERT_EQ(UserMngr::ErrCode::InvalidName, um.createUser("u2", "abc123", 3, 10));  // exists
  ASSERT_EQ(UserMngr::ErrCode::InvalidName, um.createUser(" ", "abc123", 3, 10));  // empty name
  ASSERT_EQ(UserMngr::ErrCode::InvalidName, um.createUser("", "abc123", 3, 10));  // empty name
  ASSERT_EQ(UserMngr::ErrCode::InvalidPasswort, um.createUser("xyz", "", 3, 10));  // empty PW
  ASSERT_EQ(UserMngr::ErrCode::InvalidPasswort, um.createUser("xyz", "a", 3, 10));  // PW too short
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_authUser)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u", "abc123", 3, 10));

  // reset the value for the last successful authentication
  string tUser = prefix + UserMngr::Tab_User_Ext;
  DbTab* t = db->getTab(tUser);
  TabRow r = t->operator [](1);
  r.update(UserMngr::US_LastAuthSuccessTime, 0);

  UserMngr::AuthInfo ai = um.authenticateUser("u", "abc123");
  ASSERT_EQ(UserMngr::AuthResult::Authenticated, ai.result);

  // check that the value for the last successful login time
  // has been updated
  ASSERT_EQ(time(0), r.getInt(UserMngr::US_LastAuthSuccessTime));

  // check entries of the user data field
  ASSERT_EQ(UTCTimestamp{}, ai.user->lastAuthSuccess);
  ASSERT_EQ(0, ai.user->failCount);

  // test login failures
  ai = um.authenticateUser("xxx", "abc123");
  ASSERT_EQ(UserMngr::AuthResult::UnknownUser, ai.result);
  ASSERT_TRUE(ai.user == nullptr);

  ai = um.authenticateUser("u", "abc");
  ASSERT_EQ(UserMngr::AuthResult::WrongPassword, ai.result);
  ASSERT_TRUE(ai.user != nullptr);

  // check that the value for the last failed login time
  // has been updated
  ASSERT_EQ(time(0), r.getInt(UserMngr::US_LastAuthFailTime));
  ASSERT_EQ(1, r.getInt(UserMngr::US_LoginFailCount));

  // check entries of the user data field
  ASSERT_EQ(UTCTimestamp{}, *(ai.user->lastAuthFail));
  ASSERT_EQ(1, ai.user->failCount);

  // test password expiration
  r.update(UserMngr::US_PwExpirationTime, static_cast<int>(time(0) - 1));
  ai = um.authenticateUser("u", "abc123");
  ASSERT_EQ(UserMngr::AuthResult::PasswordExpired, ai.result);

  // test locked users
  r.update(UserMngr::US_PwExpirationTime, static_cast<int>(time(0) + 100));
  ASSERT_TRUE(um.lockUser("u"));
  ai = um.authenticateUser("u", "abc123");
  ASSERT_EQ(UserMngr::AuthResult::Locked, ai.result);
  ASSERT_TRUE(um.unlockUser("u"));
  ai = um.authenticateUser("u", "abc123");
  ASSERT_EQ(UserMngr::AuthResult::Authenticated, ai.result);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_changePassword)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u", "abc123", 3, 10));

  // store the raw value of the old password
  string tUser = prefix + UserMngr::Tab_User_Ext;
  DbTab* t = db->getTab(tUser);
  TabRow r = t->operator [](1);
  string oldPw_raw = r[UserMngr::US_Password];

  // update PW
  ASSERT_EQ(UserMngr::ErrCode::Success, um.updatePassword("u", "abc123", "xyz", 3));
  ASSERT_TRUE(r[UserMngr::US_Password] != oldPw_raw);
  UserMngr::AuthInfo ai = um.authenticateUser("u", "xyz");
  ASSERT_EQ(UserMngr::AuthResult::Authenticated, ai.result);
  ai = um.authenticateUser("u", "abc123");
  ASSERT_EQ(UserMngr::AuthResult::WrongPassword, ai.result);

  // pw update with change of expiration date
  auto expTime = r.getUTCTime2(UserMngr::US_PwExpirationTime);
  ASSERT_TRUE(expTime->isNull());
  ASSERT_EQ(UserMngr::ErrCode::Success, um.updatePassword("u", "xyz", "abc", 3, 10));
  expTime = r.getUTCTime2(UserMngr::US_PwExpirationTime);
  ASSERT_FALSE(expTime->isNull());
  ASSERT_EQ(time(0) + 10, r.getInt(UserMngr::US_PwExpirationTime));

  ASSERT_EQ(UserMngr::ErrCode::Success, um.updatePassword("u", "abc", "xyz", 3));
  expTime = r.getUTCTime2(UserMngr::US_PwExpirationTime);
  ASSERT_TRUE(expTime->isNull());

  // pw update failures
  ASSERT_EQ(UserMngr::ErrCode::InvalidPasswort, um.updatePassword("u", "xyz", ""));
  ASSERT_EQ(UserMngr::ErrCode::InvalidPasswort, um.updatePassword("u", "xyz", "   123    ", 10));  // too short
  ASSERT_EQ(UserMngr::ErrCode::NotAuthenticated, um.updatePassword("u", "xxx", "aaaaaaaaaaaaaaaaaaaaaaa"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_startSession)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u", "abc123", 3, 10));

  // test successfull authentication
  UserMngr::SessionInfo si = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(si.status, UserMngr::SessionStatus::Created);
  ASSERT_EQ(6, si.cookie.length());
  ASSERT_TRUE(si.user != nullptr);
  ASSERT_EQ("u", si.user->loginName);
  UTCTimestamp now;
  ASSERT_EQ(now, si.sessionStart);
  ASSERT_EQ(now, si.lastRefresh);
  UTCTimestamp nowPlus10{now};
  nowPlus10.applyOffset(10);
  ASSERT_EQ(nowPlus10, si.sessionExpiration);
  ASSERT_EQ(0, si.refreshCount);

  // test the database entries
  string tSession = prefix + UserMngr::Tab_User2Session_Ext;
  DbTab* t = db->getTab(tSession);
  TabRow r = t->operator [](1);
  ASSERT_EQ("1", r[UserMngr::U2S_UserRef]);
  ASSERT_EQ(6, r[UserMngr::U2S_SessionCookie].length());
  ASSERT_EQ(si.cookie, r[UserMngr::U2S_SessionCookie]);
  ASSERT_EQ(now, r.getUTCTime(UserMngr::U2S_SessionStartTime));
  ASSERT_EQ(now, r.getUTCTime(UserMngr::U2S_LastRefreshTime));
  ASSERT_EQ("10", r[UserMngr::U2S_SessionLength_Secs]);
  ASSERT_EQ("0", r[UserMngr::U2S_RefreshCount]);

  // test failures
  si = um.startSession("xxxx", "abc123", 10, 6);  // wrong username
  ASSERT_EQ(si.status, UserMngr::SessionStatus::Invalid);
  si = um.startSession("u", "abc", 10, 6);  // wrong pw
  ASSERT_EQ(si.status, UserMngr::SessionStatus::Invalid);
  si = um.startSession("u", "abc123", 10, 0);  // wrong cookie len
  ASSERT_EQ(si.status, UserMngr::SessionStatus::Invalid);
  si = um.startSession("xxxx", "abc123", -1, 6);  // wrong duration
  ASSERT_EQ(si.status, UserMngr::SessionStatus::Invalid);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_validateSession)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u", "abc123", 3, 10));

  // start a session with 10 seconds validity time
  UserMngr::SessionInfo si = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, si.status);
  ASSERT_EQ(6, si.cookie.length());

  // validate the cookie without session refresh
  si = um.validateSession(si.cookie, false);
  ASSERT_EQ(UserMngr::SessionStatus::Valid, si.status);
  ASSERT_TRUE(si.user != nullptr);
  ASSERT_EQ("u", si.user->loginName);
  UTCTimestamp now;
  ASSERT_EQ(now, si.sessionStart);
  ASSERT_EQ(now, si.lastRefresh);
  UTCTimestamp nowPlus10{now};
  nowPlus10.applyOffset(10);
  ASSERT_EQ(nowPlus10, si.sessionExpiration);
  ASSERT_EQ(0, si.refreshCount);

  // fake the session start time
  string tSession = prefix + UserMngr::Tab_User2Session_Ext;
  DbTab* t = db->getTab(tSession);
  TabRow r = t->operator [](1);
  UTCTimestamp nowMinus11{now};
  nowMinus11.applyOffset(-11);
  r.update(UserMngr::U2S_SessionStartTime, static_cast<int>(nowMinus11.getRawTime()));
  r.update(UserMngr::U2S_LastRefreshTime, static_cast<int>(nowMinus11.getRawTime()));

  // test session expiration
  si = um.validateSession(si.cookie, false);
  ASSERT_EQ(UserMngr::SessionStatus::Expired, si.status);
  ASSERT_TRUE(si.user != nullptr);
  ASSERT_EQ("u", si.user->loginName);

  // forge the session duration (10 secs --> 20 secs)
  r.update(UserMngr::U2S_SessionLength_Secs, 20);

  // test a session refresh
  si = um.validateSession(si.cookie, true);
  ASSERT_EQ(UserMngr::SessionStatus::Refreshed, si.status);
  ASSERT_TRUE(si.user != nullptr);
  ASSERT_EQ("u", si.user->loginName);
  ASSERT_EQ(nowMinus11, si.sessionStart);
  ASSERT_EQ(now, si.lastRefresh);
  UTCTimestamp nowPlus20{now};
  nowPlus20.applyOffset(20);
  ASSERT_EQ(nowPlus20, si.sessionExpiration);
  ASSERT_EQ(1, si.refreshCount);

  // test wrong cookies
  si = um.validateSession("xxxxx");
  ASSERT_EQ(UserMngr::SessionStatus::Invalid, si.status);
  ASSERT_TRUE(si.user == nullptr);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_terminateSession)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u", "abc123", 3));

  // start two sessions with 10 seconds validity time
  UserMngr::SessionInfo s1 = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s1.status);
  UserMngr::SessionInfo s2 = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s2.status);
  string c1 = s1.cookie;
  string c2 = s2.cookie;
  ASSERT_TRUE(c1 != c2);

  // check database entries
  string tSession = prefix + UserMngr::Tab_User2Session_Ext;
  DbTab* t = db->getTab(tSession);
  ASSERT_EQ(2, t->getAllRows().length());

  // terminate session 1
  s1 = um.validateSession(c1, false);
  ASSERT_EQ(UserMngr::SessionStatus::Valid, s1.status);
  ASSERT_EQ(UserMngr::ErrCode::Success, um.terminateSession(c1));
  s1 = um.validateSession(c1, false);
  ASSERT_EQ(UserMngr::SessionStatus::Invalid, s1.status);
  ASSERT_EQ(1, t->getAllRows().length());
  s2 = um.validateSession(c2, false);
  ASSERT_EQ(UserMngr::SessionStatus::Valid, s2.status);

  // re-start session 1
  s1 = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s1.status);
  c1 = s1.cookie;
  ASSERT_TRUE(c1 != c2);
  s1 = um.validateSession(c1, false);
  ASSERT_EQ(UserMngr::SessionStatus::Valid, s1.status);

  // terminate all user sessions
  ASSERT_EQ(UserMngr::ErrCode::Success, um.terminateAllUserSessions("u"));
  ASSERT_EQ(0, t->getAllRows().length());
  s1 = um.validateSession(c1, false);
  ASSERT_EQ(UserMngr::SessionStatus::Invalid, s1.status);
  s2 = um.validateSession(c2, false);
  ASSERT_EQ(UserMngr::SessionStatus::Invalid, s2.status);

  // test invalid users or cookies
  ASSERT_EQ(UserMngr::ErrCode::InvalidName, um.terminateAllUserSessions("sdkfhsdf"));
  ASSERT_EQ(UserMngr::ErrCode::InvalidName, um.terminateSession("sdkfhsdf"));

  // test user without active session
  ASSERT_EQ(UserMngr::ErrCode::Success, um.terminateAllUserSessions("u"));

  // test deletion of all sessions
  s1 = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s1.status);
  s2 = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s2.status);
  ASSERT_EQ(2, t->length()); // two active sessions
  ASSERT_TRUE(um.terminateAllSessions());
  ASSERT_EQ(0, t->length()); // all sessions deleted
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_cleanupSessions)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u", "abc123", 3));

  // a call without sessions must succeed
  ASSERT_TRUE(um.cleanupExpiredSessions());

  // start two sessions with 10 seconds validity time
  UserMngr::SessionInfo s1 = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s1.status);
  UserMngr::SessionInfo s2 = um.startSession("u", "abc123", 10, 6);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s2.status);

  // fake the expiration time of the first sessions
  string tSession = prefix + UserMngr::Tab_User2Session_Ext;
  DbTab* t = db->getTab(tSession);
  TabRow r = t->operator [](1);
  int nowMinus20 = time(0) - 20;
  r.update(UserMngr::U2S_LastRefreshTime, nowMinus20);
  r.update(UserMngr::U2S_SessionStartTime, nowMinus20);

  // make sure the session is expired
  s1 = um.validateSession(s1.cookie, false);
  ASSERT_EQ(UserMngr::SessionStatus::Expired, s1.status);

  // bulk-delete all expired sessions
  ASSERT_TRUE(um.cleanupExpiredSessions());

  // only session 2 should have survived
  ASSERT_EQ(1, t->length());
  s1 = um.validateSession(s1.cookie, false);
  ASSERT_EQ(UserMngr::SessionStatus::Invalid, s1.status);
  s2 = um.validateSession(s2.cookie, false);
  ASSERT_EQ(UserMngr::SessionStatus::Valid, s2.status);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_roles)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u", "abc123", 3));

  enum class Roles
  {
    R1,
    R2,
    R3
  };

  // getters, part 1
  ASSERT_TRUE(um.getAllRoles<Roles>("u").empty());
  ASSERT_FALSE(um.hasRole<Roles>("u", Roles::R1));

  // role assignment
  ASSERT_TRUE(um.assignRole<Roles>("u", Roles::R2));
  auto ar = um.getAllRoles<Roles>("u");
  ASSERT_EQ(1, ar.size());
  ASSERT_EQ(Roles::R2, ar[0]);
  ASSERT_TRUE(um.hasRole<Roles>("u", Roles::R2));
  ASSERT_TRUE(um.assignRole<Roles>("u", Roles::R2));  // assign the same role again
  ar = um.getAllRoles<Roles>("u");
  ASSERT_EQ(1, ar.size());   // still only one entry, no duplicates
  ASSERT_TRUE(um.assignRole<Roles>("u", Roles::R1));  // assign another role
  ar = um.getAllRoles<Roles>("u");
  ASSERT_EQ(2, ar.size());
  ASSERT_TRUE(um.hasRole<Roles>("u", Roles::R2));
  ASSERT_TRUE(um.hasRole<Roles>("u", Roles::R1));

  // role removal
  ASSERT_TRUE(um.removeRole<Roles>("u", Roles::R2));
  ar = um.getAllRoles<Roles>("u");
  ASSERT_EQ(1, ar.size());
  ASSERT_FALSE(um.hasRole<Roles>("u", Roles::R2));
  ASSERT_TRUE(um.hasRole<Roles>("u", Roles::R1));
  ASSERT_TRUE(um.removeRole<Roles>("u", Roles::R2));  // R2 not set for "u", call shall succeed anyway

  // invalid calls
  ASSERT_FALSE(um.removeRole<Roles>("xxx", Roles::R2));  // invalid user
  ASSERT_FALSE(um.assignRole<Roles>("xxx", Roles::R2));  // invalid user
  ASSERT_TRUE(um.getAllRoles<Roles>("xxx").empty());  // invalid user
  ASSERT_FALSE(um.hasRole<Roles>("xxx", Roles::R2));  // invalid user
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, UserMngr_delUser)
{
  auto db = getScenario01();
  string prefix = "um";
  UserMngr::UserMngr um{db.get(), prefix};

  // create two users
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u1", "abc123", 3));
  ASSERT_EQ(UserMngr::ErrCode::Success, um.createUser("u2", "def123", 3));

  // create sessions
  auto s1 = um.startSession("u1", "abc123", 10);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s1.status);
  auto s2 = um.startSession("u1", "abc123", 10);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s2.status);
  auto s3 = um.startSession("u2", "def123", 10);
  ASSERT_EQ(UserMngr::SessionStatus::Created, s3.status);

  // assign a few roles
  enum class Roles
  {
    R1,
    R2,
    R3
  };
  ASSERT_TRUE(um.assignRole<Roles>("u1", Roles::R2));
  ASSERT_TRUE(um.assignRole<Roles>("u1", Roles::R1));
  ASSERT_TRUE(um.assignRole<Roles>("u2", Roles::R2));
  ASSERT_TRUE(um.assignRole<Roles>("u2", Roles::R1));

  // check initial table lengths
  DbTab* tUser = db->getTab(prefix + UserMngr::Tab_User_Ext);
  DbTab* tRole = db->getTab(prefix + UserMngr::Tab_User2Role_Ext);
  DbTab* tSession = db->getTab(prefix + UserMngr::Tab_User2Session_Ext);
  ASSERT_EQ(2, tUser->length());  // two users
  ASSERT_EQ(3, tSession->length());  // three active sessions
  ASSERT_EQ(4, tRole->length());  // four role assignments

  // delete u1
  ASSERT_EQ(UserMngr::ErrCode::Success, um.deleteUser("u1"));
  ASSERT_FALSE(um.hasUser("u1"));

  // make sure u2 is still intact
  ASSERT_TRUE(um.hasUser("u2"));
  ASSERT_EQ(UserMngr::SessionStatus::Valid, um.validateSession(s3.cookie, false).status);
  ASSERT_TRUE(um.hasRole<Roles>("u2", Roles::R2));
  ASSERT_TRUE(um.hasRole<Roles>("u2", Roles::R1));

  // check final table lengths
  ASSERT_EQ(1, tUser->length());  // one user
  ASSERT_EQ(1, tSession->length());  // one active sessions
  ASSERT_EQ(2, tRole->length());  // two role assignments
}
