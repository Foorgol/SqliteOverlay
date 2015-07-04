#ifndef CLAUSESANDQUERIES_H
#define	CLAUSESANDQUERIES_H

#include "SqliteDatabase.h"

namespace SqliteOverlay {

  class CommonTimestamp;

  class ColumnValueClause
  {
  public:
    ColumnValueClause(){}
    void clear();

    void addIntCol(const string& colName, int val);
    void addDoubleCol(const string& colName, double val);
    void addStringCol(const string& colName, const string& val);
    void addDateTimeCol(const string& colName, const CommonTimestamp* pTimestamp);
    void addNullCol(const string& colName);

    string getInsertStmt(const string& tabName) const;
    string getUpdateStmt(const string& tabName, int rowId) const;
    bool hasColumns() const;

  private:
    StringList colNames;
    StringList values;

    void addCol(const string& colName, const string& val, bool useQuotes=false);
  };

  class WhereClause
  {
  public:
    WhereClause();
    void clear();

    void addIntCol(const string& colName, int val);
    void addIntCol(const string& colName, const string& op, int val);
    void addDoubleCol(const string& colName, double val);
    void addDoubleCol(const string& colName, const string& op, double val);
    void addStringCol(const string& colName, const string& val);
    void addStringCol(const string& colName, const string& op, const string& val);
    void addDateTimeCol(const string& colName, const CommonTimestamp* pTimestamp);
    void addDateTimeCol(const string& colName, const string& op,  const CommonTimestamp* pTimestamp);
    void addNullCol(const string& colName);
    void addNotNullCol(const string& colName);

    string getSelectStmt(const string& tabName, bool countOnly) const;
    string getDeleteStmt(const string& tabName) const;

    bool isEmpty() const;

    void setOrderColumn_Asc(const string& colName);
    void setOrderColumn_Desc(const string& colName);

  private:
    string sql;
    int colCount;
    string orderBy;

    void addCol(const string& colName, const string& val, bool useQuotes=false);
    void addCol(const string& colName, const string& op, const string& val, bool useQuotes=false);
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

    virtual ~ScalarQueryResult(){};

    bool isNull()
    {
      return _isNull;
    }

    T get()
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
