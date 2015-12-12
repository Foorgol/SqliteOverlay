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

#include "KeyValueTab.h"
#include "HelperFunc.h"
#include "TableCreator.h"

namespace SqliteOverlay
{
  constexpr char KeyValueTab::KEY_COL_NAME[];
  constexpr char KeyValueTab::VAL_COL_NAME[];
    
    
//----------------------------------------------------------------------------

  unique_ptr<KeyValueTab> KeyValueTab::getTab(SqliteDatabase* _db, const string& _tabName, bool createNewIfMissing, int* errCodeOut)
  {
    if (_db == nullptr) return nullptr;
    if (_tabName.empty()) return nullptr;

    // check if the table exists
    if (!(_db->hasTable(_tabName)))
    {
      if (!createNewIfMissing) return nullptr;

      // create a new table
      TableCreator tc{_db};
      tc.addVarchar(KEY_COL_NAME, MAX_KEY_LEN, true, CONFLICT_CLAUSE::ROLLBACK, true, CONFLICT_CLAUSE::ROLLBACK);
      tc.addText(VAL_COL_NAME);
      tc.createTableAndResetCreator(_tabName, errCodeOut);
    }

    // return a new instance of KeyValueTab
    return unique_ptr<KeyValueTab>(new KeyValueTab(_db, _tabName));
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::set(const string& key, const string& val, int* errCodeOut) const
  {
    auto stmt = prepInsertUpdateStatementForKey(key, errCodeOut);
    if (stmt != nullptr)
    {
      stmt->bindString(1, val);
      db->execNonQuery(stmt, errCodeOut);
    }
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::set(const string& key, int val, int* errCodeOut) const
  {
    auto stmt = prepInsertUpdateStatementForKey(key, errCodeOut);
    if (stmt != nullptr)
    {
      stmt->bindInt(1, val);
      db->execNonQuery(stmt, errCodeOut);
    }
  }

  //----------------------------------------------------------------------------

  void KeyValueTab::set(const string& key, double val, int* errCodeOut) const
  {
    auto stmt = prepInsertUpdateStatementForKey(key, errCodeOut);
    if (stmt != nullptr)
    {
      stmt->bindDouble(1, val);
      db->execNonQuery(stmt, errCodeOut);
    }
  }

  //----------------------------------------------------------------------------

  KeyValueTab::KeyValueTab(SqliteDatabase* _db, const string& _tabName)
    :db(_db), tabName(_tabName), tab(db->getTab(tabName))
  {
    // does the requeted tab exist?
    if (tab == nullptr)
    {
      throw std::invalid_argument("KeyValueTab ctor: table " + tabName + " does not exist.");
    }
    // make sure that the table has the columns for keys and values
    if (!(tab->hasColumn(KEY_COL_NAME)))
    {
      throw std::invalid_argument("KeyValueTab ctor: table " + tabName + " has no valid key column");
    }
    if (!(tab->hasColumn(VAL_COL_NAME)))
    {
      throw std::invalid_argument("KeyValueTab ctor: table " + tabName + " has no valid value column");
    }

    // prepare the sql-text for looking up values once and for all
    valSelectStatement = "SELECT " + string(VAL_COL_NAME) + " FROM " + tabName + " WHERE ";
    valSelectStatement += string(KEY_COL_NAME) + " = ?";

  }

  //----------------------------------------------------------------------------

  string KeyValueTab::operator [](const string& key) const
  {
    auto result = getString2(key);
    if (result == nullptr) return string();  // key doesn't exist
    if (result->isNull()) return string();  // value is empty

    return result->get();
  }

  //----------------------------------------------------------------------------

  int KeyValueTab::getInt(const string& key) const
  {
    auto result = getInt2(key);
    if (result == nullptr) return INT_MIN;  // key doesn't exist
    if (result->isNull()) return INT_MIN;  // value is empty

    return result->get();
  }

  //----------------------------------------------------------------------------

  double KeyValueTab::getDouble(const string& key) const
  {
    auto result = getDouble2(key);
    if (result == nullptr) return NAN;  // key doesn't exist
    if (result->isNull()) return NAN;  // value is empty

    return result->get();
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<int> > KeyValueTab::getInt2(const string& key) const
  {
    auto qry = db->prepStatement(valSelectStatement, nullptr);
    qry->bindString(1, key);

    return db->execScalarQueryInt(qry, nullptr);
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<double> > KeyValueTab::getDouble2(const string& key) const
  {
    auto qry = db->prepStatement(valSelectStatement, nullptr);
    qry->bindString(1, key);

    return db->execScalarQueryDouble(qry, nullptr);
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<string> > KeyValueTab::getString2(const string& key) const
  {
    auto qry = db->prepStatement(valSelectStatement, nullptr);
    qry->bindString(1, key);

    return db->execScalarQueryString(qry, nullptr);
  }

  //----------------------------------------------------------------------------

  upSqlStatement KeyValueTab::prepInsertUpdateStatementForKey(const string& key, int* errCodeOut) const
  {
    if (key.empty()) return nullptr;
    if (key.length() > MAX_KEY_LEN) return nullptr;

    upSqlStatement result;
    if (hasKey(key))
    {
      string sql;
      sql = "UPDATE " + tabName + " SET " + string(VAL_COL_NAME) + " = ?1 ";
      sql += "WHERE " + string(KEY_COL_NAME) + "=?2";
      result = db->prepStatement(sql, errCodeOut);
      result->bindString(2, key);
    } else {
      string sql;
      sql = "INSERT INTO " + tabName + " (" + string(KEY_COL_NAME) + ", " + string(VAL_COL_NAME) + ") ";
      sql += "VALUES (?2, ?1)";
      result = db->prepStatement(sql, errCodeOut);
      result->bindString(2, key);
    }

    // the "value" is the only remaining parameter in the prepared statement
    // and can thus be accessed with index 1 from any subsequent, value-type
    // specific bind function
    return result;
  }
    
//----------------------------------------------------------------------------
    
  bool KeyValueTab::hasKey(const string& key) const
  {
    if (key.empty()) return false;

    return (tab->getMatchCountForColumnValue(KEY_COL_NAME, key) > 0);
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
