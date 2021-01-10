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

#include <iosfwd>               // for std
#include <stdexcept>            // for invalid_argument

#include <Sloppy/String.h>      // for estring

#include "ClausesAndQueries.h"  // for WhereClause
#include "SqlStatement.h"       // for SqlStatement
#include "CommonTabularClass.h"

using namespace std;

namespace SqliteOverlay
{

  ColInfo::ColInfo(int colId, const string& colName, const string& colType)
    : _id{colId}, _name{colName}, _declType{colType}, _affinity{string2Affinity(colType)}
  {
  }

  //----------------------------------------------------------------------------

  int ColInfo::id() const
  {
    return _id;
  }
  
  //----------------------------------------------------------------------------

  string ColInfo::name() const
  {
    return _name;
  }

//----------------------------------------------------------------------------

  string ColInfo::declType() const
  {
    return _declType;
  }

  //----------------------------------------------------------------------------

  ColumnAffinity ColInfo::affinity() const
  {
    return _affinity;
  }

//----------------------------------------------------------------------------

  CommonTabularClass::CommonTabularClass(const SqliteDatabase& _db, const string& _tabName, bool _isView, bool forceNameCheck)
  : db(_db), tabName(_tabName), isView(_isView),
    sqlColumnCount{"SELECT COUNT(*) FROM " + tabName + " WHERE "}
  {
    if (forceNameCheck)
    {
      Sloppy::estring tn{tabName};
      tn.trim();
      if (tn.empty())
      {
        throw invalid_argument("Received emtpty table or view name");
      }

      if (!(db.get().hasTable(tn, isView)))
      {
        throw NoSuchTableException("CommonTabularClass ctor for table/view named " + tabName);
      }

      tabName = tn;
    }
  }

  //----------------------------------------------------------------------------
/*
  CommonTabularClass::CommonTabularClass(const CommonTabularClass& other)
    :db{other.db}, tabName{other.tabName}, isView{other.isView}, sqlColumnCount{other.sqlColumnCount}
  {
  }

  //----------------------------------------------------------------------------

  CommonTabularClass::CommonTabularClass(CommonTabularClass&& other)
    :db{other.db},
     tabName{std::move(other.tabName)}, isView{other.isView}, sqlColumnCount{std::move(other.sqlColumnCount)}
  {
  }

  //----------------------------------------------------------------------------

  CommonTabularClass& CommonTabularClass::operator=(const CommonTabularClass& other)
  {
    db = other.db;
    tabName = other.tabName;
    isView = other.isView;
    sqlColumnCount = other.sqlColumnCount;

    return *this;
  }

  //----------------------------------------------------------------------------

  CommonTabularClass&CommonTabularClass::operator=(CommonTabularClass&& other)
  {
    db = other.db;
    tabName = std::move(other.tabName);
    isView = other.isView;
    sqlColumnCount = std::move(other.sqlColumnCount);

    return *this;
  }
*/

//----------------------------------------------------------------------------

  ColInfoList CommonTabularClass::allColDefs() const
  {
    ColInfoList result;
    SqlStatement stmt = db.get().execContentQuery("PRAGMA table_info(" + tabName + ")");
      
    while (stmt.hasData())
    {
      int colId = stmt.get<int>(0);
      string colName = stmt.get<std::string>(1);
      string colType = stmt.get<std::string>(2);

      result.emplace_back(colId, colName, colType);
      stmt.step();
    }
      
    return result;
  }

//----------------------------------------------------------------------------

  ColumnAffinity CommonTabularClass::colAffinity(const string& colName) const
  {
    return string2Affinity(colDeclType(colName));
  }

  //----------------------------------------------------------------------------

  string CommonTabularClass::colDeclType(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Invalid column name");
    }

    SqlStatement stmt = db.get().prepStatement("SELECT type FROM pragma_table_info(?) WHERE name=?");
    stmt.bind(1, tabName);
    stmt.bind(2, colName);

    stmt.step();
    if (!stmt.hasData())
    {
      throw std::invalid_argument("Invalid column name");
    }

    return stmt.get<std::string>(0);
  }

