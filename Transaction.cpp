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

  unique_ptr<Transaction> Transaction::startNew(SqliteDatabase* _db, TRANSACTION_TYPE tt, TRANSACTION_DESTRUCTOR_ACTION _dtorAct, int* errCodeOut)
  {
    Transaction* tmpPtr;
    try
    {
      tmpPtr = new Transaction(_db, tt, _dtorAct, errCodeOut);
    } catch (exception e) {
      return nullptr;
    }

    return unique_ptr<Transaction>(tmpPtr);
  }

  //----------------------------------------------------------------------------

  bool Transaction::isActive() const
  {
    return (!(db->isAutoCommit()) && !isFinished);
  }

  //----------------------------------------------------------------------------

  bool Transaction::commit(int* errCodeOut)
  {
    string sql = "COMMIT";
    int err;
    db->execNonQuery(sql, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;

    isFinished = (err == SQLITE_DONE);
    return isFinished;
  }

  //----------------------------------------------------------------------------

  bool Transaction::rollback(int* errCodeOut)
  {
    string sql = "ROLLBACK";
    int err;
    db->execNonQuery(sql, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;

    isFinished = (err == SQLITE_DONE);
    return isFinished;
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
    if (dtorAct == TRANSACTION_DESTRUCTOR_ACTION::COMMIT)
    {
      if (savepointName.empty())
      {
        sql = "COMMIT";
      } else {
        sql = "RELEASE SAVEPOINT " + savepointName;
      }
    }
    if (dtorAct == TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK)
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

  Transaction::Transaction(SqliteDatabase* _db, TRANSACTION_TYPE tt, TRANSACTION_DESTRUCTOR_ACTION _dtorAct, int* errCodeOut)
    :db(_db), dtorAct(_dtorAct), isFinished(false)
  {
    if (db == nullptr)
    {
      if (errCodeOut != nullptr) *errCodeOut = SQLITE_ERROR;
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
      savepointName = to_string(rand() % 100000);
      savepointName += "_" + to_string(time(nullptr));

      // create the savepoint
      sql = "SAVEPOINT " + savepointName;
    } else {
      // autocommit is set to yes, so no other
      // transaction is already in progress.
      // now let's create such a transaction
      sql = "BEGIN ";
      switch (tt)
      {
      case TRANSACTION_TYPE::DEFERRED:
        sql += "DEFERRED";
        break;

      case TRANSACTION_TYPE::EXCLUSIVE:
        sql += "EXCLUSIVE";
        break;

      default:
        sql += "IMMEDIATE";
      }
      sql += " TRANSACTION";
    }

    // try to acquire the database lock
    int err;
    db->execNonQuery(sql, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;

    // raise an exception on error, e. g. if the database is locked by
    // another transaction
    if (err != SQLITE_DONE)
    {
      throw std::runtime_error("Couldn't initiate transaction in Transaction ctor");
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
