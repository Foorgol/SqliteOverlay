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

#include <stdexcept>

#include <boost/date_time/local_time/local_time.hpp>

#include "TabRow.h"
//#include "DbTab.h"
#include "CommonTabularClass.h"
#include "ClausesAndQueries.h"

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
      stmt.getInt(0);
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

    WhereClause w{where};
    w.setLimit(1);
    try
    {
      SqlStatement stmt = w.getSelectStmt(db, tabName, false);
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

  Sloppy::MemArray TabRow::get(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }
    SqlStatement stmt;
    std::string sql = "SELECT " + colName + cachedWhereStatementForRow;
    try
    {
      stmt = db.get().prepStatement(sql);
    }
    catch (SqlStatementCreationError)
    {
      throw std::invalid_argument("Column access: received invalid column name");
    }
    catch (...)
    {
      throw;
    }

    stmt.step();

    return stmt.getBlob(0);
  }

  //----------------------------------------------------------------------------

  int TabRow::getInt(const string& colName) const
  {
    return get<int>(colName);
  }

  //----------------------------------------------------------------------------

  long TabRow::getLong(const string& colName) const
  {
    return get<long>(colName);
  }

  //----------------------------------------------------------------------------

  double TabRow::getDouble(const string& colName) const
  {
    return get<double>(colName);
  }

  //----------------------------------------------------------------------------

  Sloppy::MemArray TabRow::getBlob(const string& colName) const
  {
    return get<Sloppy::MemArray>(colName);
  }

  //----------------------------------------------------------------------------

  LocalTimestamp TabRow::getLocalTime(const string& colName, boost::local_time::time_zone_ptr tzp) const
  {
    time_t rawTime = getLong(colName);
    return LocalTimestamp(rawTime, tzp);
  }

  //----------------------------------------------------------------------------

  UTCTimestamp TabRow::getUTCTime(const string& colName) const
  {
    return get<UTCTimestamp>(colName);
  }

  //----------------------------------------------------------------------------

  nlohmann::json TabRow::getJson(const string& colName) const
  {
    return get<nlohmann::json>(colName);
  }

  //----------------------------------------------------------------------------

  boost::gregorian::date TabRow::getDate(const string& colName) const
  {
    int ymd = getInt(colName);
    return greg::from_int(ymd);
  }

//----------------------------------------------------------------------------

  optional<int> TabRow::getInt2(const string& colName) const
  {
    return get<optional<int>>(colName);
  }

  //----------------------------------------------------------------------------

  optional<long> TabRow::getLong2(const string& colName) const
  {
    return get<optional<long>>(colName);
  }

//----------------------------------------------------------------------------

  optional<double> TabRow::getDouble2(const string& colName) const
  {
    return get<optional<double>>(colName);
  }

  //----------------------------------------------------------------------------

  std::optional<Sloppy::MemArray> TabRow::getBlob2(const string& colName) const
  {
    return get<optional<Sloppy::MemArray>>(colName);
  }

//----------------------------------------------------------------------------

  optional<string> TabRow::getString2(const string& colName) const
  {
    return get<optional<string>>(colName);
  }

//----------------------------------------------------------------------------

  optional<LocalTimestamp> TabRow::getLocalTime2(const string& colName, boost::local_time::time_zone_ptr tzp) const
  {
    optional<long> rawTime = getLong2(colName);

    if (rawTime.has_value())
    {
      return LocalTimestamp{rawTime.value(), tzp};
    }

    return optional<LocalTimestamp>{};
  }

//----------------------------------------------------------------------------

  optional<UTCTimestamp> TabRow::getUTCTime2(const string& colName) const
  {
    return get<optional<UTCTimestamp>>(colName);
  }

  //----------------------------------------------------------------------------

  optional<nlohmann::json> TabRow::getJson2(const string& colName) const
  {
    return get<optional<nlohmann::json>>(colName);
  }

  //----------------------------------------------------------------------------

  optional<boost::gregorian::date> TabRow::getDate2(const string& colName) const
  {
    auto ymd = getInt2(colName);
    if (ymd.has_value())
    {
      return greg::from_int(ymd.value());
    }

    return optional<greg::date>{};
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
    return Sloppy::checkConstraint(getString2(colName), c, errMsg);
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
