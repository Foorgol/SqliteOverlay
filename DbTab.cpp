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

  DbTab::DbTab(const SqliteDatabase& _db, const string& _tabName, bool forceNameCheck)
    : CommonTabularClass (_db, _tabName, false, forceNameCheck),
      cachedSelectSql{"SELECT rowid FROM " + tabName + " WHERE "}
  {

  }

  //----------------------------------------------------------------------------

  int DbTab::insertRow(const ColumnValueClause& ic) const
  {
    SqlStatement stmt = ic.getInsertStmt(db, tabName);
    db.get().execNonQuery(stmt);

    // insert succeeded, otherwise the commands
    // above would have thrown an exception
    return db.get().getLastInsertId();
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

  optional<TabRow> DbTab::get2(const int id) const
  {
    if (!hasRowId(id)) return optional<TabRow>{};
    return TabRow(db, tabName, id, true);
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
    try
    {
      int id = db.get().execScalarQueryInt(cachedSelectSql + col + " IS NULL");
      return TabRow(db, tabName, id, true);
    }
    catch (SqlStatementCreationError)
    {
      throw NoDataException();
    }
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
    if (w.isEmpty())
    {
      throw std::invalid_argument("getRowsByWhereClause() called with empty WHERE");
    }

    try
    {
      auto stmt = w.getSelectStmt(db, tabName, false);
      return statementResultsToVector(stmt);
    }
    catch (SqlStatementCreationError)
    {
      return vector<TabRow>{};
    }
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getRowsByWhereClause(const string& w) const
  {
    if (w.empty())
    {
      throw std::invalid_argument("getRowsByWhereClause() called with empty WHERE");
    }

    try
    {
      auto stmt = db.get().prepStatement(cachedSelectSql + w);
      return statementResultsToVector(stmt);
    }
    catch (SqlStatementCreationError)
    {
      return vector<TabRow>{};
    }
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getRowsByColumnValueNull(const string& col) const
  {
    if (col.empty())
    {
      throw std::invalid_argument("getRowsByColumnValueNull() called with empty colum name");
    }

    try
    {
      SqlStatement stmt = db.get().prepStatement(cachedSelectSql + col + " IS NULL");
      return statementResultsToVector(stmt);
    }
    catch (SqlStatementCreationError)
    {
      return vector<TabRow>{};
    }
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::getAllRows() const
  {
    string sql = "SELECT rowid FROM " + tabName;
    auto stmt = db.get().prepStatement(sql);

    return statementResultsToVector(stmt);
  }

  //----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByWhereClause(const WhereClause& w) const
  {
    return TabRow(db, tabName, w);
  }

  //----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByWhereClause(const string& w) const
  {
    string sql = cachedSelectSql + w + " LIMIT 1";

    auto stmt = db.get().prepStatement(sql);
    stmt.step();
    if (!(stmt.hasData()))
    {
      throw NoDataException();
    }

    return TabRow(db, tabName, stmt.getInt(0), true);
  }

  //----------------------------------------------------------------------------

  int DbTab::deleteRowsByWhereClause(const WhereClause& where) const
  {
    if (where.isEmpty())
    {
      throw std::invalid_argument("deleteRowsByWhereClause(): called with empty WHERE");
    }

    auto stmt = where.getDeleteStmt(db, tabName);
    db.get().execNonQuery(stmt);

    // delete succeeded
    return db.get().getRowsAffected();
  }

  //----------------------------------------------------------------------------

  int DbTab::clear() const
  {
    SqlStatement stmt = db.get().prepStatement("DELETE FROM " + tabName);

    db.get().execNonQuery(stmt);

    // delete succeeded
    return db.get().getRowsAffected();
  }

  //----------------------------------------------------------------------------

  bool DbTab::hasRowId(int id) const
  {
    SqlStatement stmt = db.get().prepStatement(cachedSelectSql + " rowid=" + to_string(id));
    stmt.step();

    return stmt.hasData();
  }

  //----------------------------------------------------------------------------

  vector<int> DbTab::checkConstraint(const string& colName, Sloppy::ValueConstraint c, int firstRowId) const
  {
    vector<int> result;
    Sloppy::estring sql = "SELECT rowid,%1 FROM %2 WHERE rowid >= %3 ORDER BY rowid ASC";
    sql.arg(colName);
    sql.arg(tabName);
    sql.arg(firstRowId);

    SqlStatement stmt = db.get().prepStatement(sql);

    for (stmt.step() ; stmt.hasData() ; stmt.step())
    {
      optional<string> val;
      try
      {
        val = stmt.getString(1);
      }
      catch (NullValueException)
      {
        val.reset();
      }

      bool isOkay = Sloppy::checkConstraint(val, c);
      if (!isOkay)
      {
        result.push_back(stmt.getInt(0));
      }
    }

    return result;
  }

  //----------------------------------------------------------------------------

  void DbTab::addColumn_exec(const string& colName, ColumnDataType colType, const string& constraints) const
  {
    string cn{colName};
    boost::trim(cn);
    if (cn.empty())
    {
      throw std::invalid_argument("Called with empty column name");
    }

    string ts{};
    switch (colType)
    {
    case ColumnDataType::Integer:
      ts = "INTEGER";
      break;

    case ColumnDataType::Float:
      ts = "FLOAT";
      break;

    case ColumnDataType::Text:
      ts = "TEXT";
      break;

    case ColumnDataType::Blob:
      ts = "BLOB";
      break;
    }
    // no checking for empty "ts" here, because SQLite allows
    // column declarations with empty type

    //
    // we skip any consistency checks here; this function is private and it's assumed
    // that any checks have been performed before by the associated public functions
    //

    // construct an "alter table" sql query for inserting the column
    string sql = "ALTER TABLE " + tabName + " ADD COLUMN " + cn + " " + ts;

    // add any constraints
    if (!(constraints.empty())) sql += " " + constraints;

    // actually add the column
    SqlStatement stmt = db.get().prepStatement(sql);
    stmt.step();
  }

  //----------------------------------------------------------------------------

  vector<TabRow> DbTab::statementResultsToVector(SqlStatement& stmt) const
  {
    vector<TabRow> result;

    stmt.step();
    while (stmt.hasData())
    {
      result.emplace_back(db, tabName, stmt.getInt(0), true);

      stmt.step();
    }

    return result;
  }

  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------

}
