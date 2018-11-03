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

#include <Sloppy/String.h>

#include "CommonTabularClass.h"
#include "ClausesAndQueries.h"
#include "SqlStatement.h"

namespace SqliteOverlay
{

  ColInfo::ColInfo(int _cid, const string& _name, const string& _type)
    : cid (_cid), name (_name), type{string2Affinity(_type)}
  {
  }

  //----------------------------------------------------------------------------

  int ColInfo::getId() const
  {
    return cid;
  }
  
  //----------------------------------------------------------------------------

  string ColInfo::getColName() const
  {
    return name;
  }

//----------------------------------------------------------------------------

  ColumnDataType ColInfo::getColType() const
  {
    return type;
  }

//----------------------------------------------------------------------------

  CommonTabularClass::CommonTabularClass(SqliteDatabase* _db, const string& _tabName, bool _isView, bool forceNameCheck)
  : db(_db), tabName(_tabName), isView(_isView),
    sqlColumnCount{"SELECT COUNT(*) FROM " + tabName + " WHERE "}
  {
    if (db == NULL)
    {
      throw invalid_argument("Received null pointer for database handle");
    }
    
    if (tabName.empty())
    {
      throw invalid_argument("Received emtpty table or view name");
    }

    if (forceNameCheck)
    {
      if (!(db->hasTable(tabName, isView)))
      {
        throw NoSuchTableException("CommonTabularClass ctor for table/view named " + tabName);
      }
    }
  }

//----------------------------------------------------------------------------

  ColInfoList CommonTabularClass::allColDefs() const
  {
    ColInfoList result;
    SqlStatement stmt = db->execContentQuery("PRAGMA table_info(" + tabName + ")");
      
    while (stmt.hasData())
    {
      int colId = stmt.getInt(0);
      string colName = stmt.getString(1);
      string colType = stmt.getString(2);

      result.emplace_back(colId, colName, colType);
      stmt.step();
    }
      
    return result;
  }

//----------------------------------------------------------------------------

  ColumnDataType CommonTabularClass::getColType(const string& colName) const
  {
    SqlStatement stmt = db->prepStatement("SELECT type FROM pragma_table_info(?) WHERE name=?");
    stmt.bindString(1, tabName);
    stmt.bindString(2, colName);

    try
    {
      string t = db->execScalarQueryString(stmt);
      return string2Affinity(t);
    }
    catch (...)
    {
      return ColumnDataType::Null;
    }
  }

//----------------------------------------------------------------------------

  string CommonTabularClass::cid2name(int cid) const
  {
    SqlStatement stmt = db->prepStatement("SELECT name FROM pragma_table_info(?) WHERE cid=?");
    stmt.bindString(1, tabName);
    stmt.bindInt(2, cid);

    try
    {
      return db->execScalarQueryString(stmt);
    }
    catch (...)
    {
      return "";
    }
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::name2cid(const string& colName) const
  {
    SqlStatement stmt = db->prepStatement("SELECT cid FROM pragma_table_info(?) WHERE name=?");
    stmt.bindString(1, tabName);
    stmt.bindString(2, colName);

    try
    {
      return db->execScalarQueryInt(stmt);
    }
    catch (...)
    {
      return -1;
    }
  }

//----------------------------------------------------------------------------

  bool CommonTabularClass::hasColumn(const string& colName) const
  {
    SqlStatement stmt = db->prepStatement("SELECT count(*) FROM pragma_table_info(?) WHERE name=?");
    stmt.bindString(1, tabName);
    stmt.bindString(2, colName);

    return (db->execScalarQueryInt(stmt) > 0);
  }

//----------------------------------------------------------------------------

  bool CommonTabularClass::hasColumn(int cid) const
  {
    SqlStatement stmt = db->prepStatement("SELECT count(*) FROM pragma_table_info(?) WHERE cid=?");
    stmt.bindString(1, tabName);
    stmt.bindInt(2, cid);

    return (db->execScalarQueryInt(stmt) > 0);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForWhereClause(const WhereClause& w) const
  {
    SqlStatement stmt = w.getSelectStmt(db, tabName, true);
    return db->execScalarQueryInt(stmt);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValue(const string& col, const string& val) const
  {
    if (col.empty())
    {
      throw std::invalid_argument("getMatchCountForColumnValue(): empty column name");
    }

    string sql = sqlColumnCount + col + "=?";
    SqlStatement stmt = db->prepStatement(sql);
    stmt.bindString(1, val);
    return db->execScalarQueryInt(stmt);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValue(const string& col, int val) const
  {
    if (col.empty())
    {
      throw std::invalid_argument("getMatchCountForColumnValue(): empty column name");
    }

    string sql = sqlColumnCount + col + "=?";
    SqlStatement stmt = db->prepStatement(sql);
    stmt.bindInt(1, val);
    return db->execScalarQueryInt(stmt);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValue(const string& col, double val) const
  {
    if (col.empty())
    {
      throw std::invalid_argument("getMatchCountForColumnValue(): empty column name");
    }

    string sql = sqlColumnCount + col + "=?";
    SqlStatement stmt = db->prepStatement(sql);
    stmt.bindDouble(1, val);
    return db->execScalarQueryInt(stmt);
  }

  //----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValue(const string& col, const CommonTimestamp* pTimestamp) const
  {
    if (col.empty())
    {
      throw std::invalid_argument("getMatchCountForColumnValue(): empty column name");
    }

    // thanks to polymorphism, this works for
    // both LocalTimestamp and UTCTimestamp
    time_t rawTime = pTimestamp->getRawTime();

    string sql = sqlColumnCount + col + "=?";
    SqlStatement stmt = db->prepStatement(sql);
    stmt.bindInt(1, rawTime);
    return db->execScalarQueryInt(stmt);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValueNull(const string& col) const
  {
    if (col.empty())
    {
      throw std::invalid_argument("getMatchCountForColumnValue(): empty column name");
    }

    string sql = sqlColumnCount + col + " IS NULL";
    SqlStatement stmt = db->prepStatement(sql);
    return db->execScalarQueryInt(stmt);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForWhereClause(const string& where) const
  {
    string sql = "SELECT COUNT(*) FROM " + tabName + " WHERE " + where;

    return db->execScalarQueryInt(sql);
  }


//----------------------------------------------------------------------------

  int CommonTabularClass::length() const
  {
    return db->execScalarQueryInt("SELECT COUNT(*) FROM " + tabName);
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
