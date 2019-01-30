#include <boost/date_time/local_time/local_time.hpp>

#include "ClausesAndQueries.h"
#include "SqlStatement.h"

namespace SqliteOverlay {

  SqlStatement ColumnValueClause::getInsertStmt(const SqliteDatabase& db, const string& tabName) const
  {
    if (tabName.empty())
    {
      throw std::invalid_argument("getInsertStmt(): empty parameters");
    }

    string sql;
    if (colVals.empty())
    {
      sql = "INSERT INTO " + tabName + " DEFAULT VALUES";
    } else {

      // construct SQL INSERT statement with placeholders ('?')
      string params;
      for (const ColValInfo& curCol : colVals)
      {
        if (!(sql.empty()))
        {
          sql += ",";
          params += ",";
        }

        sql += curCol.colName;
        params += (curCol.type == ColValType::Null) ? "NULL" : "?";
      }
      sql = "INSERT INTO " + tabName + " (" + sql;
      sql += ") VALUES (" + params + ")";
    }

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  //----------------------------------------------------------------------------

  SqlStatement ColumnValueClause::getUpdateStmt(const SqliteDatabase& db, const string& tabName, int rowId) const
  {
    if (tabName.empty() || colVals.empty())
    {
      throw std::invalid_argument("getUpdateStmt(): empty parameters");
    }

    string sql;
    for (const ColValInfo& curCol : colVals)
    {
      if (!(sql.empty()))
      {
        sql += ",";
      }

      sql += curCol.colName + "=";
      sql += (curCol.type == ColValType::Null) ? "NULL" : "?";
    }
    sql = "UPDATE " + tabName + " SET " + sql;
    sql += " WHERE rowid=" + to_string(rowId);

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  //----------------------------------------------------------------------------

  bool ColumnValueClause::hasColumns() const
  {
    return (!(colVals.empty()));
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  void WhereClause::addCol(const string& colName, const string& op, int val)
  {
    addCol(colName, val);
    colVals[colVals.size() - 1].op = op;
  }

  void WhereClause::addCol(const string& colName, const string& op, long val)
  {
    addCol(colName, val);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addCol(const string& colName, const string& op, double val)
  {
    addCol(colName, val);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addCol(const string& colName, const string& op, const string& val)
  {
    addCol(colName, val);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addCol(const string& colName, const string& op, const CommonTimestamp* pTimestamp)
  {
    addCol(colName, pTimestamp);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  SqlStatement WhereClause::getSelectStmt(const SqliteDatabase& db, const string& tabName, bool countOnly) const
  {
    if (tabName.empty() || isEmpty())
    {
      throw std::invalid_argument("getSelectStmt(): empty parameters");
    }

    Sloppy::estring sql{"SELECT %1 FROM %2 WHERE %3 %4"};
    if (countOnly)
    {
      sql.arg("COUNT(*)");
    } else
    {
      sql.arg("id");
    }
    sql.arg(tabName);
    sql.arg(getWherePartWithPlaceholders());

    sql.arg(orderBy);   // removes the "%4" if orderBy is empty

    if (limit > 0)
    {
      sql += " LIMIT " + to_string(limit);
    }

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  //----------------------------------------------------------------------------

  SqlStatement WhereClause::getDeleteStmt(const SqliteDatabase& db, const string& tabName) const
  {
    if (tabName.empty() || isEmpty())
    {
      throw std::invalid_argument("getDeleteStmt(): empty parameters");
    }

    Sloppy::estring sql{"DELETE FROM %1 WHERE %2"};
    sql.arg(tabName);
    sql.arg(getWherePartWithPlaceholders());

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  //----------------------------------------------------------------------------

  bool WhereClause::isEmpty() const
  {
    return colVals.empty();
  }

  //----------------------------------------------------------------------------

  void WhereClause::setOrderColumn_Asc(const string& colName)
  {
    if (orderBy.empty())
    {
      orderBy = " ORDER BY ";
    } else {
      orderBy += ", ";
    }
    orderBy += colName + " ASC";
  }

  void WhereClause::setOrderColumn_Desc(const string& colName)
  {
    if (orderBy.empty())
    {
      orderBy = " ORDER BY ";
    } else {
      orderBy += ", ";
    }
    orderBy += colName + " DESC";
  }

  //----------------------------------------------------------------------------

  void WhereClause::setLimit(int _limit)
  {
    if (_limit > 0) limit = _limit;
  }

  //----------------------------------------------------------------------------

  string WhereClause::getWherePartWithPlaceholders() const
  {
    string w;

    for (const ColValInfo& curCol : colVals)
    {
      if (!(w.empty())) w += " AND ";

      w += curCol.colName;

      switch (curCol.type)
      {
      case ColValType::Null:
        w += " IS NULL";
        break;

      case ColValType::NotNull:
        w += " IS NOT NULL";
        break;

      default:
        w += (curCol.op.empty()) ? "=" : curCol.op;
        w += "?";
      }
    }

    return w;
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  void CommonClause::clear()
  {
    intVals.clear();
    longVals.clear();
    doubleVals.clear();
    stringVals.clear();
    colVals.clear();
  }

  //----------------------------------------------------------------------------

  SqlStatement CommonClause::createStatementAndBindValuesToPlaceholders(const SqliteDatabase& db, const string& sql) const
  {
    SqlStatement stmt = db.prepStatement(sql);

    // bind the actual column values to the placeholders
    int curPlaceholderIdx = 1;   // leftmost placeholder is at position 1
    for (const ColValInfo& curCol : colVals)
    {
      // NULL or NOT NULL has to be handled directly as literal value when
      // creating the sql statement's text
      if ((curCol.type == ColValType::Null) || (curCol.type == ColValType::NotNull)) continue;

      switch (curCol.type)
      {
      case ColValType::Int:
        stmt.bind(curPlaceholderIdx, intVals[curCol.indexInList]);
        break;

      case ColValType::Long:
        stmt.bind(curPlaceholderIdx, longVals[curCol.indexInList]);
        break;

      case ColValType::Double:
        stmt.bind(curPlaceholderIdx, doubleVals[curCol.indexInList]);
        break;

      case ColValType::String:
        stmt.bind(curPlaceholderIdx, stringVals[curCol.indexInList]);
        break;

      default:
        throw std::runtime_error("unhandled argument type when binding argument values to statement");
      }

      ++curPlaceholderIdx;
    }

    return stmt;
  }


  //----------------------------------------------------------------------------


}
