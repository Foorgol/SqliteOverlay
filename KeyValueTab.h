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

#ifndef SQLITE_OVERLAY_KEYVALUETAB_H
#define	SQLITE_OVERLAY_KEYVALUETAB_H

#include <memory>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"

namespace SqliteOverlay
{

  class KeyValueTab
  {
  public:
    // getter for a key-value-table
    static unique_ptr<KeyValueTab> getTab(SqliteDatabase* _db, const string& _tabName, bool createNewIfMissing=true, int* errCodeOut=nullptr);

    // setting of values
    void set(const string& key, const string& val, int* errCodeOut) const;
    void set(const string& key, int val, int* errCodeOut) const;
    void set(const string& key, double val, int* errCodeOut) const;

    // getters, type 1 (throws exception if key is not existing or empty)
    string operator[](const string& key) const;
    int getInt(const string& key) const;
    double getDouble(const string& key) const;

    // getters, type 2 (not throwing)
    unique_ptr<ScalarQueryResult<int>> getInt2(const string& key) const;
    unique_ptr<ScalarQueryResult<double>> getDouble2(const string& key) const;
    unique_ptr<ScalarQueryResult<string>> getString2(const string& key) const;

    // boolean queries
    bool hasKey(const string& key) const;

  private:
    KeyValueTab (SqliteDatabase* _db, const string& _tabName);
    SqliteDatabase* db;
    string tabName;
    DbTab* tab;
    upSqlStatement prepInsertUpdateStatementForKey(const string& key, int* errCodeOut) const;
    string valSelectStatement;

    static constexpr char KEY_COL_NAME[] = "K";
    static constexpr char VAL_COL_NAME[] = "V";
    static constexpr int MAX_KEY_LEN = 100;
  };
  
}
#endif	/* KEYVALUETAB_H */

