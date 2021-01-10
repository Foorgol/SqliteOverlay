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
  const std::string KeyValueTab::KeyColName{"K"};
  const std::string KeyValueTab::ValColName{"V"};

    
  //----------------------------------------------------------------------------

  KeyValueTab::KeyValueTab(SqliteDatabase* _db, const string& _tabName)
    :db(_db), tabName(_tabName), tab(DbTab{*db, tabName, true})
    , sqlSelect{"SELECT " + ValColName + " FROM " + tabName + " WHERE " + KeyColName + " = ?"}
    , sqlUpdate{"UPDATE " + tabName + " SET " + ValColName + "=? " + "WHERE " + KeyColName + "=?"}
    , sqlInsert{"INSERT INTO " + tabName + " (" + ValColName + "," + KeyColName + ") VALUES (?,?)"}
  {
    // make sure that the table has the columns for keys and values
    if (!(tab.hasColumn(KeyColName)))
    {
      throw std::invalid_argument("KeyValueTab ctor: table " + tabName + " has no valid key column");
    }
    if (!(tab.hasColumn(ValColName)))
    {
      throw std::invalid_argument("KeyValueTab ctor: table " + tabName + " has no valid value column");
    }
  }

  //----------------------------------------------------------------------------

  bool KeyValueTab::hasKey(const string& key) const
  {
    if (key.empty()) return false;

    return (tab.getMatchCountForColumnValue(KeyColName, key) > 0);
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
    optional<Sloppy::estring> v = get2<std::string>(keyName);
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
    const std::string sql{"DELETE FROM " + tabName + " WHERE " + KeyColName + "=?"};
    auto stmt = db->prepStatement(sql);
    stmt.bind(1, key);
    stmt.step();
  }

  //----------------------------------------------------------------------------

  vector<string> KeyValueTab::allKeys() const
  {
    vector<string> result;

    const std::string sql{"SELECT " + std::string{KeyColName} + " FROM " + tabName};
    auto stmt = db->prepStatement(sql);

    for (stmt.step() ; stmt.hasData() ; stmt.step())
    {
      result.push_back(stmt.get<std::string>(0));
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
