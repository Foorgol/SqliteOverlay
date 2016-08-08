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

#include "libSloppy.h"

#include "TabRow.h"
#include "HelperFunc.h"
#include "DbTab.h"
#include "CommonTabularClass.h"

namespace SqliteOverlay
{

  /**
   * Constructor for a known rowID
   * 
   * @param _db the associated database instance
   * @param _tabName the associated table name
   * @param _rowId 
   */
  TabRow::TabRow(SqliteDatabase* _db, const string& _tabName, int _rowId, bool skipCheck)
  : db(_db), tabName(_tabName), rowId(_rowId)
  {
    // Dangerous short-cut for lib-internal purposes:
    // if we're told that _db, _tabName and _rowId are GUARANTEED to be correct
    // (e. g. they result from a previous SELECT), we don't need further database
    // queries to check them again
    if (skipCheck)
    {
      cachedWhereStatementForRow = " FROM " + tabName + " WHERE id = " + to_string(rowId);
      return; // all done
    }
    
    WhereClause where;
    where.addIntCol("id", rowId);
    doInit(where);
  }

//----------------------------------------------------------------------------

  /**
   * Constructor for the first row in the table that matches a custom where clause.
   * 
   * Throws an exception if the requested row doesn't exist
   * 
   * @param _db the associated database instance
   * @param _tabName the associated table name
   * @param args where clause and/or column/value pairs
   */
  TabRow::TabRow(SqliteDatabase* _db, const string& _tabName, const WhereClause& where)
  : db(_db), tabName(_tabName), rowId(-1)
  {
    doInit(where);
  }

//----------------------------------------------------------------------------

  TabRow::~TabRow()
  {
  }

//----------------------------------------------------------------------------

  bool TabRow::doInit(const WhereClause& where)
  {
    // make sure the database handle is correct
    if (db == NULL)
    {
      throw std::invalid_argument("Received nullptr for database handle");
    }
    
    // make sure the table name exists
    // the check is performed by the DbTab constructor (so we don't
    // need to re-write it here) and we can reuse the DbTab instance later
    // in this method
    DbTab* tab = db->getTab(tabName);
    if (tab == nullptr)
    {
      throw std::invalid_argument("Received invalid table name!");
    }

    // create and execute a "SELECT id FROM ..." from the where clause
    // and limit it to the first hit
    string sql = where.getSelectStmt(tabName, false);
    sql += " LIMIT 1";
    auto stmt = db->prepStatement(sql, nullptr);
    if (stmt == nullptr)
    {
      throw std::invalid_argument("Invalid where clause or no matches for where clause");
    }
    if (!(stmt->step()))
    {
      throw std::invalid_argument("Invalid where clause or no matches for where clause");
    }

    if (!(stmt->hasData()))
    {
      throw std::invalid_argument("Invalid where clause or no matches for where clause");
    }

    if (!(stmt->getInt(0, &rowId)))
    {
      throw std::invalid_argument("Invalid where clause or no matches for where clause");
    }

    cachedWhereStatementForRow = " FROM " + tabName + " WHERE id = " + to_string(rowId);

    return true;
  }

//----------------------------------------------------------------------------

  int TabRow::getId() const
  {
    return rowId;
  }

//----------------------------------------------------------------------------

  bool TabRow::update(const ColumnValueClause& cvc, int* errCodeOut) const
  {
    if (!(cvc.hasColumns()))
    {
      return true;  // nothing to do
    }
    
    // create and execute the SQL statement
    string sql = cvc.getUpdateStmt(tabName, rowId);

    return db->execNonQuery(sql, errCodeOut);
  }

//----------------------------------------------------------------------------

  string TabRow::operator [](const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }
    
    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    string result;
    bool isOk = db->execScalarQueryString(sql, &result, nullptr);
    
    if (!isOk)
    {
      throw std::invalid_argument("Column access: received invalid column name or row has been deleted in the meantime");
    }
    
    return result;
  }

//----------------------------------------------------------------------------

  int TabRow::getInt(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    int result;
    bool isOk = db->execScalarQueryInt(sql, &result, nullptr);

    if (!isOk)
    {
      throw std::invalid_argument("Column access: received invalid column name or row has been deleted in the meantime");
    }

    return result;
  }

