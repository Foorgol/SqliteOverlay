#include <algorithm>                      // for max
#include <cstdint>                        // for int64_t
#include <cstring>                        // for strlen
#include <ctime>                          // for size_t, time_t
#include <iosfwd>                         // for std
#include <memory>                         // for allocator
#include <stdexcept>                      // for invalid_argument
#include <utility>                        // for move

#include <Sloppy/DateTime/DateAndTime.h>  // for WallClockTimepoint_secs
#include <Sloppy/String.h>                // for estring
#include <Sloppy/json.hpp>                // for json, basic_json


#include "SqliteExceptions.h"             // for GenericSqliteException, Nul...
#include "SqlStatement.h"

namespace date { class time_zone; }

using namespace std;
using namespace Sloppy::DateTime;

namespace SqliteOverlay
{
  SqlStatement::SqlStatement()
    :_isDone(true)
  {

  }

  //----------------------------------------------------------------------------

  SqlStatement::SqlStatement(sqlite3* dbPtr, const string& sqlTxt)
    :_isDone(false)
  {
    if (dbPtr == nullptr)
    {
      throw std::invalid_argument("Reived null-pointer for database handle");
    }

    if (sqlTxt.empty())
    {
      throw std::invalid_argument("Received empty SQL statement");
    }

    const int err = sqlite3_prepare_v2(dbPtr, sqlTxt.c_str(), -1, &stmt, nullptr);
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

    const int err = sqlite3_step(stmt);
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

    resultColCount = _hasData ? sqlite3_data_count(stmt) : -1;

    // for single-step statements that don't yield any
    // data, return "true" to indicate that everything was fine
    //
    // in all other cases, return "true" as long as we're not done
    return (stepCount == 1) ? true : _hasData;
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

  Sloppy::DateTime::WallClockTimepoint_secs SqlStatement::get(int colId, const date::time_zone* tzp) const
  {
    if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
    {
      throw NullValueException();
    }

    const time_t rawTime = sqlite3_column_int64(stmt, colId);
    return Sloppy::DateTime::WallClockTimepoint_secs(rawTime, tzp);
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

  std::optional<Sloppy::DateTime::WallClockTimepoint_secs> SqlStatement::get2(int colId, const date::time_zone* tzp) const
  {
    assertColumnDataAccess(colId);

    if (isNull_NoGuards(colId))
    {
      return std::nullopt;
    }

    const time_t rawTime = sqlite3_column_int64(stmt, colId);
    return Sloppy::DateTime::WallClockTimepoint_secs(rawTime, tzp);
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
    if (stmt != nullptr) {
      const int err = sqlite3_reset(stmt);
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

  string SqlStatement::getExpandedSQL() const
  {
    char* sql = sqlite3_expanded_sql(stmt);
    string result{sql};
    sqlite3_free(sql);

    return result;
  }

  //----------------------------------------------------------------------------

  Sloppy::CSV_Table SqlStatement::toCSV(bool includeHeaders)
  {
    if (isDone())
    {
      throw NoDataException{"SqlStatement::toCSV(): called on finalized statement"};
    }

    try
    {
      // if we haven't stepped to far, do this now in order to
      // determine the number of columns
      if (stepCount == 0) step();

      Sloppy::CSV_Table csvTab;

      if (!hasData() || (resultColCount < 1))
      {
        forceFinalize();
        return csvTab;  // no data ==> empty table
      }

      // add headers, if requested
      if (includeHeaders)
      {
        const auto headers = columnHeaders();

        if (!csvTab.setHeader(headers))
        {
          forceFinalize();
          throw InvalidColumnException{"SqlStatement::toCSV(): invalid column name(s) for CSV-export"};
        }
      }

      // iterate over rows and columns
      for ( ; _hasData; step())
      {
        const Sloppy::CSV_Row r = toCSV_currentRowOnly();

        if (!csvTab.append(std::move(r)))
        {
          forceFinalize();
          throw InvalidColumnException{"SqlStatement::toCSV(): inconsistent result column count in SQL statement"};
        }
      }

      return csvTab;

    } catch (...) {
      forceFinalize();
      throw;
    }
  }

  //----------------------------------------------------------------------------

  Sloppy::CSV_Row SqlStatement::toCSV_currentRowOnly() const
  {
    if ((resultColCount < 1) || !_hasData)
    {
      throw NoDataException{"SqlStatement::toCSV_currentRowOnly(): called on empty or finalized statement"};
    }

    Sloppy::CSV_Row r;
    for (int colId = 0; colId < resultColCount; ++colId)
    {
      switch (int2ColumnDataType(sqlite3_column_type(stmt, colId)))
      {
      case ColumnDataType::Integer:
        r.append(static_cast<int64_t>(sqlite3_column_int64(stmt, colId)));
        break;

      case ColumnDataType::Text:
        r.append(reinterpret_cast<const char*>(sqlite3_column_text(stmt, colId)));
        break;

      case ColumnDataType::Null:
        r.append();
        break;

      case ColumnDataType::Float:
        r.append(sqlite3_column_double(stmt, colId));
        break;

      default:
        throw InvalidColumnException{"SqlStatement::toCSV_currentRowOnly(): invalid column data type for CSV-export (probably BLOB)"};
      }
    }

    return r;
  }

  //----------------------------------------------------------------------------

  std::vector<string> SqlStatement::columnHeaders() const
  {
    if (!_hasData)
    {
      throw NoDataException{"SqlStatement::columnHeaders(): called on empty or finalized statement"};
    }

    std::vector<string> headers;
    for (int colId = 0; colId < resultColCount; ++colId)
    {
      headers.push_back(sqlite3_column_name(stmt, colId));
    }

    return headers;
  }

  //----------------------------------------------------------------------------

  void SqlStatement::assertColumnDataAccess(int colId) const
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

  bool SqlStatement::isNull_NoGuards(int colId) const
  {
    return (sqlite3_column_type(stmt, colId) == SQLITE_NULL);
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, const char* val) const
  {
    const int e = sqlite3_bind_text(stmt, argPos, val, strlen(val), SQLITE_TRANSIENT);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindString() of a SqlStatement"};
    }
  }

  //----------------------------------------------------------------------------

  void SqlStatement::bind(int argPos, const void* ptr, size_t nBytes) const
  {
    const int e = sqlite3_bind_blob64(stmt, argPos, ptr, nBytes, SQLITE_TRANSIENT);
    if (e != SQLITE_OK)
    {
      throw GenericSqliteException{e, "call to bindBlob64() of a SqlStatement"};
    }

  }

  //----------------------------------------------------------------------------

  void SqlStatement::bindNull(int argPos) const
  {
    const int e = sqlite3_bind_null(stmt, argPos);
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
