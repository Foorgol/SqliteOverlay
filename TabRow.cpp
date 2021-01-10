/*
 * Copyright © 2014 Volker Knollmann
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file or visit
 * http://www.wtfpl.net/ for more details.
 * 
 * This program comes without any warranty. Use it at your own risk or
 * don't use it at all.
 */

#include <sys/types.h>          // for time_t
#include <cstdint>              // for int64_t
#include <iosfwd>               // for std
#include <stdexcept>            // for invalid_argument
#include <utility>              // for move

#include "ClausesAndQueries.h"  // for ColumnValueClause, WhereClause
#include "TabRow.h"

using namespace std;

namespace SqliteOverlay
{

  TabRow::TabRow(const SqliteDatabase& _db, const string& _tabName, int _rowId, bool skipCheck)
    : db{cref(_db)}, tabName(_tabName), rowId(_rowId),
    cachedWhereStatementForRow{" FROM " + tabName + " WHERE rowid = " + to_string(rowId)},
    cachedUpdateStatementForRow{"UPDATE " + tabName + " SET %1=? WHERE rowid=" + to_string(rowId)}
  {
    if (tabName.empty() || (rowId < 1))
    {
      throw std::invalid_argument("TabRow ctor: empty or invalid parameters");
    }

    if (skipCheck) return; // done if we're working without ID check

    try
    {
      SqlStatement stmt = db.get().prepStatement("SELECT rowid " + cachedWhereStatementForRow);
      stmt.step();
      stmt.get<int>(0);
    }
    catch (SqlStatementCreationError)
    {
      throw std::invalid_argument("TabRow ctor: invalid table name");
    }
    catch (NoDataException)
    {
      throw std::invalid_argument("TabRow ctor: invalid row ID");
    }

  }

//----------------------------------------------------------------------------

  TabRow::TabRow(const SqliteDatabase& _db, const string& _tabName, const WhereClause& where)
  : db(_db), tabName(_tabName), rowId(-1)
  {
    if (tabName.empty() || where.isEmpty())
    {
      throw std::invalid_argument("TabRow ctor: empty or invalid parameters");
    }

    //WhereClause w{where};
    //w.setLimit(1);
    try
    {
      SqlStatement stmt = where.getSelectStmt(db, tabName, false);
      rowId = db.get().execScalarQueryInt(stmt);
    }
    catch (BusyException e)
    {
      throw;
    }
    catch(...)
    {
      throw std::invalid_argument("TabRow ctor: invalid WHERE clause or no match for WHERE clause");
    }

    cachedWhereStatementForRow = " FROM " + tabName + " WHERE rowid = " + to_string(rowId);
    cachedUpdateStatementForRow = "UPDATE " + tabName + " SET %1=? WHERE rowid=" + to_string(rowId);
  }

  //----------------------------------------------------------------------------

  TabRow& TabRow::operator=(const TabRow& other)
  {
    db = ref(other.db);
    tabName = other.tabName;
    rowId = other.rowId;
    cachedWhereStatementForRow = other.cachedWhereStatementForRow;
    cachedUpdateStatementForRow = other.cachedUpdateStatementForRow;

    return *this;
  }

  //----------------------------------------------------------------------------

  TabRow::TabRow(const TabRow& other)
    :db{other.db}
  {
    tabName = other.tabName;
    rowId = other.rowId;
    cachedWhereStatementForRow = other.cachedWhereStatementForRow;
    cachedUpdateStatementForRow = other.cachedUpdateStatementForRow;
  }

  //----------------------------------------------------------------------------

  TabRow& TabRow::operator=(TabRow&& other)
  {
    db = other.db;
    tabName = std::move(other.tabName);
    rowId = other.rowId;
    cachedWhereStatementForRow = std::move(other.cachedWhereStatementForRow);
    cachedUpdateStatementForRow = std::move(other.cachedUpdateStatementForRow);

    // pseudo-invalidation; the string should already
    // be empty due to the move op earlier
    other.rowId = -1;

    return *this;
  }

  //----------------------------------------------------------------------------

  TabRow::TabRow(TabRow&& other)
    :db{other.db}
  {
    operator=(std::move(other));
  }

//----------------------------------------------------------------------------

  int TabRow::id() const
  {
    return rowId;
  }

//----------------------------------------------------------------------------

  void TabRow::update(const ColumnValueClause& cvc) const
  {
    if (!(cvc.hasColumns()))
    {
      return;  // nothing to do
    }
    
    // create and execute the SQL statement
    SqlStatement stmt = cvc.getUpdateStmt(db, tabName, rowId);
    db.get().execNonQuery(stmt);
  }

//----------------------------------------------------------------------------

  string TabRow::operator [](const string& colName) const
  {
    return get<string>(colName);
  }

  //----------------------------------------------------------------------------

  optional<Sloppy::DateTime::WallClockTimepoint_secs> TabRow::get2(const string& colName, date::time_zone* tzp) const
  {
    const auto rawTime = get2<int64_t>(colName);

    if (rawTime)
    {
      return Sloppy::DateTime::WallClockTimepoint_secs{rawTime.value(), tzp};
    }

    return std::nullopt;
  }

//----------------------------------------------------------------------------

  void TabRow::updateToNull(const string& colName) const
  {
    ColumnValueClause cvc;
    cvc.addNullCol(colName);
    update(cvc);
  }

//----------------------------------------------------------------------------

  const SqliteDatabase& TabRow::getDb() const
  {
    return db;
  }

//----------------------------------------------------------------------------

  void TabRow::erase() const
  {
    string sql = "DELETE " + cachedWhereStatementForRow;
    db.get().execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  bool TabRow::checkConstraint(const string& colName, Sloppy::ValueConstraint c, string* errMsg) const
  {
    return Sloppy::checkConstraint(get2<std::string>(colName), c, errMsg);
  }

  //----------------------------------------------------------------------------

  Sloppy::CSV_Row TabRow::toCSV(bool includeRowId) const
  {
    string sql = "SELECT ";
    if (includeRowId)
    {
      sql += "rowid,";
    }

    sql += "*" + cachedWhereStatementForRow;
    auto stmt = db.get().prepStatement(sql);
    stmt.step();

    return stmt.toCSV_currentRowOnly();
  }

  //----------------------------------------------------------------------------

  Sloppy::CSV_Row TabRow::toCSV(const std::vector<string>& colNames) const
  {
    if (colNames.empty()) return Sloppy::CSV_Row{};

    auto it = colNames.cbegin();
    std::string colList = *it;
    ++it;
    for (; it != colNames.cend(); ++it)
    {
      colList += "," + *it;
    }

    auto stmt = db.get().prepStatement("SELECT " + colList + cachedWhereStatementForRow);
    stmt.step();

    return stmt.toCSV_currentRowOnly();
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

}
