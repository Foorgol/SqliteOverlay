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

#include <stdint.h>         // for int64_t
#include <algorithm>        // for max
#include <cstddef>          // for size_t, std
#include <memory>           // for unique_ptr, make_unique

#include <Sloppy/CSV.h>     // for CSV_Table, CSV_Value, CSV_Value::Type
#include <Sloppy/String.h>  // for estring
#include <Sloppy/Utils.h>   // for trimAndCheckString

#include "TabRow.h"         // for TabRow
#include "Transaction.h"    // for Transaction
#include "DbTab.h"

using namespace std;

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
    return getSingleRowByWhereClause2(w);
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
      throw;
    }
  }

  //----------------------------------------------------------------------------

  optional<TabRow> DbTab::getSingleRowByColumnValueNull2(const string& col) const
  {
    //
    // No error handling here... BUSY etc. should be handled by the caller
    //

    auto stmt = db.get().prepStatement(cachedSelectSql + col + " IS NULL");
    stmt.step();
    if (!stmt.hasData()) return optional<TabRow>{};
    return TabRow(db, tabName, stmt.get<int>(0), true);
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
    try
    {
      return TabRow(db, tabName, w);
    }
    catch (std::invalid_argument&)
    {
      throw NoDataException();
    }
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

    return TabRow(db, tabName, stmt.get<int>(0), true);
  }

  //----------------------------------------------------------------------------

  optional<TabRow> DbTab::getSingleRowByWhereClause2(const WhereClause& w) const
  {
    auto stmt = w.getSelectStmt(db.get(), tabName, false);
    stmt.step();
    if (!(stmt.hasData()))
    {
      return optional<TabRow>{};
    }

    return TabRow(db, tabName, stmt.get<int>(0), true);
  }

  //----------------------------------------------------------------------------

  optional<TabRow> DbTab::getSingleRowByWhereClause2(const string& w) const
  {
    string sql = cachedSelectSql + w + " LIMIT 1";

    auto stmt = db.get().prepStatement(sql);
    stmt.step();
    if (!(stmt.hasData()))
    {
      return optional<TabRow>{};
    }

    return TabRow(db, tabName, stmt.get<int>(0), true);
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
        val = stmt.get<std::string>(1);
      }
      catch (NullValueException)
      {
        val.reset();
      }

      bool isOkay = Sloppy::checkConstraint(val, c);
      if (!isOkay)
      {
        result.push_back(stmt.get<int>(0));
      }
    }

    return result;
  }

  //----------------------------------------------------------------------------

  TabRowIterator DbTab::tabRowIterator(int minRowId, int maxRowId)
  {
    return TabRowIterator(db.get(), tabName, minRowId, maxRowId);
  }

  //----------------------------------------------------------------------------

  TabRowIterator DbTab::tabRowIterator(const WhereClause& w)
  {
    return TabRowIterator(db.get(), tabName, w);
  }

  //----------------------------------------------------------------------------

  int DbTab::importCSV(const Sloppy::CSV_Table& csvTab, TransactionType tt) const
  {
    if (csvTab.empty()) return 0;

    if (!csvTab.hasHeaders())
    {
      throw std::invalid_argument{"DbTab::importCSV(): called with invalid CSV data (no column names)"};
    }

    // prepare an SQL string that contains all column names
    // and a bind parameter for each column
    //
    // quote column names to provide a little more resistance against
    // ugly / unsecure / dangerous strings provided by untrusted users
    Sloppy::estring sql = "INSERT INTO " + tabName +
                          " (\"" + csvTab.getHeader(0) + "\"%1) VALUES (?%2)";
    string colNames;
    string qMarks;
    for (size_t colIdx = 1; colIdx < csvTab.nCols(); ++colIdx)
    {
      colNames += ",\"" + csvTab.getHeader(colIdx) + "\"";
      qMarks += ",?";
    }
    sql.arg(colNames);
    sql.arg(qMarks);
    auto stmt = db.get().prepStatement(sql);

    // lock the table
    auto trans = db.get().startTransaction(tt);

    // iterate over all data rows and import data one-by-one
    size_t cnt{0};
    for (auto rowIt = csvTab.cbegin(); rowIt != csvTab.cend(); ++rowIt)
    {
      stmt.reset(true);

      // Note: SQLite bind parameters are 1-based while the
      // CSV columns are 0-based!
      for (size_t colIdx = 1; colIdx <= csvTab.nCols(); ++colIdx)
      {
        const auto& val = rowIt->get(colIdx - 1);  // 1-based --> 0-based

        switch (val.valueType())
        {
        case Sloppy::CSV_Value::Type::Long:
          stmt.bind(colIdx, val.get<int64_t>());
          break;

        case Sloppy::CSV_Value::Type::String:
          stmt.bind(colIdx, val.get<string>());
          break;

        case Sloppy::CSV_Value::Type::Double:
          stmt.bind(colIdx, val.get<double>());
          break;

        case Sloppy::CSV_Value::Type::Null:
          stmt.bindNull(colIdx);
          break;
        }
      }

      // the actual insertion
      stmt.step();
      ++cnt;
    }

    trans.commit();

    return cnt;
  }

  //----------------------------------------------------------------------------

  void DbTab::addColumn_exec(const string& colName, ColumnDataType colType, const string& constraints) const
  {
    string cn{colName};
    if (!Sloppy::trimAndCheckString(cn))
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
      result.emplace_back(db, tabName, stmt.get<int>(0), true);

      stmt.step();
    }

    return result;
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  TabRowIterator::TabRowIterator(const SqliteDatabase& _db, const string& _tabName, int minRowId, int maxRowId)
    :db{_db}, tabName{_tabName}
  {
    WhereClause w;
    if (minRowId > 0)
    {
      w.addCol("rowid", ">=", minRowId);
    }
    if (maxRowId > 0)
    {
      w.addCol("rowid", "<=", maxRowId);
    }
    init(w);
  }

  //----------------------------------------------------------------------------

  TabRowIterator::TabRowIterator(const SqliteDatabase& _db, const string& _tabName, const WhereClause& w)
    :db{_db}, tabName{_tabName}
  {
    init(w);
  }

  //----------------------------------------------------------------------------

  bool TabRowIterator::operator++()
  {
    curRow.reset(); // delete the old TabRow object

    stmt.step();
    if (stmt.hasData())
    {
      curRow = make_unique<TabRow>(db.get(), tabName, rowid(), true);
    }

    return stmt.hasData();
  }

  //----------------------------------------------------------------------------

  TabRow& TabRowIterator::operator*() const
  {
    if (!stmt.hasData())
    {
      throw NoDataException("TabRowIterator: trying to de-reference empty / exhausted SQL statement");
    }

    return *curRow;
  }

  //----------------------------------------------------------------------------

  TabRow* TabRowIterator::operator->() const
  {
    if (!stmt.hasData())
    {
      throw NoDataException("TabRowIterator: trying to de-reference empty / exhausted SQL statement");
    }

    return curRow.get();
  }

  //----------------------------------------------------------------------------

  void TabRowIterator::init(const WhereClause& w)
  {
    if (w.isEmpty())
    {
      stmt = db.get().prepStatement("SELECT rowid FROM " + tabName);
    } else {
      stmt = w.getSelectStmt(db, tabName, false);
    }

    // call `step()` and creates a new TabRow instance, if necessary
    operator++();
  }

  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------

}
