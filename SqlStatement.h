#ifndef SQLSTATEMENT_H
#define SQLSTATEMENT_H

#include <string>
#include <memory>
#include <ctime>

#include <sqlite3.h>

#include "Logger.h"

using namespace std;

namespace SqliteOverlay
{  
  // a wrapper class for tm for storing timestamps
  class CommonTimestamp
  {
  public:
    CommonTimestamp(int year, int month, int day, int hour, int min, int sec);
    virtual time_t getRawTime() const = 0;

    string getISODate() const;
    string getTime() const;
    string getTimestamp() const;

  private:
    tm timestamp;
    string getFormattedString(const string& fmt) const;
  };

  // an extension of struct tm to clearly indicate that local time
  // is stored
  class LocalTimestamp : public CommonTimestamp
  {
  public:
    LocalTimestamp(int year, int month, int day, int hour, int min, int sec);
    LocalTimestamp(time_t rawTimeInUTC);
    LocalTimestamp();
    virtual time_t getRawTime() const override;

  private:
    tm timestamp;
  };

  // an extension of struct tm to clearly indicate that UTC
  // is stored
  class UTCTimestamp : public CommonTimestamp
  {
  public:
    UTCTimestamp(int year, int month, int day, int hour, int min, int sec);
    UTCTimestamp(time_t rawTimeInUTC);
    UTCTimestamp();
    virtual time_t getRawTime() const override;

  private:
    tm timestamp;
  };

  class SqlStatement
  {
  public:
    static unique_ptr<SqlStatement> get(sqlite3* dbPtr, const string& sqlTxt, const Logger* log=nullptr);
    ~SqlStatement();

    void bindInt(int argPos, int val);
    void bindDouble(int argPos, double val);
    void bindString(int argPos, string& val);

    bool step(const Logger* log=nullptr);

    bool hasData() const;
    bool isDone() const;

    bool getInt(int colId, int* out) const;
    bool getDouble(int colId, double* out) const;
    bool getString(int colId, string* out) const;
    bool getLocalTime(int colId, LocalTimestamp* out) const;
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

    SqlStatement(sqlite3* dbPtr, const string& sqlTxt);
    sqlite3_stmt* stmt;
    bool _hasData;
    bool _isDone;
    int resultColCount;
    int stepCount;
  };

  typedef unique_ptr<SqlStatement> upSqlStatement;
}

#endif /* SQLSTATEMENT_H */
