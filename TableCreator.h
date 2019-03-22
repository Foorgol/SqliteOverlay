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

#ifndef SQLITE_OVERLAY_TABLECREATOR_H
#define	SQLITE_OVERLAY_TABLECREATOR_H

#include <memory>
#include <initializer_list>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"

namespace SqliteOverlay
{
  class TableCreator
  {
  public:
    // ctor
    TableCreator(SqliteDatabase* _db);

    // adding columns
    template<typename T>
    void addCol(const string& colName, ColumnDataType t, const T& defaultValue,
                ConflictClause uniqueConflictClause=ConflictClause::NotUsed,
                ConflictClause notNullConflictClause=ConflictClause::NotUsed)
    {
      if (colName.empty())
      {
        throw std::invalid_argument("TableCreator: addCol called with empty column name!");
      }

      string colDef = buildColumnConstraint(uniqueConflictClause, notNullConflictClause, defaultValue);

    }
    void addCol(const string& colDef);

    // add foreign keys
    void addForeignKey(const string& keyName, const string& referedTable,
                               CONSISTENCY_ACTION onDelete=CONSISTENCY_ACTION::__NOT_SET,
                               CONSISTENCY_ACTION onUpdate=CONSISTENCY_ACTION::__NOT_SET,
                       bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET);

    // add a unique combination of two or more
    // column values
    bool addUniqueColumnCombination(initializer_list<string> colNames, CONFLICT_CLAUSE notUniqueConflictClause);

    // reset all internal data
    void reset();

    // creation of the final SQL-statement without
    // actually executing any database commands or
    // changing / resetting the internal state
    string getSqlStatement(const string& tabName) const;

    // the actual table creation
    DbTab* createTableAndResetCreator(const string& tabName, int* errCodeOut=nullptr);

  private:
    SqliteDatabase* db;
    Sloppy::StringList constraintCache;
    Sloppy::StringList colDefs;
    Sloppy::StringList defaultValues;
  };
  
}
#endif	/* TABLECREATOR_H */

