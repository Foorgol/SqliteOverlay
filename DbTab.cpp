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

  DbTab::CachingRowIterator::CachingRowIterator(SqliteDatabase* _db, const string& _tabName, upSqlStatement stmt)
  : db(_db), tabName(_tabName)
  {
    // no checks for the validity of _db and _tabName here, because
    // we assume that the constructor is only called internally with
    // pre-checked values

    idList.clear();
    curIdx = 0;
    cachedLength = 0;

    // make sure that the query is valid
    //
    // IMPORTANT: we assume that the caller has executed the first step() call
    // on the statement!
    if (!(stmt->hasData()))
    {
      return; // empty table or invalid query... empty table is more likely... return, not throw
    }
    if (stmt->getColName(0) != "id")
    {
      throw std::invalid_argument("CachingRowIterator: invalid SELECT query provided to constructor");
    }

    // iterate over all matches/results in the query and cache their row IDs
    do
    {
      int id;
      stmt->getInt(0, &id);
      idList.push_back(id);
      stmt->step();
    } while (!(stmt->isDone()));

    curIdx = 0;
    cachedLength = idList.size();
  }


//----------------------------------------------------------------------------

  bool DbTab::CachingRowIterator::hasMore() const
  {
    return (curIdx < (cachedLength-1));
  }

//----------------------------------------------------------------------------

  bool DbTab::CachingRowIterator::isEmpty() const
  {
    return (cachedLength == 0);
  }

//----------------------------------------------------------------------------

  void DbTab::CachingRowIterator::operator ++()
  {
    ++curIdx;

    if (curIdx >= cachedLength)
    {
      throw std::runtime_error("Iterated past the last item of the CachingRowIterator");
    }
  }

//----------------------------------------------------------------------------

  TabRow DbTab::CachingRowIterator::operator *() const
  {
    if (curIdx >= cachedLength)
    {
      throw std::runtime_error("Iterated past the last item of the CachingRowIterator");
    }
    int id = idList.at(curIdx);
    return TabRow(db, tabName, id, true);
  }

////----------------------------------------------------------------------------

  int DbTab::CachingRowIterator::length() const
  {
    return cachedLength;
  }

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

//----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getAllRows() const
  {
    string sql = "SELECT id FROM " + tabName;
    auto stmt = db->prepStatement(sql);

    // this sql query should always succeed... but the test result anyway...
    if (stmt == nullptr)
    {
      throw std::runtime_error("Weird... could query all rows");
    }
    if (!(stmt->step()))
    {
      throw std::runtime_error("Weird... could step into a query of all rows");
    }

    return DbTab::CachingRowIterator(db, tabName, std::move(stmt));
  }

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
