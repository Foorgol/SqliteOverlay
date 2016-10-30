#include <boost/algorithm/string.hpp>

#include <Sloppy/Crypto/Crypto.h>

#include "TabRow.h"
#include "Transaction.h"
#include "TableCreator.h"

#include "ActionMngr.h"

namespace SqliteOverlay {

  namespace ActionMngr
  {
    string Action::getNonce() const
    {
      return row[PA_Nonce];
    }

    //----------------------------------------------------------------------------

    string Action::getOtherData() const
    {
      return row[PA_OtherData];
    }

    //----------------------------------------------------------------------------

    ScalarQueryResult<string> Action::getOtherData2() const
    {
      auto d = row.getString2(PA_OtherData);

      if (d == nullptr) return ScalarQueryResult<string>{};

      return *d;
    }

    //----------------------------------------------------------------------------

    unique_ptr<UTCTimestamp> Action::getExpirationDate() const
    {
      auto ed = row.getUTCTime2(PA_ExpiresOn);
      if (ed == nullptr) return nullptr;
      if (ed->isNull()) return nullptr;

      return make_unique<UTCTimestamp>(ed->get());
    }

    //----------------------------------------------------------------------------

    UTCTimestamp Action::getCreationDate() const
    {
      return row.getUTCTime(PA_CreatedOn);
    }

    //----------------------------------------------------------------------------

    bool Action::setOtherData(const string& dat) const
    {
      return row.update(PA_OtherData, dat);
    }

    //----------------------------------------------------------------------------

    bool Action::removeOtherData() const
    {
      return row.updateToNull(PA_OtherData);
    }

    //----------------------------------------------------------------------------

    bool Action::hasOtherData() const
    {
      auto d = row.getString2(PA_OtherData);
      if (d == nullptr) return false;
      return (!(d->isNull()));
    }

    //----------------------------------------------------------------------------

    bool Action::isExpired() const
    {
      unique_ptr<UTCTimestamp> ed = getExpirationDate();
      if (ed ==  nullptr) return false;  // nonce has infinite validity

      UTCTimestamp now;
      return (now > *ed);
    }

    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------

    ActionMngr::ActionMngr(SqliteDatabase* _db, const string& actionTabName)
      :GenericObjectManager{_db, actionTabName}
    {
      if (db == nullptr)
      {
        throw invalid_argument("ActionMngr: got nullptr for database handle!");
      }
      if (tab == nullptr)   // if tab could not be initialized, our own table does not yet exist
      {
        TableCreator tc{db};
        tc.addVarchar(PA_Nonce, NonceLength, true, CONFLICT_CLAUSE::FAIL, true, CONFLICT_CLAUSE::FAIL);
        tc.addInt(PA_Action, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
        tc.addText(PA_OtherData);
        tc.addInt(PA_CreatedOn, false, CONFLICT_CLAUSE::__NOT_SET, true, CONFLICT_CLAUSE::FAIL);
        tc.addInt(PA_ExpiresOn);
        tab = tc.createTableAndResetCreator(actionTabName);
      }
    }

    //----------------------------------------------------------------------------

    unique_ptr<Action> ActionMngr::createNewAction(int actCode, int validity_secs) const
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
      int newId = tab->insertRow(cvc, &dbErr);
      if ((newId < 1) || (dbErr != SQLITE_DONE))
      {
        return nullptr;
      }

      return getActionById(newId);
    }

    //----------------------------------------------------------------------------

    bool ActionMngr::removeExpiredActions() const
    {
      UTCTimestamp now;
      WhereClause w;
      w.addDateTimeCol(PA_ExpiresOn, "<", &now);

      int dbErr;
      int deletedRows = tab->deleteRowsByWhereClause(w, &dbErr);

      return ((deletedRows >= 0) && (dbErr == SQLITE_DONE));
    }

    //----------------------------------------------------------------------------

    bool ActionMngr::removeAction(const Action& act) const
    {
      int dbErr;
      int rowsAffected = tab->deleteRowsByColumnValue("id", act.getId(), &dbErr);

      return ((rowsAffected > 0) && (dbErr == SQLITE_DONE));
    }

    //----------------------------------------------------------------------------

    unique_ptr<Action> ActionMngr::getActionById(int id) const
    {
      return getSingleObjectByColumnValue<Action>("id", id);
    }

    //----------------------------------------------------------------------------

    unique_ptr<Action> ActionMngr::getActionByNonce(const string& nonce) const
    {
      return getSingleObjectByColumnValue<Action>(PA_Nonce, nonce);
    }

    //----------------------------------------------------------------------------

    bool ActionMngr::hasNonce(const string& nonce) const
    {
      return (tab->getMatchCountForColumnValue(PA_Nonce, nonce) > 0);
    }


    //----------------------------------------------------------------------------


    //----------------------------------------------------------------------------


    //----------------------------------------------------------------------------


    //----------------------------------------------------------------------------


    //----------------------------------------------------------------------------
  }
}
