#ifndef SQLITE_OVERLAY_SQLITE_EXCEPTIONS_H
#define SQLITE_OVERLAY_SQLITE_EXCEPTIONS_H

#include <string>
#include <iostream>
#include <optional>
#include <string>

#include <sqlite3.h>

#include "Defs.h"

namespace SqliteOverlay
{
  class BasicException
  {
  public:
    BasicException(
        const std::string& exName,
        std::optional<PrimaryResultCode> errCode = std::optional<PrimaryResultCode>{},
        const std::string& context = "")
      :err{errCode}
    {
      msg = "Exception in SqliteOverlay lib:\n";
      msg += "  Exception: " + exName + "\n";
      if (!(context.empty()))
      {
        msg += "  Context: " + context + "\n";
      }
      if (errCode.has_value())
      {
        int e = static_cast<int>(err.value());
        std::string sErr{sqlite3_errstr(e)};
        msg += "  SQLite error: " + sErr + "(" + std::to_string(e) + ")\n";
      }

      std::cerr << msg << "\n" << std::endl;
    }

    BasicException(const std::string& exName, int errCode, const std::string& context = "")
      :BasicException(exName, static_cast<PrimaryResultCode>(errCode), context) {}

    BasicException(const std::string& exName, const std::string& context = "")
      :BasicException(exName, std::optional<PrimaryResultCode>{}, context) {}

    std::string what() const { return msg; }

    PrimaryResultCode errCode() const { return err.value(); }

  private:
    std::string msg;
    std::optional<PrimaryResultCode> err;
  };

  /** \brief An exception that is thrown in case SQLite returns an error
   * and we don't have a more specific exception at hand.
   *
   * \note Providing the SQLite error code is a MUST for this exception!
   */
  class GenericSqliteException : public BasicException
  {
  public:
    GenericSqliteException(int errCode, const std::string& context = "")
      :BasicException("Generic SQLite API error", errCode, context) {}
  };

  /** \brief An exception for SQL statement creation failures */
  class SqlStatementCreationError : public BasicException
  {
  public:
    SqlStatementCreationError(int errCode, const std::string& sqlTxt, const char* sqliteErrMsg)
      :BasicException("SqlStatement Creation Error", errCode, std::string{sqliteErrMsg} + "\n  SQL Statement: " + sqlTxt) {}
  };

  /** \brief An exception that is thrown if SQL statements couldn't be executed
   * because the database was busy.
   */
  class BusyException : public BasicException
  {
  public:
    BusyException(const std::string& context = "")
      :BasicException("Database Busy Error", SQLITE_BUSY, context) {}
  };

  /** \brief An exception for requests to invalid / non-existing columns
   */
  class InvalidColumnException : public BasicException
  {
  public:
    InvalidColumnException(const std::string& context = "")
      :BasicException("Invalid column", context) {}
  };

  /** \brief An exception that is thrown upon trying to access SQL statement
   * results that didn't return any data at all or that are finished.
   */
  class NoDataException : public BasicException
  {
  public:
    NoDataException(const std::string& context = "")
      :BasicException("Column data access in a SQL statement that did not return any data or that is finished", context) {}
  };

  /** \brief An exception that is thrown if we expected a cell to contain
   * a real, scalar value but it contained NULL instead
   */
  class NullValueException : public BasicException
  {
  public:
    NullValueException(const std::string& context = "")
      :BasicException("Null Value", context) {}
  };

  /** \brief An exception that is thrown if an invalid,
   * non-existing table name is used
   */
  class NoSuchTableException : public BasicException
  {
  public:
    NoSuchTableException(const std::string& context = "")
      :BasicException("No Such Table (e.g., invalid table name", context) {}
  };

  /** \brief An exception that is thrown if a table constraint (e.g., foreign key) would be violated
   */
  class ConstraintFailedException : public BasicException
  {
  public:
    ConstraintFailedException(const std::string& context = "")
      :BasicException("Constraint Failed (e.g., foreign key violation)", context) {}
  };
}
#endif
