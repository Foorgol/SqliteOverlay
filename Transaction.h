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

#ifndef SQLITE_OVERLAY_TRANSACTION_H
#define	SQLITE_OVERLAY_TRANSACTION_H

#include <memory>

#include "SqliteDatabase.h"

namespace SqliteOverlay
{

  enum class TRANSACTION_TYPE
  {
    DEFERRED,
    IMMEDIATE,
    EXCLUSIVE
  };

  class Transaction
  {
  public:
    static unique_ptr<Transaction> startNew(SqliteDatabase* _db, TRANSACTION_TYPE tt=TRANSACTION_TYPE::IMMEDIATE, int* errCodeOut=nullptr);
    bool isActive() const;
    bool commit(int* errCodeOut=nullptr);
    bool rollback(int* errCodeOut=nullptr);

  private:
    Transaction(SqliteDatabase* _db, TRANSACTION_TYPE tt=TRANSACTION_TYPE::IMMEDIATE, int* errCodeOut=nullptr);
    SqliteDatabase* db;
  };
  
}
#endif	/* SQLITE_OVERLAY_TRANSACTION_H */

