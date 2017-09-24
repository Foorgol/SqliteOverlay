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

#include <boost/date_time/local_time/local_time.hpp>

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

  void TableCreator::addCol(const string& colName, const string& colTypeName, bool isUnique, CONFLICT_CLAUSE uniqueConflictClause,
                            bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue)
  {
    if (colName.empty()) return;
    if (colTypeName.empty()) return;

    string colDef = colName + " " + colTypeName + " ";

    colDef += buildColumnConstraint(isUnique, uniqueConflictClause, notNull, notNullConflictClause,
                                    hasDefault, defaultValue);

    addCol(colDef);
  }

  //----------------------------------------------------------------------------

  void TableCreator::addForeignKey(const string& keyName, const string& referedTable, CONSISTENCY_ACTION onDelete, CONSISTENCY_ACTION onUpdate,
                                   bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool isUnique, CONFLICT_CLAUSE uniqueConflictClause)
  {
    string ref = "FOREIGN KEY (" + keyName + ") ";
    ref += buildForeignKeyClause(referedTable, onDelete, onUpdate);

    constraintCache.push_back(ref);
    addInt(keyName, isUnique, uniqueConflictClause, notNull, notNullConflictClause);
  }

  //----------------------------------------------------------------------------

  bool TableCreator::addUniqueColumnCombination(initializer_list<string> colNames, CONFLICT_CLAUSE notUniqueConflictClause)
  {
    if (colNames.size() < 2) return false;

    string constraint = "UNIQUE(%1) ON CONFLICT %2";
    Sloppy::strArg(constraint, Sloppy::commaSepStringFromStringList(colNames));
    Sloppy::strArg(constraint, conflictClause2String(notUniqueConflictClause));
    constraintCache.push_back(constraint);

    return true;
  }

  //----------------------------------------------------------------------------

  void TableCreator::reset()
  {
    colDefs.clear();
    constraintCache.clear();
    defaultValues.clear();
  }

  //----------------------------------------------------------------------------

  string TableCreator::getSqlStatement(const string& tabName) const
  {
    // assemble a "create table" statement from the column definitions and the
    // stored foreign key clauses
    string sql = "CREATE TABLE IF NOT EXISTS " + tabName + " (";
    sql += "id INTEGER NOT NULL PRIMARY KEY ";

    sql += "AUTOINCREMENT";

    sql += ", " + Sloppy::commaSepStringFromStringList(colDefs);

    if (constraintCache.size() != 0) {
      sql += ", " + Sloppy::commaSepStringFromStringList(constraintCache);
    }

    sql += ")";

    return sql;
  }

  //----------------------------------------------------------------------------

  DbTab* TableCreator::createTableAndResetCreator(const string& tabName, int* errCodeOut)
  {
    // get the SQL statement
    string sql = getSqlStatement(tabName);

    // execute the statement
    bool isOk = db->execNonQuery(sql, errCodeOut);
    if (!isOk) return nullptr;

    reset();

    return db->getTab(tabName);
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
