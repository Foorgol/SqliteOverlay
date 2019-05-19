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
  /** \brief Convenience class for easy creation of new tables
   */
  class TableCreator
  {
  public:
<<<<<<< HEAD
    // ctor
    TableCreator(SqliteDatabase* _db);

    // adding columns
    void addVarchar(const string& colName, int len, bool isUnique=false, CONFLICT_CLAUSE uniqueConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                    bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                    bool hasDefault=false, const string& defaultValue="");
    void addInt(const string& colName, bool isUnique=false, CONFLICT_CLAUSE uniqueConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                bool hasDefault=false, const string& defaultValue="");
    void addText(const string& colName, bool isUnique=false, CONFLICT_CLAUSE uniqueConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                 bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                 bool hasDefault=false, const string& defaultValue="");
    void addCol(const string& colDef);
    void addCol(const string& colName, const string& typeName, bool isUnique=false, CONFLICT_CLAUSE uniqueConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                 bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                 bool hasDefault=false, const string& defaultValue="");

    // add foreign keys
    void addForeignKey(const string& keyName, const string& referedTable,
                               CONSISTENCY_ACTION onDelete=CONSISTENCY_ACTION::__NOT_SET,
                               CONSISTENCY_ACTION onUpdate=CONSISTENCY_ACTION::__NOT_SET,
                       bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                       bool isUnique=false, CONFLICT_CLAUSE uniqueConflictClause=CONFLICT_CLAUSE::__NOT_SET);

    // add a unique combination of two or more
    // column values
    bool addUniqueColumnCombination(initializer_list<string> colNames, CONFLICT_CLAUSE notUniqueConflictClause);

    // reset all internal data
=======
    /** \brief Default ctor, does nothing
     */
    TableCreator() {}

    /** \brief Adds a column *without* a default value
     *
     * \throws std::invalid_argument if the column name is empty
     */
    void addCol(
        const string& colName,   ///< the new column's name
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
        const string& colName,   ///< the new column's name
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

      string colDef = buildColumnConstraint(uniqueConflictClause, notNullConflictClause, defaultValue);

      colDef = colName + " " + to_string(t) + " " + colDef;

      colDefs.push_back(colDef);
    }

    /** \brief Adds a colunm with a fully custom column definition
     */
    void addCol(
        const string& colDef   ///< the custom column definition
        );

    /** \brief Adds a colunm for a foreign key
     */
    void addForeignKey(
        const string& keyName,   ///< the name of the column that contains the foreign key
        const string& referedTable,   ///< the name of the table that should be referenced
        ConsistencyAction onDelete,   ///< the action that should be triggered on deletion of the target row(s)
        ConsistencyAction onUpdate,   ///< the action that should be triggered on updates of the target row(s)
        ConflictClause uniqueConflictClause,   ///< enforcement of unique key values; set to `NotUsed' if you don't need uniqueness
        ConflictClause notNullConflictClause,   ///< enforcement of non-NULL key values; set to `NotUsed' if you want to allow NULL
        const string& referedColumn = "id"   ///< the refered-to column's name; usually it's just "id"; "rowid" is not allowed by SQLite
        );

    /** \brief Adds a constraint that enforces a unique combination of two or more column values
     *
     * \returns `false` if just one column name was provided, `true` otherwise
     */
    bool addUniqueColumnCombination(
        initializer_list<string> colNames,   ///< a list of column names
        ConflictClause notUniqueConflictClause   ///< the action that should be taken if the requested constraint would be violated
        );

    /** \brief Erases all previously added definitions from the objects
     * and resets it back to its initial state
     */
>>>>>>> dev
    void reset();

    /** \brief Creates the final SQL-statement text for creating the table; the database itself is left untouched
     *
     * \returns a string with the `CREATE TABLE` command
     */
    string getSqlStatement(
        const string& tabName   ///< the name of the to-be-created table
        ) const;

    /** \brief Builds and executes the command for creating the table
     *
     * \returns a `DbTab` instance for the new table
     */
    DbTab createTableAndResetCreator(
        const SqliteDatabase& db,   ///< the database in which the table shall be created
        const string& tabName   ///< the name of the to-be-created table
        );

  private:
    Sloppy::StringList constraintCache;
    Sloppy::StringList colDefs;
  };
  
}
#endif	/* TABLECREATOR_H */

