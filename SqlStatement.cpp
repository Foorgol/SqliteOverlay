#include <stdexcept>
#include <ctime>
#include <cstring>
#include <cstdlib>

#include <Sloppy/DateTime/DateAndTime.h>
#include <Sloppy/String.h>
#include <Sloppy/json.hpp>

#include "SqlStatement.h"
<<<<<<< HEAD
#include "Utils.h"
=======
#include "SqliteExceptions.h"
>>>>>>> dev

using namespace std;
using namespace Sloppy::DateTime;

namespace SqliteOverlay
{
  SqlStatement::SqlStatement()
    :stmt(nullptr), _hasData(false), _isDone(true), resultColCount(-1), stepCount(0)
  {

  }

  //----------------------------------------------------------------------------

  SqlStatement::SqlStatement(sqlite3* dbPtr, const string& sqlTxt)
    :stmt(nullptr), _hasData(false), _isDone(false), resultColCount(-1), stepCount(0)
  {
    if (dbPtr == nullptr)
    {
      throw std::invalid_argument("Reived null-pointer for database handle");
    }

    if (sqlTxt.empty())
    {
      throw invalid_argument("Received empty SQL statement");
    }

    int err = sqlite3_prepare_v2(dbPtr, sqlTxt.c_str(), -1, &stmt, nullptr);
    if (err != SQLITE_OK)
    {
      throw SqlStatementCreationError(err, sqlTxt, sqlite3_errmsg(dbPtr));
    }
  }

  //----------------------------------------------------------------------------

  SqlStatement::~SqlStatement()
  {
    forceFinalize();
  }

  //----------------------------------------------------------------------------

  SqlStatement& SqlStatement::operator=(SqlStatement&& other)
  {
    if (stmt != nullptr)
    {
      sqlite3_finalize(stmt);
    }
    stmt = other.stmt;
    other.stmt = nullptr;

    _hasData = other._hasData;
    other._hasData = false;
    _isDone = other._isDone;
    other._isDone = true;
    resultColCount = other.resultColCount;
    other.resultColCount = -1;
    stepCount = other.stepCount;
    other.stepCount = -1;

    return *this;
  }

  //----------------------------------------------------------------------------

  SqlStatement::SqlStatement(SqlStatement&& other)
  {
    operator=(std::move(other));
  }

  //----------------------------------------------------------------------------

  bool SqlStatement::step()
  {
    if (_isDone)
    {
      return false;
    }

    int err = sqlite3_step(stmt);
    ++stepCount;

    if (err == SQLITE_BUSY)
    {
      throw BusyException("call to step() in a SQL statement");
    }
    if (err == SQLITE_CONSTRAINT)
    {
      throw ConstraintFailedException("call to step() in a SQL statement");
    }
    if ((err != SQLITE_ROW) && (err != SQLITE_DONE) && (err != SQLITE_OK))
    {
      throw GenericSqliteException(err, "call to step() in a SQL statement");
    }

    _hasData = (err == SQLITE_ROW);
    _isDone = (err == SQLITE_DONE);

    if (_hasData)
    {
      resultColCount = sqlite3_column_count(stmt);
    } else {
      resultColCount = -1;
    }

    // for single-step statements that don't yield any
    // data, return "true" to indicate that everything was fine
    //
    // in all other cases, return "true" as long as we're not done
    if (stepCount == 1)
    {
        return true;
    }
    return _hasData;
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

  int SqlStatement::getInt(int colId) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    return sqlite3_column_int(stmt, colId);
  }

  //----------------------------------------------------------------------------

