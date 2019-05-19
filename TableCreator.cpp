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

#include <Sloppy/Utils.h>

#include "TableCreator.h"

namespace SqliteOverlay
{

  void TableCreator::addCol(const string& colName, ColumnDataType t, ConflictClause uniqueConflictClause, ConflictClause notNullConflictClause)
  {
    if (colName.empty())
    {
      throw std::invalid_argument("TableCreator: addCol called with empty column name!");
    }

    string colDef = buildColumnConstraint(uniqueConflictClause, notNullConflictClause);

    colDef = colName + " " + to_string(t) + " " + colDef;

    colDefs.push_back(colDef);
  }

  //----------------------------------------------------------------------------

  void TableCreator::addCol(const string& colDef)
  {
    colDefs.push_back(colDef);
  }

  //----------------------------------------------------------------------------

<<<<<<< HEAD
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
=======
  void TableCreator::addForeignKey(const string& keyName, const string& referedTable, ConsistencyAction onDelete, ConsistencyAction onUpdate, ConflictClause uniqueConflictClause, ConflictClause notNullConflictClause, const string& referedColumn)
>>>>>>> dev
  {
    string ref = "FOREIGN KEY (" + keyName + ") ";
    ref += buildForeignKeyClause(referedTable, onDelete, onUpdate, referedColumn);

    constraintCache.push_back(ref);
<<<<<<< HEAD
    addInt(keyName, isUnique, uniqueConflictClause, notNull, notNullConflictClause);
=======
    addCol(keyName, ColumnDataType::Integer, uniqueConflictClause, notNullConflictClause);
>>>>>>> dev
  }

  //----------------------------------------------------------------------------

  bool TableCreator::addUniqueColumnCombination(initializer_list<string> colNames, ConflictClause notUniqueConflictClause)
  {
    if (colNames.size() < 2) return false;

    Sloppy::estring constraint = "UNIQUE(%1) ON CONFLICT %2";
    constraint.arg(Sloppy::commaSepStringFromValues(colNames));
    constraint.arg(to_string(notUniqueConflictClause));
    constraintCache.push_back(constraint);

    return true;
  }

  //----------------------------------------------------------------------------

  void TableCreator::reset()
  {
    colDefs.clear();
    constraintCache.clear();
  }

  //----------------------------------------------------------------------------

  string TableCreator::getSqlStatement(const string& tabName) const
  {
    // assemble a "create table" statement from the column definitions and the
    // stored foreign key clauses
    string sql = "CREATE TABLE IF NOT EXISTS " + tabName + " (";

    // we always define a column "id" that becomes an alias for "rowid"
    // but can be referenced by a foreign key constraint
    //
    // for this to work, the declared type has to be exactly "INTEGER PRIMARY KEY"
    //
    sql += "id INTEGER PRIMARY KEY";  //

    if (!colDefs.empty())
    {
      sql += ", " + Sloppy::commaSepStringFromValues(colDefs);
    }

    if (!constraintCache.empty()) {
      sql += ", " + Sloppy::commaSepStringFromValues(constraintCache);
    }

    sql += ")";

    return sql;
  }

  //----------------------------------------------------------------------------

  DbTab TableCreator::createTableAndResetCreator(const SqliteDatabase& db, const string& tabName)
  {
    // get the SQL statement
    string sql = getSqlStatement(tabName);
    db.execNonQuery(sql);

    reset();

    return DbTab(db, tabName, false);
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
