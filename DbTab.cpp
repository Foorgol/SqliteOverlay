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

#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "DbTab.h"
#include "TabRow.h"

namespace SqliteOverlay
{

  DbTab::DbTab(SqliteDatabase* _db, const string& _tabName, bool forceNameCheck)
    : CommonTabularClass (_db, _tabName, false, forceNameCheck),
      cachedSelectSql{"SELECT id FROM " + tabName + " WHERE %1=?"}
  {

  }

  //----------------------------------------------------------------------------

  DbTab::~DbTab()
  {
  }

  //----------------------------------------------------------------------------

  int DbTab::insertRow(const ColumnValueClause& ic) const
  {
    SqlStatement stmt = ic.getInsertStmt(db, tabName);
    db->execNonQuery(stmt);

    // insert succeeded, otherwise the commands
    // above would have thrown an exception
    return db->getLastInsertId();
  }

  //----------------------------------------------------------------------------

  int DbTab::insertRow() const
  {
    ColumnValueClause empty;

    return insertRow(empty);
  }

  //----------------------------------------------------------------------------

  TabRow DbTab::operator [](const int id) const
  {
    return TabRow(db, tabName, id, true);
  }

  //----------------------------------------------------------------------------

  TabRow DbTab::operator [](const WhereClause& w) const
  {
    return TabRow(db, tabName, w);
  }

  //----------------------------------------------------------------------------

  optional<TabRow> DbTab::get2(int id) const
  {
    try
    {
      TabRow r{db, tabName, id, false};

      // if we survived this, the row exists
      return optional<TabRow>{r};
    }
    catch(std::invalid_argument& e)
    {
      // the row ID was invalid
      return optional<TabRow>{};
    }
    catch (...)
    {
      // rethrow any other exception
      throw;
    }

    return optional<TabRow>{};   // we should never reach this point...
  }

  //----------------------------------------------------------------------------

  optional<TabRow> DbTab::get2(const WhereClause& w) const
  {
    try
    {
      TabRow r{db, tabName, w};

      // if we survived this, the row exists
      return r;
    }
    catch(std::invalid_argument& e)
    {
      // the row ID was invalid
      return optional<TabRow>{};
    }
    catch (...)
    {
      // rethrow any other exception
      throw;
    }

    return optional<TabRow>{};   // we should never reach this point...
  }

  //----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByColumnValueNull(const string& col) const
  {
    int id = db->execScalarQueryInt("SELECT id FROM " + tabName + " WHERE " + col + " IS NULL");
    return TabRow(db, tabName, id, true);
  }

  //----------------------------------------------------------------------------

  optional<TabRow> DbTab::getSingleRowByColumnValueNull2(const string& col) const
  {
    try
    {
      // try a regular lookup and let the compiler
      // pick the best function (int, double, string, ...)
      return getSingleRowByColumnValueNull(col);
    }
    catch (NoDataException& e)
    {
      // return an empty optional if no such column exists
      return optional<TabRow>{};
    }
    catch (...)
    {
      // rethrow any other exceptio
      throw;
    }

    return optional<TabRow>{};   // we should never reach this point...
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getRowsByWhereClause(const WhereClause& w) const
  {
    auto stmt = w.getSelectStmt(db, tabName, false);

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

  vector<TabRow> DbTab::getRowsByWhereClause(const string& w, bool isWhereClauseOnly) const
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

  vector<TabRow> DbTab::getRowsByColumnValue(const string& col, int val) const
  {
    WhereClause w;
    w.addIntCol(col, val);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getRowsByColumnValue(const string& col, double val) const
  {
    WhereClause w;
    w.addDoubleCol(col, val);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getRowsByColumnValue(const string& col, const string& val) const
  {
    WhereClause w;
    w.addStringCol(col, val);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getRowsByColumnValueNull(const string& col) const
  {
    WhereClause w;
    w.addNullCol(col);
    return getRowsByWhereClause(w);
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getAllRows() const
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
    auto stmt = where.getDeleteStmt(db, tabName);
    if (stmt == nullptr) return -1;

    bool isOk = db->execNonQuery(stmt, errCodeOut);
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

  bool DbTab::addColumn(const string& colName, const string& typeStr, const string& constraints)
  {
    string cn{colName};
    boost::trim(cn);
    if (cn.empty()) return false;

    string ts{typeStr};
    boost::trim(ts);
    if (ts.empty()) return false;

    //
    // we skip any consistency checks here; this function is private and it's assumed
    // that any checks have been performed before by the associated public functions
    //

    // construct an "alter table" sql query for inserting the column
    string sql = "ALTER TABLE " + tabName + " ADD COLUMN " + cn + " " + ts;

    // add any constraints
    if (!(constraints.empty())) sql += " " + constraints;

    // actually add the column
    int dbErr;
    bool isOk = db->execNonQuery(sql, &dbErr);
    return (isOk && (dbErr == SQLITE_DONE));
  }

  //----------------------------------------------------------------------------

  bool DbTab::addColumn(const string& colName, const string& typeStr, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue, bool returnErrorIfExists)
  {
    // check if the column exists
    if (hasColumn(colName))
    {
      return !returnErrorIfExists;
    }

    // if "not null" is requested, a default value is mandatory
    if (notNull && !hasDefault) return false;

    string constraints = buildColumnConstraint(false, CONFLICT_CLAUSE::__NOT_SET, notNull, notNullConflictClause,
                                               hasDefault, defaultValue);
    return addColumn(colName, typeStr, constraints);
  }

  //----------------------------------------------------------------------------

  bool DbTab::addColumn_Varchar(const string& colName, int len, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue, bool returnErrorIfExists)
  {
    string tName = "VARCHAR(" + to_string(len) + ")";
    return addColumn(colName, tName, notNull, notNullConflictClause, hasDefault, defaultValue, returnErrorIfExists);
  }

  //----------------------------------------------------------------------------

  bool DbTab::addColumn_int(const string& colName, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, int defaultValue, bool returnErrorIfExists)
  {
    return addColumn(colName, "INTEGER", notNull, notNullConflictClause, hasDefault, to_string(defaultValue), returnErrorIfExists);
  }

  //----------------------------------------------------------------------------

  bool DbTab::addColumn_text(const string& colName, bool notNull, CONFLICT_CLAUSE notNullConflictClause, bool hasDefault, const string& defaultValue, bool returnErrorIfExists)
  {
    return addColumn(colName, "TEXT", notNull, notNullConflictClause, hasDefault, defaultValue, returnErrorIfExists);
  }

  //----------------------------------------------------------------------------

  bool DbTab::addColumn_foreignKey(const string& colName, const string& referedTabName, CONSISTENCY_ACTION onDelete, CONSISTENCY_ACTION onUpdate, bool returnErrorIfExists)
  {
    // check if the column exists
    if (hasColumn(colName))
    {
      return !returnErrorIfExists;
    }

    // build a foreign key constraint
    string constraints = buildForeignKeyClause(referedTabName, onDelete, onUpdate);

    return addColumn(colName, "INTEGER", constraints);
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

  SqlStatement DbTab::getSelectStmtForColumn(const string& colName) const
  {
    Sloppy::estring sql{cachedSelectSql};
    sql.arg(colName);
    return db->prepStatement(sql);
  }

  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------

}
