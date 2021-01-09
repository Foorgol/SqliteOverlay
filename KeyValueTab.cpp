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

#include <sys/types.h>                                  // for time_t
#include <algorithm>                                    // for max
#include <cstdint>                                      // for int64_t
#include <iosfwd>                                       // for std
#include <stdexcept>                                    // for invalid_argument
#include <utility>                                      // for move

#include <Sloppy/ConfigFileParser/ConstraintChecker.h>  // for checkConstraint
#include <Sloppy/DateTime/DateAndTime.h>                // for WallClockTime...
#include <Sloppy/String.h>                              // for estring
#include <Sloppy/json.hpp>                              // for json

#include "SqliteDatabase.h"                             // for SqliteDatabase
#include "SqliteExceptions.h"                           // for NullValueExce...

#include "KeyValueTab.h"

using namespace std;

namespace SqliteOverlay
{
  constexpr char KeyValueTab::KEY_COL_NAME[];
  constexpr char KeyValueTab::VAL_COL_NAME[];
    
    
  //----------------------------------------------------------------------------

  KeyValueTab::KeyValueTab(const SqliteDatabase& _db, const string& _tabName)
    :db(_db), tabName(_tabName), tab(DbTab{db, tabName, true})
  {
    // make sure that the table has the columns for keys and values
    if (!(tab.hasColumn(KEY_COL_NAME)))
    {
      throw std::invalid_argument("KeyValueTab ctor: table " + tabName + " has no valid key column");
    }
    if (!(tab.hasColumn(VAL_COL_NAME)))
    {
      throw std::invalid_argument("KeyValueTab ctor: table " + tabName + " has no valid value column");
    }

  }

  //----------------------------------------------------------------------------

