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
#include "SqlStatement.h"

namespace SqliteOverlay
{

  class KeyValueTab
  {
  public:
    KeyValueTab (const SqliteDatabase& _db, const string& _tabName);

    // setting of values
    template<typename T>
    void set(const string& key, const T& val)
    {
      if (hasKey(key))
      {
        valUpdateStatement.bind(1, val);
        valUpdateStatement.bind(2, key);
        valUpdateStatement.step();
        valUpdateStatement.reset(true);
      } else {
        valInsertStatement.bind(1, key);
        valInsertStatement.bind(2, key);
        valInsertStatement.step();
        valInsertStatement.reset(true);
      }
    }

    template<typename T>
    void get(const string& key, T& outVal)
    {
      valSelectStatement.bind(1, key);
      valSelectStatement.step();
      valSelectStatement.get(0, outVal);
      valSelectStatement.reset(true);
    }

    string operator[](const string& key);
    int getInt(const string& key);
    long getLong(const string& key);
    double getDouble(const string& key);
    UTCTimestamp getUTCTimestamp(const string& key);

    // getters, type 2 (not throwing)
    optional<string> getString2(const string& key);
    optional<int> getInt2(const string& key);
    optional<long> getLong2(const string& key);
    optional<double> getDouble2(const string& key);
    optional<UTCTimestamp> getUTCTimestamp2(const string& key);

    // boolean queries
    bool hasKey(const string& key) const;

  private:
    reference_wrapper<const SqliteDatabase> db;
    string tabName;
    const DbTab tab;
    SqlStatement valSelectStatement;
    SqlStatement valUpdateStatement;
    SqlStatement valInsertStatement;

    static constexpr char KEY_COL_NAME[] = "K";
    static constexpr char VAL_COL_NAME[] = "V";
    static constexpr int MAX_KEY_LEN = 100;
  };
  
}
#endif	/* KEYVALUETAB_H */

