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

#ifndef DBTAB_H
#define	DBTAB_H

#include "CommonTabularClass.h"
#include "HelperFunc.h"
#include "ClausesAndQueries.h"


namespace SqliteOverlay
{
  class TabRow;

  class DbTab : public CommonTabularClass
  {
    friend class SqliteDatabase;

  public:
//    class CachingRowIterator
//    {
//    public:
//      CachingRowIterator(SqliteDatabase* db, const string& tabName, QSqlQuery& qry);
//      void operator++();
//      TabRow operator*() const;
//      bool isEnd() const;
//      bool isValid() const;
//      int length() const;
      
//    private:
//      QList<int> idList;
//      GenericDatabase* db;
//      QString tabName;
//      QList<int>::const_iterator listIter;
//    };
    
  public:
    virtual ~DbTab ();
    int insertRow(const ColumnValueClause& ic);
    int insertRow();
    TabRow operator[](const int id) const;
    TabRow operator[](const WhereClause& w ) const;
    TabRow getSingleRowByColumnValue(const string& col, int val) const;
    TabRow getSingleRowByColumnValue(const string& col, double val) const;
    TabRow getSingleRowByColumnValue(const string& col, const string& val) const;
    TabRow getSingleRowByColumnValueNull(const string& col) const;
    TabRow getSingleRowByWhereClause(const WhereClause& w) const;
//    CachingRowIterator getRowsByWhereClause(const QString& where, const QVariantList& args = QVariantList()) const;
//    CachingRowIterator getRowsByColumnValue(const QVariantList& args = QVariantList()) const;
//    CachingRowIterator getRowsByColumnValue(const QString& col, const QVariant& val) const;
//    CachingRowIterator getAllRows() const;
    int deleteRowsByWhereClause(const WhereClause& where) const;
    int deleteRowsByColumnValue(const string& col, const int val) const;
    int deleteRowsByColumnValue(const string& col, const double val) const;
    int deleteRowsByColumnValue(const string& col, const string& val) const;

  private:
    DbTab (SqliteDatabase* db, const string& tabName);
  };

}

#endif	/* DBTAB_H */

