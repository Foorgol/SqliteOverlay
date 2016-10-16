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

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"

namespace SqliteOverlay
{
  enum class CONFLICT_CLAUSE
  {
    ROLLBACK,
    ABORT,
    FAIL,
    IGNORE,
    REPLACE,
    __NOT_SET
  };

  class TableCreator
  {
  public:
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
                               CONSISTENCY_ACTION onUpdate=CONSISTENCY_ACTION::__NOT_SET);
    // reset all internal data
    void reset();

    // the actual table creation
    DbTab* createTableAndResetCreator(const string& tabName, int* errCodeOut=nullptr);

  private:
    SqliteDatabase* db;
    Sloppy::StringList foreignKeyCreationCache;
    Sloppy::StringList colDefs;
    string conflictClause2String(CONFLICT_CLAUSE cc);
  };
  
}
#endif	/* TABLECREATOR_H */

