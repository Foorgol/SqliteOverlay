#include <stdint.h>          // for int64_t
#include <iosfwd>            // for std
#include <memory>            // for allocator_traits<>::value_type
#include <stdexcept>         // for invalid_argument, runtime_error

#include <Sloppy/String.h>   // for estring
#include <Sloppy/json.hpp>   // for json

#include "SqlStatement.h"    // for SqlStatement
#include "SqliteDatabase.h"  // for SqliteDatabase

#include "ClausesAndQueries.h"

using namespace std;

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

  bool CommonClause::isEmpty() const
  {
    return colVals.empty();
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  SqlStatement WhereClause::getSelectStmt(const SqliteDatabase& db, const string& tabName, bool countOnly) const
  {
    if (tabName.empty() || (!countOnly && isEmpty()))
    {
      throw std::invalid_argument("getSelectStmt(): empty parameters");
    }

    // if we have no column values and countOnly is true, we simply
    // return the number of rows in the table
    if (isEmpty())
    {
      Sloppy::estring sql = "SELECT COUNT(*) FROM " + tabName;
      return createStatementAndBindValuesToPlaceholders(db, sql);
    }

    // prepare a select statement for the provided table name
    // with a specific where clause
    //
    // Although it would look nicer, we don't use arg() here
    // because it is comparably slow and getSelectStmt() is
    // called VERY often by client applications.
    //
    // So we simply concatenate strings here.
    std::string sql{"SELECT "};
    if (countOnly)
    {
      sql += ("COUNT(*)");
    } else
    {
      sql += ("rowid");
    }
    sql += " FROM " + tabName + " WHERE ";
    sql += getWherePartWithPlaceholders(true);

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  //----------------------------------------------------------------------------

  SqlStatement WhereClause::getDeleteStmt(const SqliteDatabase& db, const string& tabName) const
  {
    if (tabName.empty() || isEmpty())
    {
      throw std::invalid_argument("getDeleteStmt(): empty parameters");
    }

    // Although it would look nicer, we don't use arg() here
    // because it is comparably slow.
    // So we simply concatenate strings here.
    //Sloppy::estring sql{"DELETE FROM %1 WHERE %2"};
    std::string sql{"DELETE FROM " + tabName + " WHERE "};
    sql += getWherePartWithPlaceholders(false);

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  //----------------------------------------------------------------------------

  void WhereClause::setOrderColumn_Asc(const string& colName)
  {
    if (orderBy.empty())
    {
      orderBy = "ORDER BY ";
    } else {
      orderBy += ", ";
    }
    orderBy += colName + " ASC";
  }

  //----------------------------------------------------------------------------

  void WhereClause::setOrderColumn_Desc(const string& colName)
  {
    if (orderBy.empty())
    {
      orderBy = "ORDER BY ";
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

  void WhereClause::clear()
  {
    CommonClause::clear();
    limit = 0;
    orderBy.clear();
  }

  //----------------------------------------------------------------------------

  string WhereClause::getWherePartWithPlaceholders(bool includeOrderByAndLimit) const
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

    if (includeOrderByAndLimit && !orderBy.empty())
    {
      w += " " + orderBy;
    }
    if (includeOrderByAndLimit && (limit > 0))
    {
      w += " LIMIT " + to_string(limit);
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
