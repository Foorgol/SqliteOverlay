/*
 *    This is SqliteOverlay, a database abstraction layer on top of SQLite.
 *    Copyright (C) 2015  Volker Knollmann
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Transaction.h"

namespace SqliteOverlay
{

  Transaction::Transaction(const SqliteDatabase* _db, TransactionType tt, TransactionDtorAction _dtorAct)
    :db(_db), dtorAct(_dtorAct), isFinished(false)
  {
    if (db == nullptr)
    {
      throw invalid_argument("Received NULL handle for database in Transaction ctor");
    }

    string sql;
    savepointName.clear();

    // Is another transaction already in progress?
    if (db->isAutoCommit() == false)
    {
      // No autocommit, so another transaction has
      // already been started outside this of this
      // constructor. Thus we create a savepoint
      // within the outer transaction

      // create a "unique" savepoint name. We use
      // a combination of a 5-digit random number
      // and the current time. Should be sufficiently
      // unique
      savepointName = "SP" + to_string(rand() % 100000);
      savepointName += to_string(time(nullptr));

      // create the savepoint
      sql = "SAVEPOINT " + savepointName;
    } else {
      // autocommit is set to yes, so no other
      // transaction is already in progress.
      // now let's create such a transaction
      sql = "BEGIN ";
      switch (tt)
      {
      case TransactionType::Deferred:
        sql += "DEFERRED";
        break;

      case TransactionType::Exclusive:
        sql += "EXCLUSIVE";
        break;

      default:
        sql += "IMMEDIATE";
      }
      sql += " TRANSACTION";
    }

    // try to acquire the database lock
    db->execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  Transaction::Transaction(Transaction&& other)
  {
    operator=(std::move(other));
  }

  //----------------------------------------------------------------------------

  Transaction& Transaction::operator=(Transaction&& other)
  {
    db = other.db;
    other.db = nullptr;

    dtorAct =other.dtorAct;

    savepointName = other.savepointName;
    other.savepointName.clear();

    isFinished = other.isFinished;
    other.isFinished = true;

    return *this;
  }

//----------------------------------------------------------------------------

  bool Transaction::isActive() const
  {
    return (!(db->isAutoCommit()) && !isFinished);
  }

  //----------------------------------------------------------------------------

  void Transaction::commit()
  {
    try
    {
      if (savepointName.empty())
      {
        db->execNonQuery("COMMIT");   // we're the outermost transaction
      } else {
        db->execNonQuery("RELEASE " + savepointName);   // we're an inner, nested transaction
      }
    }
    catch (GenericSqliteException e)
    {
      // re-throw anything but in case of a "SQL logic error"
      // tag this transaction as finished because the
      // logic error indicated that the transaction has already been committed / rolled back
      // by an outer transaction
      //
      // tagging this transaction as finished avoids termination
      // of the programm when we hit the dtor
      if (e.errCode() == PrimaryResultCode::ERROR) isFinished = true;
      throw;
    }

    isFinished = true;
  }

  //----------------------------------------------------------------------------

  void Transaction::rollback()
  {
    try
    {
      if (savepointName.empty())
      {
        db->execNonQuery("ROLLBACK");   // we're the outermost transaction
      } else {
        db->execNonQuery("ROLLBACK TO " + savepointName);   // we're an inner, nested transaction
      }
    }
    catch (GenericSqliteException e)
    {
      // re-throw anything but in case of a "SQL logic error"
      // tag this transaction as finished because the
      // logic error indicated that the transaction has already been committed / rolled back
      // by an outer transaction
      //
      // tagging this transaction as finished avoids termination
      // of the programm when we hit the dtor
      if (e.errCode() == PrimaryResultCode::ERROR) isFinished = true;
      throw;
    }

    isFinished = true;
  }

  //----------------------------------------------------------------------------

  bool Transaction::isNested() const
  {
    return (!(savepointName.empty()));
  }

  //----------------------------------------------------------------------------

  Transaction::~Transaction()
  {
    if (isFinished) return;

    // the transaction has not been finalized yet.
    // What to do?
    string sql;
    if (dtorAct == TransactionDtorAction::Commit)
    {
      if (savepointName.empty())
      {
        sql = "COMMIT";
      } else {
        sql = "RELEASE SAVEPOINT " + savepointName;
      }
    }
    if (dtorAct == TransactionDtorAction::Rollback)
    {
      sql = "ROLLBACK";
      if (!(savepointName.empty()))
      {
        sql += " TO SAVEPOINT " + savepointName;
      }
    }

    if (!(sql.empty()) && (db != nullptr))
    {
      db->execNonQuery(sql);
    }
  }

//----------------------------------------------------------------------------


    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
}
