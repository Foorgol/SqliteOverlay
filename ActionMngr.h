#ifndef SQLITE_OVERLAY__ACTION_MNGR_H
#define SQLITE_OVERLAY__ACTION_MNGR_H

#include <memory>
#include <vector>

#include <boost/date_time/gregorian/gregorian.hpp>

#include <SqliteOverlay/GenericObjectManager.h>
#include <SqliteOverlay/GenericDatabaseObject.h>

namespace SqliteOverlay {

  namespace ActionMngr
  {
    // column names
    static constexpr const char* PA_Nonce = "Nonce";
    static constexpr const char* PA_Action = "Action";
    static constexpr const char* PA_OtherData = "Other";
    static constexpr const char* PA_CreatedOn = "CreatedOn";
    static constexpr const char* PA_ExpiresOn = "ExpiresOn";

    // constants
    static constexpr int NonceLength = 20;

    //----------------------------------------------------------------------------

    class Action : public GenericDatabaseObject<SqliteDatabase>
    {
      friend class ActionMngr;
      friend class GenericObjectManager<SqliteDatabase>;

    public:
      // getters
      string getNonce() const;
      string getOtherData() const;   // only use this, if PA_OtherData is NOT NULL!!
      ScalarQueryResult<string> getOtherData2() const;
      unique_ptr<UTCTimestamp> getExpirationDate() const;
      UTCTimestamp getCreationDate() const;
      template<typename ActionType>
      ActionType getType() const {
        return static_cast<ActionType>(row.getInt(PA_Action));
      }

      // setters
      bool setOtherData(const string& dat) const;
      bool removeOtherData() const;

      // queries
      bool hasOtherData() const;
      bool isExpired() const;


    private:
      //Action(SqliteDatabase* db, int rowId)
      //  :GenericDatabaseObject(db, TabPendingActions, rowId) {}

      Action(SqliteDatabase* db, TabRow row)
        :GenericDatabaseObject(db, row) {}

    };

    //----------------------------------------------------------------------------

    class ActionMngr : public GenericObjectManager<SqliteDatabase>
    {
    public:
      ActionMngr(SqliteDatabase* _db, const string& actionTabName);

      // creation
      template<typename ActionType>
      unique_ptr<Action> createNewAction(ActionType act, int validity_secs) const
      {
        return createNewAction(static_cast<int>(act), validity_secs);
      }

      // deletion
      bool removeExpiredActions() const;
      bool removeAction(const Action& act) const;

      // getters
      unique_ptr<Action> getActionById(int id) const;
      unique_ptr<Action> getActionByNonce(const string& nonce) const;

      template<typename ActionType>
      vector<Action> getActionsByType(ActionType act) const {
        return getObjectsByColumnValue<Action>(PA_Action, static_cast<int>(act));
      }

      // queries
      bool hasNonce(const string& nonce) const;

    protected:
      unique_ptr<Action> createNewAction(int actCode, int validity_secs) const;
    };
  }
}
#endif