  long SqlStatement::getLong(int colId) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    return sqlite3_column_int64(stmt, colId);
  }

  //----------------------------------------------------------------------------

  double SqlStatement::getDouble(int colId) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    return sqlite3_column_double(stmt, colId);
  }

  //----------------------------------------------------------------------------

  string SqlStatement::getString(int colId) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    return string{reinterpret_cast<const char*>(sqlite3_column_text(stmt, colId))};
  }

  //----------------------------------------------------------------------------

  LocalTimestamp SqlStatement::getLocalTime(int colId, boost::local_time::time_zone_ptr tzp) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    time_t rawTime = sqlite3_column_int64(stmt, colId);
    return LocalTimestamp(rawTime, tzp);
  }

  //----------------------------------------------------------------------------

  UTCTimestamp SqlStatement::getUTCTime(int colId) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    time_t rawTime = sqlite3_column_int64(stmt, colId);
    return UTCTimestamp(rawTime);
  }

  //----------------------------------------------------------------------------

  Sloppy::MemArray SqlStatement::getBlob(int colId) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    // get the number of bytes in the blob
    size_t nBytes = sqlite3_column_bytes(stmt, colId);
    if (nBytes == 0)
    {
      return Sloppy::MemArray{}; // empty blob
    }

    // wrap the data from SQLite into a MemView
    const void* srcPtr = sqlite3_column_blob(stmt, colId);
    if (srcPtr == nullptr)  // shouldn't happen after the previous check, but we want to be sure
    {
      return Sloppy::MemArray{}; // empty blob
    }
    Sloppy::MemView fakeView{static_cast<const char*>(srcPtr), nBytes};

    return Sloppy::MemArray{fakeView};  // creates a deep copy
  }

  //----------------------------------------------------------------------------

  nlohmann::json SqlStatement::getJson(int colId) const
  {
    const string jsonData = getString(colId);

    return nlohmann::json::parse(jsonData);
  }

  //----------------------------------------------------------------------------

  ColumnDataType SqlStatement::getColDataType(int colId) const
  {
    assertColumnDataAccess(colId);
    return int2ColumnDataType(sqlite3_column_type(stmt, colId));
  }

  //----------------------------------------------------------------------------

  string SqlStatement::getColName(int colId) const
  {
    assertColumnDataAccess(colId);
    return sqlite3_column_name(stmt, colId);
  }

  //----------------------------------------------------------------------------

  bool SqlStatement::isNull(int colId) const
  {
    assertColumnDataAccess(colId);
    return (sqlite3_column_type(stmt, colId) == SQLITE_NULL);
  }

  //----------------------------------------------------------------------------

  void SqlStatement::reset(bool clearBindings)
  {
    int err = sqlite3_reset(stmt);
    if (err != SQLITE_OK)
    {
      throw GenericSqliteException(err, "SqlStatement reset()");
    }
    if (clearBindings)
    {
      // no error checking here; the manual doesn't say
      // anything about the return code. Most likely
      // it's SQLITE_OK
      sqlite3_clear_bindings(stmt);
    }

    _hasData = false;
    _isDone = false;
    resultColCount = -1;
    stepCount = 0;
  }

  //----------------------------------------------------------------------------

  void SqlStatement::forceFinalize()
  {
    if (stmt != nullptr)
    {
      sqlite3_finalize(stmt);
      stmt = nullptr;
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::get(int colId, nlohmann::json& result) const
  {
    result = getJson(colId);
  }

  //----------------------------------------------------------------------------

<<<<<<< HEAD
  string SqlStatement::toCSV(const string& sep) const
  {
    if (resultColCount < 1) return "";

    string result;
    int64_t i;  // placeholder for switch-case-section
    double d;  // placeholder for switch-case-section
    string s;  // placeholder for switch-case-section
    for (int idx = 0; idx < resultColCount; ++idx)
    {
      if (idx > 0) result += sep;

      switch (getColType(idx)) {
      case SQLITE_INTEGER:
        i = sqlite3_column_int64(stmt, idx);
        result += to_string(i);
        break;

      case SQLITE_FLOAT:
        d = sqlite3_column_double(stmt, idx);
        result += to_string(d);
        break;

      case SQLITE_NULL:    // NULL results in ",," or just ",<EOL>"
        break;

      default:
        s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, idx));
        s = quoteAndEscapeStringForCSV(s);
        result += s;
      }
    }

    return result;
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bindInt(int argPos, int val)
=======
  void SqlStatement::assertColumnDataAccess(int colId) const
>>>>>>> dev
  {
    if (!hasData())
    {
      throw NoDataException{"call to SqlStatement::getXXX() but the statement didn't return any data"};
    }

    if (colId >= resultColCount)
    {
      Sloppy::estring msg{"attempt to access column ID %1 of %2 columns"};
      msg.arg(colId);
      msg.arg(resultColCount);
      throw InvalidColumnException(msg);
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, int val) const
  {
    int e = sqlite3_bind_int(stmt, argPos, val);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindInt() of a SqlStatement"};
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, long val) const
  {
    int e = sqlite3_bind_int64(stmt, argPos, val);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindInt64() of a SqlStatement"};
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, double val) const
  {
    int e = sqlite3_bind_double(stmt, argPos, val);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindDouble() of a SqlStatement"};
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, const string& val) const
  {
    int e = sqlite3_bind_text(stmt, argPos, val.c_str(), val.length(), SQLITE_TRANSIENT);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindString() of a SqlStatement"};
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, const char* val) const
  {
    int e = sqlite3_bind_text(stmt, argPos, val, strlen(val), SQLITE_TRANSIENT);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindString() of a SqlStatement"};
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, const void* ptr, size_t nBytes) const
  {
    int e = sqlite3_bind_blob64(stmt, argPos, ptr, nBytes, SQLITE_TRANSIENT);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindBlob64() of a SqlStatement"};
    }

  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, const nlohmann::json& v) const
  {
    const string jsonData = v.dump();
    bind(argPos, jsonData);  // the actual data binding is handled as a string value
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bindNull(int argPos) const
  {
    int e = sqlite3_bind_null(stmt, argPos);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindNull() of a SqlStatement"};
    }
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


  //----------------------------------------------------------------------------

}