  string KeyValueTab::operator [](const string& key)
  {
    string result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  int KeyValueTab::getInt(const string& key)
  {
    int result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  int64_t KeyValueTab::getLong(const string& key)
  {
    int64_t result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  double KeyValueTab::getDouble(const string& key)
  {
    double result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  bool KeyValueTab::getBool(const string& key)
  {
    bool result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  Sloppy::DateTime::WallClockTimepoint_secs KeyValueTab::getUTCTimestamp(const string& key)
  {
    time_t raw;
    get(key, raw);

    return Sloppy::DateTime::WallClockTimepoint_secs{raw};
  }

  //----------------------------------------------------------------------------

  nlohmann::json KeyValueTab::getJson(const string& key)
  {
    nlohmann::json result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  optional<string> KeyValueTab::getString2(const string& key)
  {
    optional<string> result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  optional<int> KeyValueTab::getInt2(const string& key)
  {
    optional<int> result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  optional<int64_t> KeyValueTab::getInt64_2(const string& key)
  {
    optional<int64_t> result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  optional<double> KeyValueTab::getDouble2(const string& key)
  {
    optional<double> result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  optional<bool> KeyValueTab::getBool2(const string& key)
  {
    optional<bool> result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  optional<Sloppy::DateTime::WallClockTimepoint_secs> KeyValueTab::getUTCTimestamp2(const string& key)
  {
    auto raw = getInt64_2(key);
    if (!raw) return std::nullopt;
    return Sloppy::DateTime::WallClockTimepoint_secs{static_cast<time_t>(raw.value())};
  }

  //----------------------------------------------------------------------------

  optional<nlohmann::json> KeyValueTab::getJson2(const string& key)
  {
    optional<nlohmann::json> result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  bool KeyValueTab::hasKey(const string& key) const
  {
    if (key.empty()) return false;

    return (tab.getMatchCountForColumnValue(KEY_COL_NAME, key) > 0);
  }

  //----------------------------------------------------------------------------

  bool KeyValueTab::checkConstraint(const string& keyName, Sloppy::ValueConstraint c, string* errMsg)
  {
    if (keyName.empty())
    {
      throw std::invalid_argument("KeyValueTab constraint check: received empty key name!");
    }

    // prepare a helper functions that puts the keyname
    // into the first two part of an error message
    auto prepErrMsg = [&keyName]() {
      Sloppy::estring msg{"The key %1 "};
      msg.arg(keyName);
      return msg;
    };

    // first basic check: does the key-value pair exist at all?
    if (!hasKey(keyName))
    {
      if (errMsg != nullptr)
      {
        auto e = prepErrMsg();
        e += "does not exist!";
        *errMsg = e;
      }

      return false;
    }

    // if the key exists but the optional value is empty
    // then somehow we stumbled across a NULL entry.
    //
    // such an entry must have been created by something else
    // than a KeyValueTab instance because we don't work with "NULL" here
    optional<Sloppy::estring> v = getString2(keyName);
    if (!(v.has_value()))
    {
      if (errMsg != nullptr)
      {
        auto e = prepErrMsg();
        e += "contains a NULL value!";
        *errMsg = e;
      }

      return false;
    }

    // enough to satisfy "Exists"
    if (c == Sloppy::ValueConstraint::Exist)
    {
      return true;
    }

    // leave all further checks to the constraint checker
    if (!Sloppy::checkConstraint(v, c, errMsg))
    {
      if (errMsg != nullptr)
      {
        auto e = prepErrMsg();
        *errMsg = e + *errMsg;
      }

      return false;
    }

    return true;

  }

  //----------------------------------------------------------------------------

  void KeyValueTab::remove(const string& key)
  {
    std::string sql{"DELETE FROM " + tabName + " WHERE " + KEY_COL_NAME + "=?"};
    auto stmt = db.get().prepStatement(sql);
    stmt.bind(1, key);
    stmt.step();
  }

  //----------------------------------------------------------------------------

  vector<string> KeyValueTab::allKeys() const
  {
    vector<string> result;

    std::string sql{"SELECT " + std::string{KEY_COL_NAME} + " FROM " + tabName};
    auto stmt = db.get().prepStatement(sql);

    for (stmt.step() ; stmt.hasData() ; stmt.step())
    {
      result.push_back(stmt.getString(0));
    }

    return result;
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::releaseDatabase()
  {
    valSelectStatement.reset();
    valUpdateStatement.reset();
    valInsertStatement.reset();
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::enableStatementCache(bool useCaching)
  {
    cacheStatements = useCaching;
    if (!useCaching) releaseDatabase();
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::prepValueSelectStmt()
  {
    // prepare the sql-text for looking up values once and for all
    static const string sql{"SELECT " + string(VAL_COL_NAME) + " FROM " + tabName +
                            " WHERE " + string(KEY_COL_NAME) + " = ?"};

    // do we need to create a new statement at all?
    if (valSelectStatement)
    {
      valSelectStatement->reset(true);
      return;
    }

    // prepare an empty dummy statement on the heap
    valSelectStatement = std::make_unique<SqlStatement>();

    // create the "true" select statement in a tmp variable
    // on the stack
    auto tmpStatement = db.get().prepStatement(sql);

    // move "from stack to heap" ;)
    //
    // remember: statements are resource owners that
    // do not support copying
    *valSelectStatement = std::move(tmpStatement);
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::prepValueUpdateStmt()
  {
    // prepare the sql-text for updating values once and for all
    static const string sql{
      "UPDATE " + tabName + " SET " + std::string{VAL_COL_NAME} + "=? " +
      "WHERE " + std::string{KEY_COL_NAME} + "=?"
    };

    // do we need to create a new statement at all?
    if (valUpdateStatement)
    {
      valUpdateStatement->reset(true);
      return;
    }

    // prepare an empty dummy statement on the heap
    valUpdateStatement = std::make_unique<SqlStatement>();

    // create the "true" select statement in a tmp variable
    // on the stack
    auto tmpStatement = db.get().prepStatement(sql);

    // move "from stack to heap" ;)
    //
    // remember: statements are resource owners that
    // do not support copying
    *valUpdateStatement = std::move(tmpStatement);
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::prepValueInsertStmt()
  {
    // prepare the sql-text for inserting values once and for all
    static const string sql{
      "INSERT INTO " + tabName + " (" + string{KEY_COL_NAME} + "," +
      string{VAL_COL_NAME} + ") VALUES (?,?)"
    };

    // do we need to create a new statement at all?
    if (valInsertStatement)
    {
      valInsertStatement->reset(true);
      return;
    }

    // prepare an empty dummy statement on the heap
    valInsertStatement = std::make_unique<SqlStatement>();

    // create the "true" select statement in a tmp variable
    // on the stack
    auto tmpStatement = db.get().prepStatement(sql);

    // move "from stack to heap" ;)
    //
    // remember: statements are resource owners that
    // do not support copying
    *valInsertStatement = std::move(tmpStatement);
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
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
    
//----------------------------------------------------------------------------
    
}
