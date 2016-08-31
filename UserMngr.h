#ifndef SQLITE_OVERLAY_USERMNGR_H
#define SQLITE_OVERLAY_USERMNGR_H

#include "GenericObjectManager.h"
#include "Sloppy/DateTime/DateAndTime.h"

using namespace SqliteOverlay;
using namespace boost::gregorian;

namespace RankingApp {

  namespace UserMngr
  {
    static constexpr const char* Tab_User_Ext = "_User";
    static constexpr const char* US_LoginName = "LoginName";
    static constexpr const char* US_Password = "Password";
    static constexpr const char* US_Email = "Email";
    static constexpr const char* US_CreationTime = "CreationTime";
    static constexpr const char* US_LastPwChangeTime = "LastPwTime";
    static constexpr const char* US_PwExpirationTime = "PwExpirationTime";
    static constexpr const char* US_LastAuthSuccessTime = "LastAuthSuccessTime";
    static constexpr const char* US_LastAuthFailTime = "LastAuthFailTime";
    static constexpr const char* US_LoginFailCount = "LoginFailCount";
    static constexpr const char* US_State = "State";

    static constexpr const char* Tab_User2Role_Ext = "_User2Role";
    static constexpr const char* U2R_UserRef = "UserRef";
    static constexpr const char* U2R_Role = "Role";
    static constexpr const char* U2R_CreationTime = "CreationTime";

    static constexpr const char* Tab_User2Session_Ext = "_User2Session";
    static constexpr const char* U2S_UserRef = "UserRef";
    static constexpr const char* U2S_SessionCookie = "Cookie";
    static constexpr const char* U2S_SessionStartTime = "StartTime";
    static constexpr const char* U2S_SessionLength_Secs = "Length_secs";
    static constexpr const char* U2S_LastRefreshTime = "LastRefreshTime";
    static constexpr const char* U2S_RefreshCount = "RefreshCount";

    static constexpr int MaxUserNameLen = 100;
    static constexpr int DefaultMinPwLen = 3;
    static constexpr int MaxPwLen = 100;
    static constexpr int MaxEmailLen = 200;
    static constexpr int DefaultSaltLen = 8;
    static constexpr int MaxSaltLen = 30;
    static constexpr int MinSaltLen = 30;
    static constexpr int DefaultHashCycles = 1000;
    static constexpr int DefaultSessionCookiLen = 30;
    static constexpr int MaxSessionCookiLen = 100;

    enum class UserState
    {
      Active,
      Locked,
    };

    enum class AuthResult
    {
      Authenticated,
      UnknownUser,
      WrongPassword,
      PasswordExpired,
      Locked,
    };

    enum class SessionStatus
    {
      Created,
      Valid,
      Refreshed,
      Expired,
      Unknown
    };

    enum class ErrCode
    {
      Success,
      InvalidName,
      InvalidPasswort,
      NotAuthenticated,
      NotUnique,
      DatabaseError,
      UnspecifiedError
    };

    struct UserData
    {
      string loginName;
      string email;
      Sloppy::DateTime::UTCTimestamp userCreationTime;
      unique_ptr<Sloppy::DateTime::UTCTimestamp> pwExpirationTime;
      Sloppy::DateTime::UTCTimestamp lastPwChange;
      Sloppy::DateTime::UTCTimestamp lastAuthSuccess;
      unique_ptr<Sloppy::DateTime::UTCTimestamp> lastAuthFail;
      int failCount;
      UserState state;
    };

    struct AuthInfo
    {
      AuthResult result;
      unique_ptr<UserData> user;
    };

    struct SessionInfo
    {
      string cookie;
      unique_ptr<UserData> user;
      Sloppy::DateTime::UTCTimestamp sessionStart;
      Sloppy::DateTime::UTCTimestamp sessionExpiration;
      Sloppy::DateTime::UTCTimestamp lastRefresh;
      int refreshCount;
      SessionStatus status;
    };

    class UserMngr : public GenericObjectManager<SqliteDatabase>
    {
    public:
      UserMngr(SqliteDatabase* _db, const string& tablePrefix)
        :GenericObjectManager{_db, tablePrefix + string{Tab_User_Ext}},
          roleTab{db->getTab(tablePrefix + string{Tab_User2Role_Ext})},
          sessionTab{db->getTab(tablePrefix + string{Tab_User2Session_Ext})}
      {
        if (tab == nullptr)   // if tab could not be initialized, our own tables do not yet exist
        {
          initTabs(tablePrefix);
        }
      }

      // user management
      ErrCode createUser(const string& name, const string& pw, int minPwLen=DefaultMinPwLen, int pwExiration__secs = -1,
                         int saltLen = DefaultSaltLen, int hashCycles = DefaultHashCycles) const;
      ErrCode setEmail(const string& name, const string& email) const;
      ErrCode updatePassword(const string& name, const string& oldPw, const string& newPw, int minPwLen=DefaultMinPwLen, int pwExiration__secs = -1,
                             int saltLen = DefaultSaltLen, int hashCycles = DefaultHashCycles) const;
      ErrCode deleteUser(const string& name) const;
      bool lockUser(const string& name) const;
      bool unlockUser(const string& name) const;
      unique_ptr<UserData> getUserData(const string& userName) const;
      unique_ptr<UserData> getUserData(int id) const;

      // authentication
      bool hasUser(const string& name);
      AuthInfo authenticateUser(const string& name, const string& pw) const;

      // session management
      SessionInfo startSession(const string& name, const string& pw, int sessionExpiration__secs, int cookieLen = DefaultSessionCookiLen) const;
      SessionInfo validateSession(const string& cookie, bool refreshValidity=true) const;
      ErrCode terminateSession(const string& cookie) const;
      ErrCode terminateAllUserSessions(const string& name) const;

    protected:
      void initTabs(const string& tp);
      int getIdForUser(const string& name) const;
      bool checkPasswort(int userId, const string& providedPw) const;

    private:
      DbTab* roleTab;
      DbTab* sessionTab;

    };
  }
}

#endif  /* SQLITE_OVERLAY_USERMNGR_H */
