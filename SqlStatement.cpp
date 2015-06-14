#include <stdexcept>
#include "SqlStatement.h"

using namespace std;

namespace SqliteOverlay
{
  SqlStatement::SqlStatement(sqlite3* dbPtr, const string& sqlTxt)
    :stmt(nullptr), _hasData(false), _isDone(false), resultColCount(-1), stepCount(0)
  {
    if (dbPtr == nullptr)
    {
      throw runtime_error("Reived null-pointer for database handle");
    }

    if (sqlTxt.empty())
    {
      throw invalid_argument("Receied empty SQL statement");
    }

    int err = sqlite3_prepare_v2(dbPtr, sqlTxt.c_str(), -1, &stmt, nullptr);
    if (err != SQLITE_OK)
    {
      throw invalid_argument(sqlite3_errmsg(dbPtr));
    }
  }

  //----------------------------------------------------------------------------

  unique_ptr<SqlStatement> SqlStatement::get(sqlite3* dbPtr, const string& sqlTxt, const Logger* log)
  {
    SqlStatement* tmpPtr;
    try
    {
      tmpPtr = new SqlStatement(dbPtr, sqlTxt);
    } catch (exception e) {
      if (log != nullptr)
      {
        string msg = "Creating SQL statement '";
        msg += sqlTxt;
        msg += "' raised: ";
        msg += e.what();
        log->warn(msg);
      }
      return nullptr;
    }

    return upSqlStatement(tmpPtr);
  }

  //----------------------------------------------------------------------------

  SqlStatement::~SqlStatement()
  {
    if (stmt != nullptr)
    {
      sqlite3_finalize(stmt);
      stmt = nullptr;
    }
  }

  //----------------------------------------------------------------------------

  bool SqlStatement::step(const Logger* log)
  {
    if (_isDone)
    {
      if (log != nullptr)
      {
        string msg = "Tried to execute step ";
        msg += to_string(stepCount + 1);
        msg += " on the already finished query '";
        msg += sqlite3_sql(stmt);
        msg += "'";
        log->warn(msg);
      }
      return false;
    }

    int err = sqlite3_step(stmt);
    ++stepCount;

    _hasData = (err == SQLITE_ROW);
    _isDone = (err == SQLITE_DONE);

    if (_hasData)
    {
      resultColCount = sqlite3_column_count(stmt);
    } else {
      resultColCount = -1;
    }

    if (log != nullptr)
    {
      string msg = "Executed step ";
      msg += to_string(stepCount);
      msg += " on '";
      msg += sqlite3_sql(stmt);
      msg += "'; ";
      msg += "hasData = " + to_string(_hasData);
      msg += "; isDone = " + to_string(_isDone);
      msg += "; columns returned = " + to_string(resultColCount);
      log->info(msg);
    }

    return ((err == SQLITE_ROW) || (err == SQLITE_DONE) || (err == SQLITE_OK));
  }

  //----------------------------------------------------------------------------

  bool SqlStatement::hasData() const
  {
    return _hasData;
  }

  //----------------------------------------------------------------------------

  bool SqlStatement::isDone() const
  {
    return _isDone;
  }

  //----------------------------------------------------------------------------

  bool SqlStatement::getInt(int colId, int* out) const
  {
    if (!hasData()) return false;
    if (out == nullptr) return false;
    if (colId >= resultColCount) return false;

    *out = sqlite3_column_int(stmt, colId);
    return true;
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bindInt(int argPos, int val)
  {
    sqlite3_bind_int(stmt, argPos, val);
  }

  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------

}