//----------------------------------------------------------------------------

  double TabRow::getDouble(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;
    double result;
    bool isOk = db->execScalarQueryDouble(sql, &result, nullptr);

    if (!isOk)
    {
      throw std::invalid_argument("Column access: received invalid column name or row has been deleted in the meantime");
    }

    return result;
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

  unique_ptr<ScalarQueryResult<int> > TabRow::getInt2(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;

    return db->execScalarQueryInt(sql, nullptr);
  }

//----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<double> > TabRow::getDouble2(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;

    return db->execScalarQueryDouble(sql, nullptr);
  }

//----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<string> > TabRow::getString2(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;

    return db->execScalarQueryString(sql, nullptr);
  }

//----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<LocalTimestamp> > TabRow::getLocalTime2(const string& colName, boost::local_time::time_zone_ptr tzp) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;

    auto rawTime = db->execScalarQueryInt(sql, nullptr);

    if (rawTime->isNull())
    {
      return unique_ptr<ScalarQueryResult<LocalTimestamp>>(new ScalarQueryResult<LocalTimestamp>(tzp));
    }

    LocalTimestamp result(rawTime->get(), tzp);
    return unique_ptr<ScalarQueryResult<LocalTimestamp>>(new ScalarQueryResult<LocalTimestamp>(result));
  }

//----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<UTCTimestamp> > TabRow::getUTCTime2(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Column access: received empty column name");
    }

    string sql = "SELECT " + colName + cachedWhereStatementForRow;

    auto rawTime = db->execScalarQueryInt(sql, nullptr);

    if (rawTime->isNull())
    {
      return unique_ptr<ScalarQueryResult<UTCTimestamp>>(new ScalarQueryResult<UTCTimestamp>());
    }

    UTCTimestamp result(rawTime->get());
    return unique_ptr<ScalarQueryResult<UTCTimestamp>>(new ScalarQueryResult<UTCTimestamp>(result));
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<boost::gregorian::date> > TabRow::getDate2(const string& colName) const
  {
    auto ymd = getInt2(colName);
    if (ymd == nullptr) return nullptr;

    if (ymd->isNull())
    {
      return unique_ptr<ScalarQueryResult<greg::date>>(new ScalarQueryResult<greg::date>());
    }

    greg::date d = greg::from_int(ymd->get());
    return unique_ptr<ScalarQueryResult<greg::date>>(new ScalarQueryResult<greg::date>(d));
  }

//----------------------------------------------------------------------------

  bool TabRow::update(const string& colName, const int newVal, int* errCodeOut) const
  {
    ColumnValueClause cvc;
    cvc.addIntCol(colName, newVal);
    return update(cvc, errCodeOut);
  }

//----------------------------------------------------------------------------

  bool TabRow::update(const string& colName, const double newVal, int* errCodeOut) const
  {
    ColumnValueClause cvc;
    cvc.addDoubleCol(colName, newVal);
    return update(cvc, errCodeOut);
  }

//----------------------------------------------------------------------------

  bool TabRow::update(const string& colName, const string newVal, int* errCodeOut) const
  {
    ColumnValueClause cvc;
    cvc.addStringCol(colName, newVal);
    return update(cvc, errCodeOut);
  }

//----------------------------------------------------------------------------

  bool TabRow::update(const string& colName, const LocalTimestamp& newVal, int* errCodeOut) const
  {
    ColumnValueClause cvc;
    cvc.addDateTimeCol(colName, &newVal);
    return update(cvc, errCodeOut);
  }

//----------------------------------------------------------------------------

  bool TabRow::update(const string& colName, const UTCTimestamp& newVal, int* errCodeOut) const
  {
    ColumnValueClause cvc;
    cvc.addDateTimeCol(colName, &newVal);
    return update(cvc, errCodeOut);
  }

//----------------------------------------------------------------------------

  bool TabRow::update(const string& colName, const boost::gregorian::date& newVal, int* errCodeOut) const
  {
    return update(colName, greg::to_int(newVal), errCodeOut);
  }

//----------------------------------------------------------------------------

  bool TabRow::updateToNull(const string& colName, int* errCodeOut) const
  {
    ColumnValueClause cvc;
    cvc.addNullCol(colName);
    return update(cvc, errCodeOut);
  }

//----------------------------------------------------------------------------

  SqliteDatabase* TabRow::getDb()
  {
    return db;
  }

//----------------------------------------------------------------------------

  bool TabRow::erase(int* errCodeOut)
  {
    string sql = "DELETE FROM " + tabName + " WHERE id = " + to_string(rowId);
    bool success = db->execNonQuery(sql, errCodeOut);
    
    // make this instance unusable by setting the ID to an invalid value
    if (success)
    {
      rowId = -1;
    }
    
    return success;
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
