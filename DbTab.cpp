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

  int DbTab::insertRow(const ColumnValueClause& ic, int* errCodeOut)
  {
    string sql = ic.getInsertStmt(tabName);
    bool isOk = db->execNonQuery(sql, errCodeOut);
    if (!isOk)
    {
      // insert failed
      return -1;
    }

    // insert succeeded
    return db->getLastInsertId();
  }

  //----------------------------------------------------------------------------

  int DbTab::insertRow(int* errCodeOut)
  {
    ColumnValueClause empty;

    return insertRow(empty, errCodeOut);
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

    cachedLength = idList.size();
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator::CachingRowIterator()
    : db(nullptr), tabName("")
  {
    curIdx = -1;
    cachedLength = 0;
  }

  //----------------------------------------------------------------------------

  bool DbTab::CachingRowIterator::isEnd() const
  {
    return (curIdx  == cachedLength);
  }

  //----------------------------------------------------------------------------

  bool DbTab::CachingRowIterator::pointsToElement() const
  {
    return (curIdx < cachedLength);
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

    if (curIdx > cachedLength)
    {
      curIdx = cachedLength;
    }
  }

  //----------------------------------------------------------------------------

  TabRow DbTab::CachingRowIterator::operator *() const
  {
    if (curIdx >= cachedLength)
    {
      throw std::runtime_error("Access past the last item of the CachingRowIterator");
    }
    int id = idList.at(curIdx);
    return TabRow(db, tabName, id, true);
  }

  //----------------------------------------------------------------------------

  int DbTab::CachingRowIterator::length() const
  {
    return cachedLength;
  }

  //----------------------------------------------------------------------------

  double DbTab::CachingRowIterator::getPercentage() const
  {
    if (cachedLength == 0)
    {
      return -1;
    }
    if ((cachedLength == 1) && (curIdx >= 0))
    {
      return 1.0;
    }
    return curIdx / (cachedLength -1.0);
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByWhereClause(const WhereClause& w) const
  {
    string sql = w.getSelectStmt(tabName, false);

    return getRowsByWhereClause(sql, false);
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByWhereClause(const string& w, bool isWhereClauseOnly) const
  {
    string sql = w;
    if (isWhereClauseOnly)
    {
      sql = "SELECT id FROM " + tabName + " WHERE " + w;
    }

    auto stmt = db->prepStatement(sql, nullptr);

    // in case of errors, return an empty CachingRowIterator
    if (stmt == nullptr)
    {
      throw std::invalid_argument("getRowsByWhereClause: invalid query!");
    }
    if (!(stmt->step()))
    {
      return CachingRowIterator();
    }

    return DbTab::CachingRowIterator(db, tabName, std::move(stmt));
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByColumnValue(const string& col, int val) const
  {
    WhereClause w;
    w.addIntCol(col, val);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByColumnValue(const string& col, double val) const
  {
    WhereClause w;
    w.addDoubleCol(col, val);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByColumnValue(const string& col, const string& val) const
  {
    WhereClause w;
    w.addStringCol(col, val);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByColumnValueNull(const string& col) const
  {
    WhereClause w;
    w.addNullCol(col);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getAllRows() const
  {
    string sql = "SELECT id FROM " + tabName;
    auto stmt = db->prepStatement(sql, nullptr);

    // this sql query should always succeed... but test the result anyway...
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

  TabRow DbTab::getSingleRowByWhereClause(const string& w) const
  {
    string sql = "SELECT id FROM " + tabName + " WHERE " + w + " LIMIT 1";

    auto stmt = db->prepStatement(sql, nullptr);

    // in case of errors, throw an error
    if (stmt == nullptr)
    {
      throw std::invalid_argument("getRowsByWhereClause: invalid query!");
    }
    if (!(stmt->step()))
    {
      throw std::invalid_argument("getSingleRowByWhereClause: no matches!");
    }

    int rowId;
    if (!(stmt->getInt(0, &rowId)))
    {
      throw std::invalid_argument("Unexpected database error!");
    }

    return TabRow(db, tabName, rowId, true);
  }

  //----------------------------------------------------------------------------

  int DbTab::deleteRowsByWhereClause(const WhereClause& where, int* errCodeOut) const
  {
    string sql = where.getDeleteStmt(tabName);

    bool isOk = db->execNonQuery(sql, errCodeOut);
    if (!isOk)
    {
      // delete failed
      return -1;
    }

    // delete succeeded
    return db->getRowsAffected();
  }

  //----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const string& col, const int val, int* errCodeOut) const
  {
    WhereClause w;
    w.addIntCol(col, val);
    return deleteRowsByWhereClause(w, errCodeOut);
  }

  //----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const string& col, const double val, int* errCodeOut) const
  {
    WhereClause w;
    w.addDoubleCol(col, val);
    return deleteRowsByWhereClause(w, errCodeOut);
  }

  //----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const string& col, const string& val, int* errCodeOut) const
  {
    WhereClause w;
    w.addStringCol(col, val);
    return deleteRowsByWhereClause(w, errCodeOut);
  }

  //----------------------------------------------------------------------------

  int DbTab::clear(int* errCodeOut) const
  {
    string sql = "DELETE FROM " + tabName;

    bool isOk = db->execNonQuery(sql, errCodeOut);
    if (!isOk)
    {
      // delete failed
      return -1;
    }

    // delete succeeded
    return db->getRowsAffected();
  }

  //----------------------------------------------------------------------------

  bool DbTab::hasRowId(int id) const
  {
    string sql = "SELECT id FROM " + tabName + " WHERE id = " + to_string(id);
    auto stmt = db->prepStatement(sql, nullptr);

    // this sql query should always succeed... but test the result anyway...
    if (stmt == nullptr)
    {
      throw std::runtime_error("Weird... could query single row");
    }
    if (!(stmt->step()))
    {
      throw std::runtime_error("Weird... could step into a query for a single row");
    }

    return stmt->hasData();
  }

  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------

}
