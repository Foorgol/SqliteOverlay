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

#include <Sloppy/libSloppy.h>

#include "TableCreator.h"

namespace SqliteOverlay
{

  TableCreator::TableCreator(SqliteDatabase* _db)
    :db(_db)
  {
    if (db == nullptr)
    {
      throw invalid_argument("Received null handle for database in ctor of TableCreator");
    }

  }

  //----------------------------------------------------------------------------

  void TableCreator::addVarchar(const string& colName, int len, bool isUnique, CONFLICT_CLAUSE uniqueConflictClause, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue)
  {
    string tName = "VARCHAR(" + to_string(len) + ")";

    addCol(colName, tName, isUnique, uniqueConflictClause, notNull, notNullConflictClause, hasDefault, defaultValue);
  }

  //----------------------------------------------------------------------------

  void TableCreator::addInt(const string& colName, bool isUnique, CONFLICT_CLAUSE uniqueConflictClause, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue)
  {
    addCol(colName, "INTEGER", isUnique, uniqueConflictClause, notNull, notNullConflictClause, hasDefault, defaultValue);
  }

  //----------------------------------------------------------------------------

  void TableCreator::addText(const string& colName, bool isUnique, CONFLICT_CLAUSE uniqueConflictClause, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue)
  {
    addCol(colName, "TEXT", isUnique, uniqueConflictClause, notNull, notNullConflictClause, hasDefault, defaultValue);
  }

  //----------------------------------------------------------------------------

  void TableCreator::addCol(const string& colDef)
  {
    colDefs.push_back(colDef);
  }

  //----------------------------------------------------------------------------

  void TableCreator::addCol(const string& colName, const string& colTypeName, bool isUnique, CONFLICT_CLAUSE uniqueConflictClause, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue)
  {
    if (colName.empty()) return;
    if (colTypeName.empty()) return;

    string colDef = colName + " " + colTypeName;

    if (isUnique)
    {
      colDef += " UNIQUE";
      if (uniqueConflictClause != CONFLICT_CLAUSE::__NOT_SET)
      {
        colDef += " ON CONFLICT " + conflictClause2String(uniqueConflictClause);
      }
    }

    if (notNull)
    {
      colDef += " NOT NULL";
      if (notNullConflictClause != CONFLICT_CLAUSE::__NOT_SET)
      {
        colDef += " ON CONFLICT " + conflictClause2String(notNullConflictClause);
      }
    }

    if (hasDefault)
    {
      colDef += " DEFAULT " + defaultValue;
    }

    addCol(colDef);
  }

  //----------------------------------------------------------------------------

  void TableCreator::addForeignKey(const string& keyName, const string& referedTable, CONSISTENCY_ACTION onDelete, CONSISTENCY_ACTION onUpdate)
  {
    // a little helper to translate a CONSISTENCY_ACTION value into a string
    auto ca2string = [](CONSISTENCY_ACTION ca) -> string {
      switch (ca)
      {
        case CONSISTENCY_ACTION::NO_ACTION:
          return "NO ACTION";
        case CONSISTENCY_ACTION::SET_NULL:
          return "SET NULL";
        case CONSISTENCY_ACTION::SET_DEFAULT:
          return "SET DEFAULT";
        case CONSISTENCY_ACTION::CASCADE:
          return "CASCADE";
        case CONSISTENCY_ACTION::RESTRICT:
          return "RESTRICT";
      }
      return "";
    };

    string ref = "FOREIGN KEY (" + keyName + ") REFERENCES " + referedTable + "(id)";

    if (onDelete != CONSISTENCY_ACTION::__NOT_SET)
    {
      ref += " ON DELETE " + ca2string(onDelete);
    }

    if (onUpdate != CONSISTENCY_ACTION::__NOT_SET)
    {
      ref += " ON UPDATE " + ca2string(onUpdate);
    }

    foreignKeyCreationCache.push_back(ref);
    addInt(keyName);
  }

  //----------------------------------------------------------------------------

  void TableCreator::reset()
  {
    colDefs.clear();
    foreignKeyCreationCache.clear();
  }

  //----------------------------------------------------------------------------

  DbTab* TableCreator::createTableAndResetCreator(const string& tabName, int* errCodeOut)
  {
    string sql = "CREATE TABLE IF NOT EXISTS " + tabName + " (";
    sql += "id INTEGER NOT NULL PRIMARY KEY ";

    sql += "AUTOINCREMENT";

    sql += ", " + Sloppy::commaSepStringFromStringList(colDefs);

    if (foreignKeyCreationCache.size() != 0) {
      sql += ", " + Sloppy::commaSepStringFromStringList(foreignKeyCreationCache);
      foreignKeyCreationCache.clear();
    }

    sql += ");";

    db->execNonQuery(sql, errCodeOut);
    reset();

    return db->getTab(tabName);
  }

  //----------------------------------------------------------------------------

  string TableCreator::conflictClause2String(CONFLICT_CLAUSE cc)
  {
    switch (cc)
    {
    case CONFLICT_CLAUSE::ABORT:
      return "ABORT";

    case CONFLICT_CLAUSE::FAIL:
      return "FAIL";

    case CONFLICT_CLAUSE::IGNORE:
      return "IGNORE";

    case CONFLICT_CLAUSE::REPLACE:
      return "REPLACE";

    case CONFLICT_CLAUSE::ROLLBACK:
      return "ROLLBACK";
    }

    return "";  // includes __NOT_SET
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
    
}
