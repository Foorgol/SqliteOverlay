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

#pragma once

#include <initializer_list>  // for initializer_list
#include <stdexcept>         // for invalid_argument
#include <string>            // for allocator, char_traits, string, operator+

#include <Sloppy/String.h>   // for StringList

#include "DbTab.h"           // for DbTab
#include "Defs.h"            // for ConflictClause, to_string, ColumnDataType
#include "SqliteDatabase.h"  // for SqliteDatabase (ptr only), buildColumnCo...

namespace SqliteOverlay
{
  /** \brief Convenience class for easy creation of new tables
   */
  class TableCreator
  {
  public:
    /** \brief Default ctor, does nothing
     */
    TableCreator() {}

    /** \brief Adds a column *without* a default value
     *
     * \throws std::invalid_argument if the column name is empty
     */
    void addCol(
        const std::string& colName,   ///< the new column's name
        ColumnDataType t,   ///< the "declared type" of the new column (determines its type affinity)
        ConflictClause uniqueConflictClause,   ///< enforcement of unique values; set to `NotUsed' if you don't need uniqueness
        ConflictClause notNullConflictClause   ///< enforcement of non-NULL values; set to `NotUsed' if you want to allow NULL
        );

    /** \brief Adds a column *with* a default value
     *
     * \throws std::invalid_argument if the column name is empty
     */
    template<typename T>
    void addCol(
        const std::string& colName,   ///< the new column's name
        ColumnDataType t,   ///< the "declared type" of the new column (determines its type affinity)
        ConflictClause uniqueConflictClause,   ///< enforcement of unique values; set to `NotUsed' if you don't need uniqueness
        ConflictClause notNullConflictClause,   ///< enforcement of non-NULL values; set to `NotUsed' if you want to allow NULL
        const T& defaultValue   ///< the default value for the column
        )
    {
      if (colName.empty())
      {
        throw std::invalid_argument("TableCreator: addCol called with empty column name!");
      }

      std::string colDef = buildColumnConstraint(uniqueConflictClause, notNullConflictClause, defaultValue);

      colDef = colName + " " + std::to_string(t) + " " + colDef;

      colDefs.push_back(colDef);
    }

    /** \brief Adds a colunm with a fully custom column definition
     */
    void addCol(
        const std::string& colDef   ///< the custom column definition
        );

    /** \brief Adds a colunm for a foreign key
     */
    void addForeignKey(
        const std::string& keyName,   ///< the name of the column that contains the foreign key
        const std::string& referedTable,   ///< the name of the table that should be referenced
        ConsistencyAction onDelete,   ///< the action that should be triggered on deletion of the target row(s)
        ConsistencyAction onUpdate,   ///< the action that should be triggered on updates of the target row(s)
        ConflictClause uniqueConflictClause,   ///< enforcement of unique key values; set to `NotUsed' if you don't need uniqueness
        ConflictClause notNullConflictClause,   ///< enforcement of non-NULL key values; set to `NotUsed' if you want to allow NULL
        const std::string& referedColumn = "id"   ///< the refered-to column's name; usually it's just "id"; "rowid" is not allowed by SQLite
        );

    /** \brief Adds a constraint that enforces a unique combination of two or more column values
     *
     * \returns `false` if just one column name was provided, `true` otherwise
     */
    bool addUniqueColumnCombination(
        std::initializer_list<std::string> colNames,   ///< a list of column names
        ConflictClause notUniqueConflictClause   ///< the action that should be taken if the requested constraint would be violated
        );

    /** \brief Erases all previously added definitions from the objects
     * and resets it back to its initial state
     */
    void reset();

    /** \brief Creates the final SQL-statement text for creating the table; the database itself is left untouched
     *
     * \returns a string with the `CREATE TABLE` command
     */
    std::string getSqlStatement(
        const std::string& tabName   ///< the name of the to-be-created table
        ) const;

    /** \brief Builds and executes the command for creating the table
     *
     * \returns a `DbTab` instance for the new table
     */
    DbTab createTableAndResetCreator(
        const SqliteDatabase& db,   ///< the database in which the table shall be created
        const std::string& tabName   ///< the name of the to-be-created table
        );

  private:
    Sloppy::StringList constraintCache;
    Sloppy::StringList colDefs;
  };
  
}

