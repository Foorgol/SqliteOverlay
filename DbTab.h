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

#ifndef SQLITE_OVERLAY_DBTAB_H
#define	SQLITE_OVERLAY_DBTAB_H

#include "CommonTabularClass.h"
#include "ClausesAndQueries.h"


namespace SqliteOverlay
{
  class TabRow;

  class DbTab : public CommonTabularClass
  {
    friend class SqliteDatabase;

  public:
    class CachingRowIterator
    {
      friend class DbTab;

    public:
      void operator++();
      TabRow operator*() const;
      bool isEnd() const;
      bool pointsToElement() const;
      bool isEmpty() const;
      int length() const;
      double getPercentage() const;
      
    private:
      CachingRowIterator(SqliteDatabase* db, const string& tabName, upSqlStatement stmt);
      CachingRowIterator();
      vector<int> idList;
      SqliteDatabase* db;
      string tabName;
      int curIdx;
      int cachedLength;
    };
    
  public:
    virtual ~DbTab ();
    int insertRow(const ColumnValueClause& ic, int* errCodeOut=nullptr);
    int insertRow(int* errCodeOut=nullptr);
    TabRow operator[](const int id) const;
    TabRow operator[](const WhereClause& w ) const;
    TabRow getSingleRowByColumnValue(const string& col, int val) const;
    TabRow getSingleRowByColumnValue(const string& col, double val) const;
    TabRow getSingleRowByColumnValue(const string& col, const string& val) const;
    TabRow getSingleRowByColumnValueNull(const string& col) const;
    TabRow getSingleRowByWhereClause(const WhereClause& w) const;
    TabRow getSingleRowByWhereClause(const string& w) const;
    CachingRowIterator getRowsByWhereClause(const WhereClause& w) const;
    CachingRowIterator getRowsByWhereClause(const string& w, bool isWhereClauseOnly=true) const;
    CachingRowIterator getRowsByColumnValue(const string& col, int val) const;
    CachingRowIterator getRowsByColumnValue(const string& col, double val) const;
    CachingRowIterator getRowsByColumnValue(const string& col, const string& val) const;
    CachingRowIterator getRowsByColumnValueNull(const string& col) const;
    CachingRowIterator getAllRows() const;
    int deleteRowsByWhereClause(const WhereClause& where, int* errCodeOut=nullptr) const;
    int deleteRowsByColumnValue(const string& col, const int val, int* errCodeOut=nullptr) const;
    int deleteRowsByColumnValue(const string& col, const double val, int* errCodeOut=nullptr) const;
    int deleteRowsByColumnValue(const string& col, const string& val, int* errCodeOut=nullptr) const;

    bool hasRowId(int id) const;

  private:
    DbTab (SqliteDatabase* db, const string& tabName);
  };

}

#endif	/* DBTAB_H */

