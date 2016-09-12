#ifndef SQLITE_OVERLAY_USERMNGR_H
#define SQLITE_OVERLAY_USERMNGR_H

#include <vector>

#include "GenericObjectManager.h"
#include "Sloppy/DateTime/DateAndTime.h"

using namespace boost::gregorian;

namespace SqliteOverlay {

  namespace UserMngr
  {
    static constexpr const char* Tab_User_Ext = "_User";
    static constexpr const char* US_LoginName = "LoginName";
    static constexpr const char* US_Email = "Email";
    static constexpr const char* US_CreationTime = "CreationTime";
    static constexpr const char* US_LastAuthSuccessTime = "LastAuthSuccessTime";
    static constexpr const char* US_LastAuthFailTime = "LastAuthFailTime";
    static constexpr const char* US_LoginFailCount = "LoginFailCount";
    static constexpr const char* US_State = "State";

    static constexpr const char* Tab_User2Password_Ext = "_Password";
    static constexpr const char* U2P_UserRef = "UserRef";
    static constexpr const char* U2P_Password = "Password";
    static constexpr const char* U2P_CreationTime = "CreationTime";
    static constexpr const char* U2P_ExpirationTime = "ExpiresOn";
    static constexpr const char* U2P_DisablingTime = "DisabledOn";

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
      Invalid
    };

    enum class ErrCode
    {
      Success,
      InvalidName,
      InvalidPassword,
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
          roleTab{_db->getTab(tablePrefix + string{Tab_User2Role_Ext})},
          sessionTab{_db->getTab(tablePrefix + string{Tab_User2Session_Ext})},
          pwTab{_db->getTab(tablePrefix + string{Tab_User2Password_Ext})}
      {
        if (db == nullptr)
        {
          throw invalid_argument("UserMngr: got nullptr for database handle!");
        }
        if (tab == nullptr)   // if tab could not be initialized, our own tables do not yet exist
        {
          initTabs(tablePrefix);
        }
      }

      // user management
      ErrCode createUser(const string& name, const string& pw, int minPwLen=DefaultMinPwLen, int pwExiration__secs = -1, bool createAsLocked=false,
                         int saltLen = DefaultSaltLen, int hashCycles = DefaultHashCycles) const;
      ErrCode setEmail(const string& name, const string& email) const;
      ErrCode updatePassword(const string& name, const string& oldPw, const string& newPw, int historyCheckDepth=1, int minPwLen=DefaultMinPwLen, int pwExiration__secs = -1,
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
      bool terminateAllSessions() const;
      bool cleanupExpiredSessions() const;

      // role management
      template<typename RoleEnum>
      inline bool assignRole(const string& name, RoleEnum r) const
      {
        return assignRole(name, static_cast<int>(r));
      }

      template<typename RoleEnum>
      inline bool removeRole(const string& name, RoleEnum r) const
      {
        return removeRole(name, static_cast<int>(r));
      }

      template<typename RoleEnum>
      inline bool hasRole(const string& name, RoleEnum r) const
      {
        return hasRole(name, static_cast<int>(r));
      }

      template<typename RoleEnum>
      inline vector<RoleEnum> getAllRoles(const string& name) const
      {
        vector<RoleEnum> result;

        int uid = getIdForUser(name);
        if (uid < 1) return result;

        auto it = roleTab->getRowsByColumnValue(U2R_UserRef, uid);
        while (!(it.isEnd()))
        {
          TabRow r = *it;
          result.push_back(static_cast<RoleEnum>(r.getInt(U2R_Role)));
          ++it;
        }

        return result;
      }

    protected:
      void initTabs(const string& tp);
      int getIdForUser(const string& name) const;
      bool checkPasswort(int userId, const string& providedPw) const;
      bool assignRole(const string& name, int roleId) const;
      bool removeRole(const string& name, int roleId) const;
      bool hasRole(const string& name, int roleId) const;
      bool hasRole(int uid, int roleId) const;

    private:
      DbTab* roleTab;
      DbTab* sessionTab;
      DbTab* pwTab;
    };
  }
}

#endif  /* SQLITE_OVERLAY_USERMNGR_H */
