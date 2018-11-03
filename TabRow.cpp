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
#include "DbTab.h"
#include "CommonTabularClass.h"
#include "ClausesAndQueries.h"

namespace SqliteOverlay
{

  TabRow::TabRow(const SqliteDatabase* _db, const string& _tabName, int _rowId, bool skipCheck)
  : db(_db), tabName(_tabName), rowId(_rowId),
    cachedWhereStatementForRow{" FROM " + tabName + " WHERE id = " + to_string(rowId)},
    cachedUpdateStatementForRow{"UPDATE " + tabName + " SET %1=? WHERE id=" + to_string(rowId)}
  {
    if ((db == nullptr) || tabName.empty() || (rowId < 1))
    {
      throw std::invalid_argument("TabRow ctor: empty or invalid parameters");
    }

    if (!skipCheck)
    {
      SqlStatement stmt = db->prepStatement("SELECT COUNT(*) " + cachedWhereStatementForRow);
      if (db->execScalarQueryInt(stmt) == 0)
      {
        throw std::invalid_argument("TabRow ctor: invalid row ID");
      }
    }

  }

//----------------------------------------------------------------------------

  TabRow::TabRow(const SqliteDatabase* _db, const string& _tabName, const WhereClause& where)
  : db(_db), tabName(_tabName), rowId(-1)
  {
    if ((db == nullptr) || tabName.empty())
    {
      throw std::invalid_argument("TabRow ctor: empty or invalid parameters");
    }

    WhereClause w{where};
    w.setLimit(1);
    SqlStatement stmt = w.getSelectStmt(db, tabName, false);
    try
    {
      rowId = db->execScalarQueryInt(stmt);
    }
    catch (BusyException e)
    {
      throw;
    }
    catch(...)
    {
      throw std::invalid_argument("TabRow ctor: invalid WHERE clause or no match for WHERE clause");
    }

    cachedWhereStatementForRow = " FROM " + tabName + " WHERE id = " + to_string(rowId);
    cachedUpdateStatementForRow = "UPDATE " + tabName + " SET %1=? WHERE id=" + to_string(rowId);
  }

//----------------------------------------------------------------------------

  int TabRow::getId() const
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
    db->execNonQuery(stmt);
  }

//----------------------------------------------------------------------------

  string TabRow::operator [](const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }
    
    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    return db->execScalarQueryString(sql);
  }

//----------------------------------------------------------------------------

  int TabRow::getInt(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    return db->execScalarQueryInt(sql);
  }

//----------------------------------------------------------------------------

  double TabRow::getDouble(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    return db->execScalarQueryDouble(sql);
  }

  //----------------------------------------------------------------------------

  LocalTimestamp TabRow::getLocalTime(const string& colName, boost::local_time::time_zone_ptr tzp) const
  {
    time_t rawTime = getInt(colName);
    return LocalTimestamp(rawTime, tzp);
  }

  //----------------------------------------------------------------------------

  UTCTimestamp TabRow::getUTCTime(const string& colName) const
  {
    time_t rawTime = getInt(colName);
    return UTCTimestamp(rawTime);
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
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    return db->execScalarQueryIntOrNull(sql);
  }

//----------------------------------------------------------------------------

  optional<double> TabRow::getDouble2(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    return db->execScalarQueryDoubleOrNull(sql);
  }

//----------------------------------------------------------------------------

  optional<string> TabRow::getString2(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    return db->execScalarQueryStringOrNull(sql);
  }

//----------------------------------------------------------------------------

  optional<LocalTimestamp> TabRow::getLocalTime2(const string& colName, boost::local_time::time_zone_ptr tzp) const
  {
    optional<int> rawTime = getInt2(colName);

    if (rawTime.has_value())
    {
      return LocalTimestamp{rawTime.value(), tzp};
    }

    return optional<LocalTimestamp>{};
  }

//----------------------------------------------------------------------------

  optional<UTCTimestamp> TabRow::getUTCTime2(const string& colName) const
  {
    optional<int> rawTime = getInt2(colName);

    if (rawTime.has_value())
    {
      return UTCTimestamp{rawTime.value()};
    }

    return optional<UTCTimestamp>{};
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

  void TabRow::update(const string& colName, const LocalTimestamp& newVal) const
  {
    ColumnValueClause cvc;
    cvc.addDateTimeCol(colName, &newVal);
    update(cvc);
  }

//----------------------------------------------------------------------------

  void TabRow::update(const string& colName, const UTCTimestamp& newVal) const
  {
    ColumnValueClause cvc;
    cvc.addDateTimeCol(colName, &newVal);
    update(cvc);
  }

//----------------------------------------------------------------------------

  void TabRow::update(const string& colName, const boost::gregorian::date& newVal) const
  {
    Sloppy::estring sql{cachedUpdateStatementForRow};
    sql.arg(colName);
    SqlStatement stmt = db->prepStatement(sql);
    stmt.bindInt(1, greg::to_int(newVal));

    db->execNonQuery(stmt);
  }

//----------------------------------------------------------------------------

  void TabRow::updateToNull(const string& colName) const
  {
    ColumnValueClause cvc;
    cvc.addNullCol(colName);
    update(cvc);
  }

//----------------------------------------------------------------------------

  const SqliteDatabase* TabRow::getDb() const
  {
    return db;
  }

//----------------------------------------------------------------------------

  void TabRow::erase() const
  {
    string sql = "DELETE " + cachedWhereStatementForRow;
    db->execNonQuery(sql);
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
