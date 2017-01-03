#include <boost/date_time/local_time/local_time.hpp>

#include <Sloppy/libSloppy.h>

#include "ClausesAndQueries.h"
#include "SqlStatement.h"

namespace SqliteOverlay {

  unique_ptr<SqlStatement> ColumnValueClause::getInsertStmt(SqliteDatabase* db, const string& tabName) const
  {
    if (tabName.empty())
    {
      return nullptr;
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

  unique_ptr<SqlStatement> ColumnValueClause::getUpdateStmt(SqliteDatabase* db, const string& tabName, int rowId) const
  {
    if (tabName.empty() || colVals.empty())
    {
      return nullptr;
    }

    string sql;
    for (const ColValInfo& curCol : colVals)
    {
      if (!(sql.empty()))
      {
        sql += ", ";
      }

      sql += curCol.colName + "=";
      sql += (curCol.type == ColValType::Null) ? "NULL" : "?";
    }
    sql = "UPDATE " + tabName + " SET " + sql;
    sql += " WHERE id=" + to_string(rowId);

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  bool ColumnValueClause::hasColumns() const
  {
    return (!(colVals.empty()));
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  void WhereClause::addIntCol(const string& colName, const string& op, int val)
  {
    addIntCol(colName, val);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addDoubleCol(const string& colName, const string& op, double val)
  {
    addDoubleCol(colName, val);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addStringCol(const string& colName, const string& op, const string& val)
  {
    addStringCol(colName, val);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addDateTimeCol(const string& colName, const string& op, const CommonTimestamp* pTimestamp)
  {
    addDateTimeCol(colName, pTimestamp);
    colVals[colVals.size() - 1].op = op;
  }

  //----------------------------------------------------------------------------

  unique_ptr<SqlStatement> WhereClause::getSelectStmt(SqliteDatabase* db, const string& tabName, bool countOnly) const
  {
    if ((tabName.empty()) || (isEmpty()))
    {
      return nullptr;
    }

    string sql = "SELECT ";
    if (countOnly) sql += "COUNT(*)";
    else sql += "id";
    sql += " FROM " + tabName + " WHERE " + getWherePartWithPlaceholders();

    if (!(orderBy.empty()))
    {
      sql += orderBy;
    }

    if (limit > 0)
    {
      sql += " LIMIT " + to_string(limit);
    }

    return createStatementAndBindValuesToPlaceholders(db, sql);
  }

  //----------------------------------------------------------------------------

  unique_ptr<SqlStatement> WhereClause::getDeleteStmt(SqliteDatabase* db, const string& tabName) const
  {
    if ((tabName.empty()) || (isEmpty()))
    {
      return nullptr;
    }

    string sql = "DELETE ";
    sql += "FROM " + tabName + " WHERE " + getWherePartWithPlaceholders();

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
    doubleVals.clear();
    stringVals.clear();
    colVals.clear();
  }

  //----------------------------------------------------------------------------

  unique_ptr<SqlStatement> CommonClause::createStatementAndBindValuesToPlaceholders(SqliteDatabase* db, const string& sql) const
  {
    auto stmt = db->prepStatement(sql);
    if (stmt == nullptr) return nullptr;

    // bind the actual column values to the placeholders
    int curPlaceholderIdx = 1;   // leftmost placeholder is at position 1
    for (size_t i=0; i < colVals.size(); ++i)
    {
      const ColValInfo& curCol = colVals[i];

      // NULL or NOT NULL has to handled directly as literal value when
      // creating the sql statement's text
      if ((curCol.type == ColValType::Null) || (curCol.type == ColValType::NotNull)) continue;

      switch (curCol.type)
      {
      case ColValType::Int:
        stmt->bindInt(curPlaceholderIdx, intVals[curCol.indexInList]);
        break;

      case ColValType::Double:
        stmt->bindDouble(curPlaceholderIdx, doubleVals[curCol.indexInList]);
        break;

      case ColValType::String:
        stmt->bindString(curPlaceholderIdx, stringVals[curCol.indexInList]);
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
