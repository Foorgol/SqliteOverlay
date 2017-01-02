#ifndef SQLITE_OVERLAY_CLAUSESANDQUERIES_H
#define	SQLITE_OVERLAY_CLAUSESANDQUERIES_H

#include <vector>

#include <Sloppy/libSloppy.h>
#include <Sloppy/DateTime/DateAndTime.h>

#include "SqliteDatabase.h"

using namespace Sloppy::DateTime;

namespace SqliteOverlay {

  class CommonClause
  {
  public:
    CommonClause(){}
    virtual ~CommonClause(){}

    inline void addIntCol(const string& colName, int val)
    {
      intVals.push_back(val);
      colVals.push_back(ColValInfo{colName, ColValType::Int, static_cast<int>(intVals.size()) - 1, ""});
    }

    inline void addDoubleCol(const string& colName, double val)
    {
      doubleVals.push_back(val);
      colVals.push_back(ColValInfo{colName, ColValType::Double, static_cast<int>(doubleVals.size()) - 1, ""});
    }

    inline void addStringCol(const string& colName, const string& val)
    {
      stringVals.push_back(val);
      colVals.push_back(ColValInfo{colName, ColValType::String, static_cast<int>(stringVals.size()) - 1, ""});
    }

    inline void addNullCol(const string& colName)
    {
      colVals.push_back(ColValInfo{colName, ColValType::Null, -1, ""});
    }

    inline void addDateTimeCol(const string& colName, const CommonTimestamp* pTimestamp)
    {
      addIntCol(colName, pTimestamp->getRawTime());
    }

    inline void addDateCol(const string& colName, const boost::gregorian::date& d)
    {
      addIntCol(colName, boost::gregorian::to_int(d));
    }

    void clear();

    unique_ptr<SqlStatement> createStatementAndBindValuesToPlaceholders(SqliteDatabase* db, const string& sql) const;

  protected:
    enum ColValType
    {
      Int,
      Double,
      String,
      Null,
      NotNull,
    };

    struct ColValInfo
    {
      string colName;
      ColValType type;
      int indexInList;
      string op;
    };

    vector<int> intVals;
    vector<double> doubleVals;
    vector<string> stringVals;

    vector<ColValInfo> colVals;
  };

  //----------------------------------------------------------------------------

  class ColumnValueClause : public CommonClause
  {
  public:
    ColumnValueClause()
      :CommonClause() {}

    unique_ptr<SqlStatement> getInsertStmt(SqliteDatabase* db, const string& tabName) const;
    unique_ptr<SqlStatement> getUpdateStmt(SqliteDatabase* db, const string& tabName, int rowId) const;
    bool hasColumns() const;

  private:
  };

  //----------------------------------------------------------------------------

  class WhereClause : public CommonClause
  {
  public:
    WhereClause()
      :CommonClause(){}


    using CommonClause::addIntCol;
    using CommonClause::addDoubleCol;
    using CommonClause::addStringCol;
    using CommonClause::addDateTimeCol;
    using CommonClause::addDateCol;
    using CommonClause::addNullCol;

    void addIntCol(const string& colName, const string& op, int val);
    void addDoubleCol(const string& colName, const string& op, double val);
    void addStringCol(const string& colName, const string& op, const string& val);
    void addDateTimeCol(const string& colName, const string& op,  const CommonTimestamp* pTimestamp);
    inline void addNotNullCol(const string& colName)
    {
      colVals.push_back(ColValInfo{colName, ColValType::NotNull, -1, ""});
    }
    inline void addDateCol(const string& colName, const string& op, const boost::gregorian::date& d)
    {
      addIntCol(colName, op, boost::gregorian::to_int(d));
    }

    unique_ptr<SqlStatement> getSelectStmt(SqliteDatabase* db, const string& tabName, bool countOnly) const;
    unique_ptr<SqlStatement> getDeleteStmt(SqliteDatabase* db, const string& tabName) const;

    bool isEmpty() const;

    void setOrderColumn_Asc(const string& colName);
    void setOrderColumn_Desc(const string& colName);
    void setLimit(int _limit);

  protected:
    string getWherePartWithPlaceholders() const;

  private:
    string orderBy;
    int limit;
  };


  template<class T>
  class ScalarQueryResult
  {
  public:
    ScalarQueryResult(T result)
      : cachedResult(result), _isNull(false)
    {
    }

    ScalarQueryResult()
      : _isNull(true)
    {
    }

    virtual ~ScalarQueryResult(){}

    bool isNull() const
    {
      return _isNull;
    }

    T get() const
    {
      if (_isNull)
      {
        throw std::invalid_argument("Attempt to access NULL result");
      }
      return cachedResult;
    }

  private:
    T cachedResult;
    bool _isNull;
  };

}

#endif  /* CLAUSESANDQUERIES_H */
