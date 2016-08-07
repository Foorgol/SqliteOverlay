#include "ClausesAndQueries.h"
#include "SqlStatement.h"
#include "StringHelper.h"

namespace SqliteOverlay {

  void ColumnValueClause::clear()
  {
    colNames.clear();
    values.clear();
  }

//----------------------------------------------------------------------------

  void ColumnValueClause::addIntCol(const string& colName, int val)
  {
    addCol(colName, to_string(val));
  }

//----------------------------------------------------------------------------

  void ColumnValueClause::addDoubleCol(const string& colName, double val)
  {
    addCol(colName, to_string(val));
  }

//----------------------------------------------------------------------------

  void ColumnValueClause::addStringCol(const string& colName, const string& val)
  {
    addCol(colName, val, true);
  }

//----------------------------------------------------------------------------

  void ColumnValueClause::addDateTimeCol(const string& colName, const CommonTimestamp* pTimestamp)
  {
    // thanks to polymorphism, this works for
    // both LocalTimestamp and UTCTimestamp
    time_t rawTime = pTimestamp->getRawTime();
    addIntCol(colName, rawTime);
  }

//----------------------------------------------------------------------------

  void ColumnValueClause::addNullCol(const string& colName)
  {
    addCol(colName, "NULL");
  }

  //----------------------------------------------------------------------------

  void ColumnValueClause::addDateCol(const string& colName, const boost::gregorian::date& d)
  {
    addIntCol(colName, boost::gregorian::to_int(d));
  }

//----------------------------------------------------------------------------

  string ColumnValueClause::getInsertStmt(const string& tabName) const
  {
    if (tabName.empty())
    {
      return string();
    }

    if (!(hasColumns()))
    {
      return "INSERT INTO " + tabName + " DEFAULT VALUES";
    }

    string sql = "INSERT INTO " + tabName + " (";
    sql += commaSepStringFromStringList(colNames) + ") VALUES (";
    sql += commaSepStringFromStringList(values) + ")";

    return sql;
  }

//----------------------------------------------------------------------------

  string ColumnValueClause::getUpdateStmt(const string& tabName, int rowId) const
  {
    if ((tabName.empty()) || (!(hasColumns())))
    {
      return string();
    }

    string sql = "UPDATE " + tabName + " SET ";
    for (int i=0; i < colNames.size(); ++i)
    {
      if (i > 0)
      {
        sql += ", ";
      }
      sql += colNames.at(i) + "=" + values.at(i);
    }

    sql += " WHERE id=" + to_string(rowId);

    return sql;
  }

//----------------------------------------------------------------------------

  bool ColumnValueClause::hasColumns() const
  {
    return (!(colNames.empty()));
  }

//----------------------------------------------------------------------------

  void ColumnValueClause::addCol(const string& colName, const string& val, bool useQuotes)
  {
    colNames.push_back(colName);
    if (useQuotes)
    {
      values.push_back("\"" + val + "\"");
    } else {
      values.push_back(val);
    }
  }

//----------------------------------------------------------------------------


  WhereClause::WhereClause()
  {
    clear();
  }

  void WhereClause::clear()
  {
    sql.clear();
    orderBy.clear();
    colCount = 0;
    limit = -1;
  }

//----------------------------------------------------------------------------

  void WhereClause::addIntCol(const string& colName, int val)
  {
    addCol(colName, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addIntCol(const string& colName, const string& op, int val)
  {
    addCol(colName, op, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addDoubleCol(const string& colName, double val)
  {
    addCol(colName, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addDoubleCol(const string& colName, const string& op, double val)
  {
    addCol(colName, op, to_string(val));
  }

  //----------------------------------------------------------------------------

  void WhereClause::addStringCol(const string& colName, const string& val)
  {
    addCol(colName, val, true);
  }

  //----------------------------------------------------------------------------

  void WhereClause::addStringCol(const string& colName, const string& op, const string& val)
  {
    addCol(colName, op, val, true);
  }

  void WhereClause::addDateTimeCol(const string& colName, const CommonTimestamp* pTimestamp)
  {
    // thanks to polymorphism, this works for
    // both LocalTimestamp and UTCTimestamp
    time_t rawTime = pTimestamp->getRawTime();
    addIntCol(colName, rawTime);
  }

  //----------------------------------------------------------------------------

  void WhereClause::addDateTimeCol(const string& colName, const string& op, const CommonTimestamp* pTimestamp)
  {
    // thanks to polymorphism, this works for
    // both LocalTimestamp and UTCTimestamp
    time_t rawTime = pTimestamp->getRawTime();
    addIntCol(colName, op, rawTime);
  }

  //----------------------------------------------------------------------------

  void WhereClause::addNullCol(const string& colName)
  {
    if (colCount > 0)
    {
      sql += " AND ";
    }
    sql += colName + " IS NULL";

    ++colCount;
  }

  //----------------------------------------------------------------------------

  void WhereClause::addNotNullCol(const string& colName)
  {
    if (colCount > 0)
    {
      sql += " AND ";
    }
    sql += colName + " IS NOT NULL";

    ++colCount;
  }

  //----------------------------------------------------------------------------

  string WhereClause::getSelectStmt(const string& tabName, bool countOnly) const
  {
    if ((tabName.empty()) || (isEmpty()))
    {
      return string();
    }

    string result = "SELECT ";
    if (countOnly) result += "COUNT(*)";
    else result += "id";
    result += " FROM " + tabName + " WHERE " + sql;

    if (!(orderBy.empty()))
    {
      result += orderBy;
    }

    if (limit > 0)
    {
      result += " LIMIT " + to_string(limit);
    }

    return result;
  }

//----------------------------------------------------------------------------

  string WhereClause::getDeleteStmt(const string& tabName) const
  {
    if ((tabName.empty()) || (isEmpty()))
    {
      return string();
    }

    string result = "DELETE ";
    result += "FROM " + tabName + " WHERE " + sql;

    return result;
  }

//----------------------------------------------------------------------------

  bool WhereClause::isEmpty() const
  {
    return (colCount == 0);
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

  void WhereClause::addCol(const string& colName, const string& val, bool useQuotes)
  {
    addCol(colName, "=", val, useQuotes);
  }

//----------------------------------------------------------------------------

  void WhereClause::addCol(const string& colName, const string& op, const string& val, bool useQuotes)
  {
    if (colCount > 0)
    {
      sql += " AND ";
    }
    sql += colName + op;
    if (useQuotes) sql += "\"" + val + "\"";
    else sql += val;

    ++colCount;
  }

}