//----------------------------------------------------------------------------

  string CommonTabularClass::cid2name(int cid) const
  {
    if (cid < 0)
    {
      throw std::invalid_argument("Invalid column ID");
    }

    SqlStatement stmt = db.get().prepStatement("SELECT name FROM pragma_table_info(?) WHERE cid=?");
    stmt.bind(1, tabName);
    stmt.bind(2, cid);

    stmt.step();
    if (!stmt.hasData())
    {
      throw std::invalid_argument("Invalid column ID");
    }

    return stmt.get<std::string>(0);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::name2cid(const string& colName) const
  {
    if (colName.empty())
    {
      throw std::invalid_argument("Invalid column name");
    }

    SqlStatement stmt = db.get().prepStatement("SELECT cid FROM pragma_table_info(?) WHERE name=?");
    stmt.bind(1, tabName);
    stmt.bind(2, colName);

    stmt.step();
    if (!stmt.hasData())
    {
      throw std::invalid_argument("Invalid column name");
    }

    return stmt.get<int>(0);
  }

//----------------------------------------------------------------------------

  bool CommonTabularClass::hasColumn(const string& colName) const
  {
    try
    {
      name2cid(colName);
    }
    catch (std::invalid_argument& e)
    {
      return false;
    }

    return true;
  }

//----------------------------------------------------------------------------

  bool CommonTabularClass::hasColumn(int cid) const
  {
    SqlStatement stmt = db.get().prepStatement("SELECT count(*) FROM pragma_table_info(?) WHERE cid=?");
    stmt.bind(1, tabName);
    stmt.bind(2, cid);

    return (db.get().execScalarQueryInt(stmt) > 0);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForWhereClause(const WhereClause& w) const
  {
    if (w.isEmpty())
    {
      throw std::invalid_argument("Match count for empty where clause");
    }
    SqlStatement stmt = w.getSelectStmt(db, tabName, true);
    return db.get().execScalarQueryInt(stmt);
  }

//----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForColumnValueNull(const string& col) const
  {
    if (col.empty())
    {
      throw std::invalid_argument("getMatchCountForColumnValue(): empty column name");
    }

    string sql = sqlColumnCount + col + " IS NULL";

    try
    {
      SqlStatement stmt = db.get().prepStatement(sql);
      stmt.step();
      return stmt.get<int>(0);
    }
    catch (SqlStatementCreationError e)
    {
      throw std::invalid_argument("getMatchCountForColumnValueNull(): invalid column name");
    }
  }

  //----------------------------------------------------------------------------

  int CommonTabularClass::getMatchCountForWhereClause(const string& where) const
  {
    if (where.empty())
    {
      throw std::invalid_argument("Empty string for match count by WHERE clause");
    }

    //string sql = "SELECT COUNT(*) FROM " + tabName + " WHERE " + where;
    string sql = sqlColumnCount + where;

    return db.get().execScalarQueryInt(sql);
  }


//----------------------------------------------------------------------------

  int CommonTabularClass::length() const
  {
    return db.get().execScalarQueryInt("SELECT COUNT(*) FROM " + tabName);
  }

  //----------------------------------------------------------------------------

  Sloppy::CSV_Table CommonTabularClass::toCSV(bool includeHeaders) const
  {
    const string sql = "SELECT * FROM " + tabName;
    auto stmt = db.get().prepStatement(sql);
    return stmt.toCSV(includeHeaders);
  }

  //----------------------------------------------------------------------------

  Sloppy::CSV_Table CommonTabularClass::toCSV(std::vector<string> colNames, WhereClause w, bool includeHeaders) const
  {
    Sloppy::estring sql{"SELECT "};
    if (colNames.empty())
    {
      // get all columns in case of an empty column list
      sql += "*";
    } else {
      // create a comma-separated list of column names
      auto it = colNames.cbegin();
      sql += *it;
      ++it;
      for (; it != colNames.cend(); ++it)
      {
        sql += "," + *it;
      }
    }

    sql += " FROM " + tabName;

    SqlStatement stmt;
    if (!w.isEmpty())
    {
      // create a statement with WHERE clause
      sql += " WHERE " + w.getWherePartWithPlaceholders(true);
      stmt = w.createStatementAndBindValuesToPlaceholders(db.get(), sql);
    } else {
      // create a plain statement that retrieves all rows
      stmt = db.get().prepStatement(sql);
    }

    return stmt.toCSV(includeHeaders);
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
