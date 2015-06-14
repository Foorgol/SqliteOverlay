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

#include "CommonTabularClass.h"
#include "HelperFunc.h"

namespace SqliteOverlay
{

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

  string ColInfo::getColType() const
  {
    return type;
  }

//----------------------------------------------------------------------------

  /**
   * Basic constructor of a table or view
   * 
   * @param _db the reference to the database instance for this table / view
   * @param _tabName the name of the table / view
   * @param _isView true if name refers to a view and "false" for tables
   */
  CommonTabularClass::CommonTabularClass(SqliteDatabase* _db, const string& _tabName, bool _isView)
  : db(_db), tabName(_tabName), isView(_isView)
  {
    if (db == NULL)
    {
      throw invalid_argument("Received null pointer for database handle");
    }
    
    if (tabName.empty())
    {
      throw invalid_argument("Receied emtpty table or view name");
    }
    
    if ((isView) && (!db->hasView(tabName)))
    {
      throw invalid_argument("Tried to access non-existing view");
    }
    
    if ((!isView) && (!db->hasTable(tabName)))
    {
      throw invalid_argument("Tried to access non-existing table");
    }
  }

//----------------------------------------------------------------------------

  CommonTabularClass::~CommonTabularClass()
  {
  }

//----------------------------------------------------------------------------

  ColInfoList CommonTabularClass::allColDefs() const
  {
    ColInfoList result;
    auto stmt = db->execContentQuery("PRAGMA table_info(" + tabName + ")");
      
    while (stmt->hasData())
    {
      int colId;
      string colName;
      string colType;
      stmt->getInt(0, &colId);
      stmt->getString(1, &colName);
      stmt->getString(2, &colType);

      ColInfo ci(colId, colName, colType);
      result.push_back(ci);
      stmt->step();
    }
      
    return result;
  }

//----------------------------------------------------------------------------

  string CommonTabularClass::getColType(const string& colName) const
  {
    ColInfoList cil = allColDefs();

    for (ColInfo ci : cil)
    {
      if (ci.getColName() == colName) {
        return ci.getColType();
      }
    };
    
    return string();
  }

//----------------------------------------------------------------------------

  string CommonTabularClass::cid2name(int cid) const
  {
    ColInfoList cil = allColDefs();

    for (ColInfo ci : cil)
    {
      if (ci.getId() == cid) {
        return ci.getColName();
      }
    };

    return string();
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::name2cid(const string& colName) const
  {
    ColInfoList cil = allColDefs();

    for (ColInfo ci : cil)
    {
      if (ci.getColName() == colName) {
        return ci.getId();
      }
    };

    return -1;
  }

//----------------------------------------------------------------------------

  bool CommonTabularClass::hasColumn(const string& colName) const
  {
    ColInfoList cil = allColDefs();

    for (ColInfo ci : cil)
    {
      if (ci.getColName() == colName) {
        return true;
      }
    };

    return false;
  }

//----------------------------------------------------------------------------

  bool CommonTabularClass::hasColumn(int cid) const
  {
    ColInfoList cil = allColDefs();
    
    return ((cid >= 0) && (cid < cil.size()));
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForWhereClause(const WhereClause& w) const
  {
    string sql = w.getSql(tabName, true);
    int cnt;
    bool isOk = db->execScalarQueryInt(sql, &cnt);

    return isOk ? cnt : -1;
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValue(const string& col, const string& val) const
  {
    WhereClause w;
    w.addStringCol(col, val);
    return getMatchCountForWhereClause(w);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValue(const string& col, int val) const
  {
    WhereClause w;
    w.addIntCol(col, val);
    return getMatchCountForWhereClause(w);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValue(const string& col, double val) const
  {
    WhereClause w;
    w.addDoubleCol(col, val);
    return getMatchCountForWhereClause(w);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForWhereClause(const string& where) const
  {
    string sql = "SELECT COUNT(*) FROM " + tabName + " WHERE " + where;

    int cnt;
    bool isOk = db->execScalarQueryInt(sql, &cnt);

    return isOk ? cnt : -1;
  }


//----------------------------------------------------------------------------

  int CommonTabularClass::length() const
  {
    string sql = "SELECT count(*) FROM " + tabName;
    int result;
    db->execScalarQueryInt(sql, &result);
    return result;
  }

//----------------------------------------------------------------------------

  WhereClause::WhereClause()
  {
    clear();
  }

  void WhereClause::clear()
  {
    sql.clear();
    colCount = 0;
  }

//----------------------------------------------------------------------------

  void WhereClause::addIntCol(const string& colName, int val)
  {
    addCol(colName, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addIntCol(const string& colName, const string& op, int val)
  {
    addCol(colName, op, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addDoubleCol(const string& colName, double val)
  {
    addCol(colName, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addDoubleCol(const string& colName, const string& op, double val)
  {
    addCol(colName, op, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addStringCol(const string& colName, const string& val)
  {
    addCol(colName, val, true);
  }

  //----------------------------------------------------------------------------

  void WhereClause::addStringCol(const string& colName, const string& op, const string& val)
  {
    addCol(colName, op, val, true);
  }

  //----------------------------------------------------------------------------

  void WhereClause::addNullCol(const string& colName)
  {
    if (colCount > 0)
    {
      sql += " AND ";
    }
    sql += colName + " IS NULL";

    ++colCount;
  }

//----------------------------------------------------------------------------

  string WhereClause::getSql(const string& tabName, bool countOnly) const
  {
    if (tabName.empty())
    {
      return string();
    }

    string result = "SELECT ";
    if (countOnly) result += "COUNT(*)";
    else result += "id";
    result += " FROM " + tabName + " WHERE " + sql;

    return result;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addCol(const string& colName, const string& val, bool useQuotes)
  {
    addCol(colName, "=", val, useQuotes);
  }

  //----------------------------------------------------------------------------

  void WhereClause::addCol(const string& colName, const string& op, const string& val, bool useQuotes)
  {
    if (colCount > 0)
    {
      sql += " AND ";
    }
    sql += colName + op;
    if (useQuotes) sql += "'" + val + "'";
    else sql += val;

    ++colCount;
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

}