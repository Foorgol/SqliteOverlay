#ifndef SQLITE_OVERLAY_SQLITE_EXCEPTIONS_H
#define SQLITE_OVERLAY_SQLITE_EXCEPTIONS_H

#include <string>
#include <iostream>
#include <optional>
#include <string>

#include <sqlite3.h>

#include "Defs.h"

using namespace std;

namespace SqliteOverlay
{
  class BasicException
  {
  public:
    BasicException(const string& exName, optional<PrimaryResultCode> errCode = optional<PrimaryResultCode>{}, const string& context = "")
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
        string sErr{sqlite3_errstr(e)};
        msg += "  SQLite error: " + sErr + "(" + to_string(e) + ")\n";
      }

      cerr << msg << "\n" << endl;
    }

    BasicException(const string& exName, int errCode, const string& context = "")
      :BasicException(exName, static_cast<PrimaryResultCode>(errCode), context) {}

    BasicException(const string& exName, const string& context = "")
      :BasicException(exName, optional<PrimaryResultCode>{}, context) {}

    string what() const { return msg; }

    PrimaryResultCode errCode() const { return err.value(); }

  private:
    string msg;
    optional<PrimaryResultCode> err;
  };

  /** \brief An exception that is thrown in case SQLite returns an error
   * and we don't have a more specific exception at hand.
   *
   * \note Providing the SQLite error code is a MUST for this exception!
   */
  class GenericSqliteException : public BasicException
  {
  public:
    GenericSqliteException(int errCode, const string& context = "")
      :BasicException("Generic SQLite API error", errCode, context) {}
  };

  /** \brief An exception for SQL statement creation failures */
  class SqlStatementCreationError : public BasicException
  {
  public:
    SqlStatementCreationError(int errCode, const string& sqlTxt, const char* sqliteErrMsg)
      :BasicException("SqlStatement Creation Error", errCode, string{sqliteErrMsg} + "\n  SQL Statement: " + sqlTxt) {}
  };

  /** \brief An exception that is thrown if SQL statements couldn't be executed
   * because the database was busy.
   */
  class BusyException : public BasicException
  {
  public:
    BusyException(const string& context = "")
      :BasicException("Database Busy Error", SQLITE_BUSY, context) {}
  };

  /** \brief An exception for requests to invalid / non-existing columns
   */
  class InvalidColumnException : public BasicException
  {
  public:
    InvalidColumnException(const string& context = "")
      :BasicException("Invalid column exception", context) {}
  };

  /** \brief An exception that is thrown upon trying to access SQL statement
   * results that didn't return any data at all or that are finished.
   */
  class NoDataException : public BasicException
  {
  public:
    NoDataException(const string& context = "")
      :BasicException("Column data access in a SQL statement that did not return any data or that is finished", context) {}
  };

  /** \brief An exception that is thrown if we expected a cell to contain
   * a real, scalar value but it contained NULL instead
   */
  class NullValueException : public BasicException
  {
  public:
    NullValueException(const string& context = "")
      :BasicException("Null Value Exception", context) {}
  };

  /** \brief An exception that is thrown if an invalid,
   * non-existing table name is used
   */
  class NoSuchTableException : public BasicException
  {
  public:
    NoSuchTableException(const string& context = "")
      :BasicException("No Such Table Exception (e.g., invalid table name", context) {}
  };
}
#endif
