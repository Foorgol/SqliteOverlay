#ifndef SQLITE_OVERLAY_SQLSTATEMENT_H
#define SQLITE_OVERLAY_SQLSTATEMENT_H

#include <string>
#include <memory>
#include <ctime>

#include <sqlite3.h>

#include <Sloppy/DateTime/DateAndTime.h>
#include <Sloppy/Logger/Logger.h>

#include "Defs.h"

using namespace std;
using namespace Sloppy::DateTime;
using namespace Sloppy::Logger;

namespace SqliteOverlay
{  

  /**  \brief A wrapper class for SQL statements
   */
  class SqlStatement
  {
  public:
    /** \brief Standard ctor for a new SQL statement
     *
     * \throws std::invalid_argument if one or both of the provided parameters are empty
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     */
    SqlStatement(
        sqlite3* dbPtr,   ///< the pointer to the raw database connection for which the statement should be created
        const string& sqlTxt   ///< the statement's SQL text
        );

    /** \brief Dtor, finalizes the statment if it hasn't been finalized already
     */
    ~SqlStatement();

    /** \brief Disabled copy ctor */
    SqlStatement(const SqlStatement& other) = delete;

    /** \brief Disabled copy assignment */
    SqlStatement& operator=(const SqlStatement& other) = delete;

    /** \brief Standard move ctor */
    SqlStatement(SqlStatement&& other) = default;

    /** \brief Standard move assignment */
    SqlStatement& operator=(SqlStatement& other) = default;

    /** \brief Binds an int value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        int val   ///< the value to bind to the placeholder
        ) const;

    /** \brief Binds a double value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        double val   ///< the value to bind to the placeholder
        ) const;

    /** \brief Binds a string value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        const string& val   ///< the value to bind to the placeholder
        ) const;

    /** \brief Binds a float value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        float val   ///< the value to bind to the placeholder
        ) const
    {
      bind(argPos, static_cast<double>(val));
    }

    /** \brief Binds a boolean value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        bool val   ///< the value to bind to the placeholder
        ) const
    {
      bind(argPos, val ? 1 : 0);
    }

    /** \brief Catches all calls to `bind()` with unsupported
     * value types at compile time.
     */
    template<typename T>
    void bind(int argPos, T val) const
    {
      static_assert (false, "SqlStatement: call to bind() with a unsupported value type!");
    }

    /** \brief Binds a NULL value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     */
    void bindNull(
        int argPos   ///< the placeholder to bind to
        ) const;

    /** \brief Executes the next step of the SQL statement
     *
     * \note It is okay to execute this statement in a loop until it returns `false`. If
     * there are no more steps to execute, a call to this function is harmless.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns `true` if the statement was executed successfully or `false` if there was nothing more to execute
     */
    bool step();

    /** \returns `true` if the last call to `step()` returned row data
     */
    bool hasData() const;

    /** \returns `true` if the statement has been completely executed
     */
    bool isDone() const;

    /** \brief Retrieves the value of a column in the statement result as int value
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as int
     */
    int getInt(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as double value
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as double
     */
    double getDouble(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as string value
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as string
     */
    string getString(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as LocalTimestamp instance
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as LocalTimestamp
     */
    LocalTimestamp getLocalTime(
        int colId,   ///< the zero-based column ID in the result row
        boost::local_time::time_zone_ptr tzp   ///< a pointer to the time zone for the local time
        ) const;

    /** \brief Retrieves the value of a column in the statement result as UTCTimestamp instance
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as UTCTimestamp
     */
    UTCTimestamp getUTCTime(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the fundamental SQLite data type for a column in the result row
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the fundamental SQLite data type for the column
     */
    ColumnDataType getColType(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the name of a column in the result row
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the name of the column
     */
    string getColName(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \returns `true` if the requested column in the result row is empty (NULL)
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     */
    bool isNull(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Resets a statement so that it can be executed all over again
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     */
    void reset(
        bool clearBindings   ///< `true`: clear existing placeholder bindings; `false`: bindings keep their values
        ) const;

  protected:
    /** \brief "Guard function" that checks preconditions for
     * the getXXX methods and throws exceptions if necessary
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     */
    void assertColumnDataAccess(
        int colId   ///< the zero-based column ID that is to be checked
        ) const;

  private:
    sqlite3_stmt* stmt;
    bool _hasData;
    bool _isDone;
    int resultColCount;
    int stepCount;
  };
}

#endif /* SQLSTATEMENT_H */
