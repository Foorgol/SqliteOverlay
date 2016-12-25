#ifndef SQLITE_OVERLAY__ACTION_MNGR_H
#define SQLITE_OVERLAY__ACTION_MNGR_H

#include <memory>
#include <vector>

#include <boost/date_time/gregorian/gregorian.hpp>

#include <Sloppy/Crypto/Crypto.h>

#include "GenericObjectManager.h"
#include "GenericDatabaseObject.h"
#include "TableCreator.h"

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

    template <typename DbType, typename ActionEnum>
    class Action : public GenericDatabaseObject<DbType>
    {
      template <typename DbType_, typename ActionType, typename ActionEnum_>
      friend class ActionMngr;

      friend class GenericObjectManager<DbType>;

    public:
      // getters
      string getNonce() const
      {
        return GenericDatabaseObject<DbType>::row[PA_Nonce];
      }

      //----------------------------------------------------------------------------

      string getOtherData() const   // only use this, if PA_OtherData is NOT NULL!!
      {
        return GenericDatabaseObject<DbType>::row[PA_OtherData];
      }

      //----------------------------------------------------------------------------

      ScalarQueryResult<string> getOtherData2() const
      {
        auto d = GenericDatabaseObject<DbType>::row.getString2(PA_OtherData);

        if (d == nullptr) return ScalarQueryResult<string>{};

        return *d;
      }

      //----------------------------------------------------------------------------

      unique_ptr<UTCTimestamp> getExpirationDate() const
      {
        auto ed = GenericDatabaseObject<DbType>::row.getUTCTime2(PA_ExpiresOn);
        if (ed == nullptr) return nullptr;
        if (ed->isNull()) return nullptr;

        return make_unique<UTCTimestamp>(ed->get());
      }

      //----------------------------------------------------------------------------

      UTCTimestamp getCreationDate() const
      {
        return GenericDatabaseObject<DbType>::row.getUTCTime(PA_CreatedOn);
      }

      //----------------------------------------------------------------------------

      ActionEnum getType() const {
        return static_cast<ActionEnum>(GenericDatabaseObject<DbType>::row.getInt(PA_Action));
      }

      // setters
      bool setOtherData(const string& dat) const
      {
        return GenericDatabaseObject<DbType>::row.update(PA_OtherData, dat);
      }

      bool removeOtherData() const
      {
        return GenericDatabaseObject<DbType>::row.updateToNull(PA_OtherData);
      }

      // queries
      bool hasOtherData() const
      {
        auto d = GenericDatabaseObject<DbType>::row.getString2(PA_OtherData);
        if (d == nullptr) return false;
        return (!(d->isNull()));
      }

      bool isExpired() const
      {
        unique_ptr<UTCTimestamp> ed = getExpirationDate();
        if (ed ==  nullptr) return false;  // nonce has infinite validity

        UTCTimestamp now;
        return (now > *ed);
      }

    protected:
      //Action(SqliteDatabase* db, int rowId)
      //  :GenericDatabaseObject(db, TabPendingActions, rowId) {}

      Action(DbType* db, TabRow row)
        :GenericDatabaseObject<DbType>(db, row) {}

    };

    //----------------------------------------------------------------------------

    template <typename DbType, typename ActionType, typename ActionEnum>
    class ActionMngr : public GenericObjectManager<DbType>
    {
    public:
      ActionMngr(DbType* _db, const string& actionTabName)
        :GenericObjectManager<DbType>{_db, actionTabName}
      {
        if (GenericObjectManager<DbType>::db == nullptr)
        {
          throw invalid_argument("ActionMngr: got nullptr for database handle!");
        }
        if (GenericObjectManager<DbType>::tab == nullptr)   // if tab could not be initialized, our own table does not yet exist
        {
          TableCreator tc{GenericObjectManager<DbType>::db};
          tc.addVarchar(PA_Nonce, NonceLength, true, CONFLICT_CLAUSE::FAIL, true, CONFLICT_CLAUSE::FAIL);
          tc.addInt(PA_Action, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
          tc.addText(PA_OtherData);
          tc.addInt(PA_CreatedOn, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
          tc.addInt(PA_ExpiresOn);
          GenericObjectManager<DbType>::tab = tc.createTableAndResetCreator(actionTabName);
        }
      }

      // creation
      unique_ptr<ActionType> createNewAction(ActionEnum act, int validity_secs) const
      {
        return createNewAction(static_cast<int>(act), validity_secs);
      }

      //----------------------------------------------------------------------------
      // deletion

      bool removeExpiredActions() const
      {
        UTCTimestamp now;
        WhereClause w;
        w.addDateTimeCol(PA_ExpiresOn, "<", &now);

        int dbErr;
        int deletedRows = GenericObjectManager<DbType>::tab->deleteRowsByWhereClause(w, &dbErr);

        return ((deletedRows >= 0) && (dbErr == SQLITE_DONE));
      }

      //----------------------------------------------------------------------------

      bool removeAction(const ActionType& act) const
      {
        int dbErr;
        int rowsAffected = GenericObjectManager<DbType>::tab->deleteRowsByColumnValue("id", act.getId(), &dbErr);

        return ((rowsAffected > 0) && (dbErr == SQLITE_DONE));
      }

      //----------------------------------------------------------------------------
      // getters

      unique_ptr<ActionType> getActionById(int id) const
      {
        using gom = GenericObjectManager<DbType>;
        return gom::template getSingleObjectByColumnValue<ActionType>("id", id);
      }

      //----------------------------------------------------------------------------

      unique_ptr<ActionType> getActionByNonce(const string& nonce) const
      {
        using gom = GenericObjectManager<DbType>;
        return gom::template getSingleObjectByColumnValue<ActionType>(PA_Nonce, nonce);
      }

      //----------------------------------------------------------------------------

      vector<ActionType> getActionsByType(ActionEnum act) const {
        using gom = GenericObjectManager<DbType>;
        return gom::template getObjectsByColumnValue<ActionType>(PA_Action, static_cast<int>(act));
      }

      //----------------------------------------------------------------------------
      // queries
      bool hasNonce(const string& nonce) const
      {
        return (GenericObjectManager<DbType>::tab->getMatchCountForColumnValue(PA_Nonce, nonce) > 0);
      }

    protected:
      unique_ptr<ActionType> createNewAction(int actCode, int validity_secs) const
      {
        // create a new, unique nonce
        string nonce;
        while (nonce.empty() || (hasNonce(nonce)))
        {
          nonce = Sloppy::Crypto::getRandomAlphanumString(NonceLength);
        }

        ColumnValueClause cvc;
        cvc.addStringCol(PA_Nonce, nonce);
        cvc.addIntCol(PA_Action, actCode);
        UTCTimestamp now;
        cvc.addDateTimeCol(PA_CreatedOn, &now);
        if (validity_secs > 0)
        {
          now.applyOffset(validity_secs);
          cvc.addDateTimeCol(PA_ExpiresOn, &now);
        }
        int dbErr;
        int newId = GenericObjectManager<DbType>::tab->insertRow(cvc, &dbErr);
        if ((newId < 1) || (dbErr != SQLITE_DONE))
        {
          return nullptr;
        }

        return getActionById(newId);
      }
    };
  }
}
#endif
