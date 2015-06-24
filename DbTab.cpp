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

#include <memory>

#include "DbTab.h"
#include "HelperFunc.h"
#include "TabRow.h"

namespace SqliteOverlay
{

  DbTab::~DbTab()
  {
  }

//----------------------------------------------------------------------------

  DbTab::DbTab(SqliteDatabase* _db, const string& _tabName)
  : CommonTabularClass (_db, _tabName, false)
  {

  }

//----------------------------------------------------------------------------

  int DbTab::insertRow(const ColumnValueClause& ic)
  {
    string sql = ic.getInsertStmt(tabName);
    bool isOk = db->execNonQuery(sql);
    if (!isOk)
    {
      // insert failed
      return -1;
    }

    // insert succeeded
    return db->getLastInsertId();
  }

//----------------------------------------------------------------------------

  int DbTab::insertRow()
  {
    ColumnValueClause empty;

    return insertRow(empty);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::operator [](const int id) const
  {
    return TabRow(db, tabName, id);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::operator [](const WhereClause& w) const
  {
    return TabRow(db, tabName, w);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByColumnValue(const string& col, int val) const
  {
    WhereClause w;
    w.addIntCol(col, val);
    return TabRow(db, tabName, w);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByColumnValue(const string& col, double val) const
  {
    WhereClause w;
    w.addDoubleCol(col, val);
    return TabRow(db, tabName, w);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByColumnValue(const string& col, const string& val) const
  {
    WhereClause w;
    w.addStringCol(col, val);
    return TabRow(db, tabName, w);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByColumnValueNull(const string& col) const
  {
    WhereClause w;
    w.addNullCol(col);
    return TabRow(db, tabName, w);
  }

//----------------------------------------------------------------------------

//  DbTab::CachingRowIterator::CachingRowIterator(GenericDatabase* _db, const QString& _tabName, QSqlQuery& qry)
//  : db(_db), tabName(_tabName)
//  {
//    // no checks for the validity of _db and _tabName here, because
//    // we assume that the constructor is only called internally with
//    // pre-checked values
//    idList = QList<int>();
    
//    // make sure that the query is valid
//    if (!(qry.isActive()))
//    {
//      throw std::invalid_argument("CachingRowIterator: inactive query provided to constructor");
//    }
//    if (!(qry.isSelect()))
//    {
//      throw std::invalid_argument("CachingRowIterator: non-SELECT query provided to constructor");
//    }
    
//    // iterate over all matches/results in the query and cache their row IDs
//    if (qry.first())
//    {
//      do
//      {
//        idList << qry.value(0).toInt();  // we implicitly assume that column 0 is the ID column!
//      }  while (qry.next());
//    }
    
//    qry.finish();
//    listIter = idList.begin();
//  }


////----------------------------------------------------------------------------

//  bool DbTab::CachingRowIterator::isEnd() const
//  {
//    return (listIter == idList.end());
//  }

////----------------------------------------------------------------------------

//  bool DbTab::CachingRowIterator::isValid() const
//  {
//    return (idList.length() != 0);
//  }

////----------------------------------------------------------------------------

//  void DbTab::CachingRowIterator::operator ++()
//  {
//    listIter++;
//  }

////----------------------------------------------------------------------------

//  TabRow DbTab::CachingRowIterator::operator *() const
//  {
//    int id = *listIter;
    
//    return TabRow(db, tabName, id, true);
//  }

////----------------------------------------------------------------------------

//  int DbTab::CachingRowIterator::length() const
//  {
//    return idList.length();
//  }

////----------------------------------------------------------------------------

//  DbTab::CachingRowIterator DbTab::getRowsByWhereClause(const QString& where, const QVariantList& args) const
//  {
//    QString sql = "SELECT id FROM " + tabName + " WHERE " + where;
//    unique_ptr<QSqlQuery> qry = unique_ptr<QSqlQuery>(db->execContentQuery(sql, args));
//    if (qry == NULL) {
//      throw std::invalid_argument("getRowsByWhereClause: invalid query!");
//    }
    
//    DbTab::CachingRowIterator result = DbTab::CachingRowIterator(db, tabName, *qry);
//    return result;
//  }

////----------------------------------------------------------------------------

//  DbTab::CachingRowIterator DbTab::getRowsByColumnValue(const QVariantList& args) const
//  {
//    QVariantList qvl;
    
//    qvl = prepWhereClause(args);  // don't catch exception, forward them to caller
    
//    QString sql = "SELECT id FROM " + tabName + " WHERE " + qvl.at(0).toString();
//    qvl.removeFirst();
//    unique_ptr<QSqlQuery> qry = unique_ptr<QSqlQuery>(db->execContentQuery(sql, qvl));
//    if (qry == NULL) {
//      throw std::invalid_argument("getRowsByColumnValue: invalid query!");
//    }
    
//    DbTab::CachingRowIterator result = DbTab::CachingRowIterator(db, tabName, *qry);
//    return result;
//  }

////----------------------------------------------------------------------------

//  DbTab::CachingRowIterator DbTab::getRowsByColumnValue(const QString& col, const QVariant& val) const
//  {
//    QVariantList qvl;
//    qvl << col << val;

//    return getRowsByColumnValue(qvl);
//  }

////----------------------------------------------------------------------------

//  DbTab::CachingRowIterator DbTab::getAllRows() const
//  {
//    QString sql = "SELECT id FROM " + tabName;
//    unique_ptr<QSqlQuery> qry = unique_ptr<QSqlQuery>(db->execContentQuery(sql));
//    if (qry == NULL) {
//      throw std::invalid_argument("getAllRows: invalid table, has no ID column!");
//    }
    
//    DbTab::CachingRowIterator result = DbTab::CachingRowIterator(db, tabName, *qry);
//    return result;
//  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByWhereClause(const WhereClause& w) const
  {
    return TabRow(db, tabName, w);
  }

//----------------------------------------------------------------------------

  int DbTab::deleteRowsByWhereClause(const WhereClause& where) const
  {
    string sql = where.getDeleteStmt(tabName);

    bool isOk = db->execNonQuery(sql);
    if (!isOk)
    {
      // delete failed
      return -1;
    }

    // delete succeeded
    return db->getRowsAffected();
  }

//----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const string& col, const int val) const
  {
    WhereClause w;
    w.addIntCol(col, val);
    return deleteRowsByWhereClause(w);
  }

//----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const string& col, const double val) const
  {
    WhereClause w;
    w.addDoubleCol(col, val);
    return deleteRowsByWhereClause(w);
  }

//----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const string& col, const string& val) const
  {
    WhereClause w;
    w.addStringCol(col, val);
    return deleteRowsByWhereClause(w);
  }

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

}
