#ifndef SQLITE_OVERLAY_SQLSTATEMENT_H
#define SQLITE_OVERLAY_SQLSTATEMENT_H

#include <string>
#include <memory>
#include <ctime>

#include <sqlite3.h>

#include "Logger.h"
#include "DateAndTime.h"

using namespace std;

namespace SqliteOverlay
{  

  class SqlStatement
  {
  public:
    static unique_ptr<SqlStatement> get(sqlite3* dbPtr, const string& sqlTxt, int* errCodeOut=nullptr, const Logger* log=nullptr);
    ~SqlStatement();

    void bindInt(int argPos, int val);
    void bindDouble(int argPos, double val);
    void bindString(int argPos, const string& val);

    bool step(int* errCodeOut=nullptr, const Logger* log=nullptr);

    bool hasData() const;
    bool isDone() const;

    bool getInt(int colId, int* out) const;
    bool getDouble(int colId, double* out) const;
    bool getString(int colId, string* out) const;
    bool getLocalTime(int colId, LocalTimestamp* out, boost::local_time::time_zone_ptr tzp) const;
    bool getUTCTime(int colId, UTCTimestamp* out) const;

    int getColType(int colId) const;
    string getColName(int colId) const;
    int isNull(int colId) const;

  private:
    template<class T>
    bool getColumnValue_prep(int colId, T* out) const {
      if (out == nullptr) return false;
      if (!hasData()) return false;
      if (colId >= resultColCount) return false;
      return true;
    }

    SqlStatement(sqlite3* dbPtr, const string& sqlTxt, int* errCodeOut);
    sqlite3_stmt* stmt;
    bool _hasData;
    bool _isDone;
    int resultColCount;
    int stepCount;
  };

  typedef unique_ptr<SqlStatement> upSqlStatement;
}

#endif /* SQLSTATEMENT_H */
