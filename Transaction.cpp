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

  unique_ptr<Transaction> Transaction::startNew(SqliteDatabase* _db, TRANSACTION_TYPE tt, int* errCodeOut)
  {
    Transaction* tmpPtr;
    try
    {
      tmpPtr = new Transaction(_db, tt, errCodeOut);
    } catch (exception e) {
      return nullptr;
    }

    return unique_ptr<Transaction>(tmpPtr);
  }

  //----------------------------------------------------------------------------

  bool Transaction::isActive() const
  {
    return (db->isAutoCommit() == 0);
  }

  //----------------------------------------------------------------------------

  bool Transaction::commit(int* errCodeOut)
  {
    string sql = "COMMIT";
    int err;
    db->execNonQuery(sql, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;

    return (err == SQLITE_OK);
  }

  //----------------------------------------------------------------------------

  bool Transaction::rollback(int* errCodeOut)
  {
    string sql = "ROLLBACK";
    int err;
    db->execNonQuery(sql, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;

    return (err == SQLITE_OK);
  }

//----------------------------------------------------------------------------

  Transaction::Transaction(SqliteDatabase* _db, TRANSACTION_TYPE tt, int* errCodeOut)
    :db(_db)
  {
    if (db == nullptr)
    {
      if (errCodeOut != nullptr) *errCodeOut = SQLITE_ERROR;
      throw invalid_argument("Received NULL handle for database in Transaction ctor");
    }

    // try to acquire the database lock
    string sql = "BEGIN ";
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
    int err;
    db->execNonQuery(sql, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;

    // raise an exception on error, e. g. if the database is locked by
    // another transaction
    if (err != SQLITE_OK)
    {
      throw runtime_error("Couldn't initiate transaction in Transaction ctor");
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
