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

#include <climits>
#include <cmath>
#include <regex>

#include <Sloppy/ConfigFileParser/ConstraintChecker.h>
#include <Sloppy/json.hpp>

#include "KeyValueTab.h"
#include "TableCreator.h"

namespace bfs = boost::filesystem;

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

    // prepare the sql-text for looking up values once and for all
    string sql = "SELECT " + string(VAL_COL_NAME) + " FROM " + tabName + " WHERE ";
    sql += string(KEY_COL_NAME) + " = ?";
    valSelectStatement = db.get().prepStatement(sql);

    // prepare the sql-text for updating values once and for all
    Sloppy::estring cmd{"UPDATE %1 SET %2=? WHERE %3=?"};
    cmd.arg(tabName);
    cmd.arg(VAL_COL_NAME);
    cmd.arg(KEY_COL_NAME);
    valUpdateStatement = db.get().prepStatement(cmd);

    // prepare the sql-text for inserting values once and for all
    cmd = string{"INSERT INTO %1 (%2,%3) VALUES (?,?)"};
    cmd.arg(tabName);
    cmd.arg(KEY_COL_NAME);
    cmd.arg(VAL_COL_NAME);
    valInsertStatement = db.get().prepStatement(cmd);
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

  long KeyValueTab::getLong(const string& key)
  {
    long result;
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

  double KeyValueTab::getBool(const string& key)
  {
    bool result;
    get(key, result);
    return result;
  }

  //----------------------------------------------------------------------------

  UTCTimestamp KeyValueTab::getUTCTimestamp(const string& key)
  {
    UTCTimestamp result;
    get(key, result);
    return result;
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

  optional<long> KeyValueTab::getLong2(const string& key)
  {
    optional<long> result;
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

  optional<UTCTimestamp> KeyValueTab::getUTCTimestamp2(const string& key)
  {
    optional<UTCTimestamp> result;
    get(key, result);
    return result;
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
    Sloppy::estring sql = "DELETE FROM %1 WHERE %2=?";
    sql.arg(tabName);
    sql.arg(KEY_COL_NAME);
    auto stmt = db.get().prepStatement(sql);
    stmt.bind(1, key);
    stmt.step();
  }

  //----------------------------------------------------------------------------

  vector<string> KeyValueTab::allKeys() const
  {
    vector<string> result;

    Sloppy::estring sql = "SELECT %1 FROM %2";
    sql.arg(KEY_COL_NAME);
    sql.arg(tabName);
    auto stmt = db.get().prepStatement(sql);

    for (stmt.step() ; stmt.hasData() ; stmt.step())
    {
      result.push_back(stmt.getString(0));
    }

    return result;
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
