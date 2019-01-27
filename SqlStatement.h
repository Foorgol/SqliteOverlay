#ifndef SQLITE_OVERLAY_SQLSTATEMENT_H
#define SQLITE_OVERLAY_SQLSTATEMENT_H

#include <string>
#include <memory>
#include <ctime>

#include <sqlite3.h>

#include <Sloppy/DateTime/DateAndTime.h>
#include <Sloppy/Memory.h>
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
     *
     * Test case: implicitly by the database query tests; does not include exception testing
     *
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

    /** \brief Move ctor, transfers the statement pointer after finalizing any old,
     * pending statements
     *
     * Test case: not yet
     *
     */
    SqlStatement(SqlStatement&& other);

    /** \brief Move assigment, transfers the statement pointer after finalizing any old,
     * pending statements
     *
     * Test case: implicitly by test case `StmtBind`
     *
     */
    SqlStatement& operator=(SqlStatement&& other);

    /** \brief Catches all calls to `bind()` with unsupported
     * value types at compile time.
     */
    template<typename T>
    void bind(int argPos, T val) const
    {
      // a literal 'false' is not possible here because it would
      // trigger the static_assert even if the template has never
      // been instantiated.
      //
      // Thus, we construct a "fake false" that depends on `T` and
      // that is therefore only triggered if we actually instantiate
      // this template.s
      static_assert (!is_same<T,T>::value, "SqlStatement: call to bind() with a unsupported value type!");
    }

    /** \brief Binds an int value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        int val   ///< the value to bind to the placeholder
        ) const;

    /** \brief Binds a long value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        long val   ///< the value to bind to the placeholder
        ) const;

    /** \brief Binds a double value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
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
     *
     * Test case: yes
     *
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        const string& val   ///< the value to bind to the placeholder
        ) const;

    /** \brief Binds a *zero-terminated* C-string to a placeholder in the statement
     *
     * \note The string length will be determined using `strlen()` so make sure that
     * the string is properly zero-terminated.
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        const char* val   ///< the value to bind to the placeholder
        ) const;

    /** \brief Binds a boolean value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(
        int argPos,   ///< the placeholder to bind to
        bool val   ///< the value to bind to the placeholder
        ) const
    {
      bind(argPos, val ? 1 : 0);
    }

    /** \brief Binds a timestamp to a placeholder in the statement; time is always stored in UTC seconds
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(int argPos,   ///< the placeholder to bind to
              const LocalTimestamp& val   ///< the value to bind to the placeholder
              ) const
    {
      bind(argPos, val.getRawTime());  // forwards the call to a int64-bind
    }

    /** \brief Binds a timestamp to a placeholder in the statement; time is always stored in UTC seconds
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(int argPos,   ///< the placeholder to bind to
              const UTCTimestamp& val   ///< the value to bind to the placeholder
              ) const
    {
      bind(argPos, val.getRawTime());  // forwards the call to a int64-bind
    }

    /** \brief Binds a blob of data to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \note SQLite makes an internal copy of the provided buffer; this is safer but
     * also more memory consuming. Bear this in mind when dealing with very large blobs.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(int argPos,   ///< the placeholder to bind to
              const void* ptr,   ///< a pointer to blob data
              size_t nBytes   ///< number of bytes in the blob data
              ) const;

    /** \brief Binds a blob of data to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \note SQLite makes an internal copy of the provided buffer; this is safer but
     * also more memory consuming. Bear this in mind when dealing with very large blobs.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    void bind(int argPos,   ///< the placeholder to bind to
              const Sloppy::MemView& v   ///< the buffer that shall be stored
              ) const
    {
      bind(argPos, v.to_voidPtr(), v.byteSize());  // forward the call to the generic bindBlob
    }

    /** \brief Binds a NULL value to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
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
     * \returns after the first step: always `true` because we either have result rows
     * or we successfully executed a no-data-query
     *
     * \returns after any subsequent step: `true` if the step produced more data rows; `false` if we're done
     *
     * Test case: yes, but onyl with partial exception testing
     *
     */
    bool step();

    /** \brief Executes the next step of the SQL statement
     *
     * This is nothing but a wrapper around `step()` with a modified return value.
     * Unlike `step()`, `dataStep()` returns `false` even after the first step if this
     * first step didn't produce any data rows.
     *
     * `dataStep()` is useful for a `while` loop. That loop would be executed zero or
     * more times for each data row produced by the SQL statement.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns `true` if the step produced a data row, `false` otherwise
     *
     * Test case: yes, but onyl with partial exception testing
     *
     */
    bool dataStep()
    {
      step();
      return _hasData;
    }

    /** \returns `true` if the last call to `step()` returned row data
     *
     * Test case: yes
     *
     */
    bool hasData() const;

    /** \returns `true` if the statement has been completely executed
     *
     * Test case: yes
     *
     */
    bool isDone() const;

    /** \brief Retrieves the value of a column in the statement result as int value (32 bit)
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as int
     *
     * Test case: yes, but without implicit conversion and only with partial exception testing
     *
     */
    int getInt(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as long value (64 bit)
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as int
     *
     * Test case: yes
     *
     */
    long getLong(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as double value
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as double
     *
     * Test case: yes, but without implicit conversion and only with partial exception testing
     *
     */
    double getDouble(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as bool value
     *
     * \warning This function assumes that the column content can be converted to an
     * integer value! This is only a wrapper around `getInt()` that compares the
     * result with `0`.
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns `false` if the column value is zero, `true` if it is not zero
     *
     * Test case: yes, but without implicit conversion and only with partial exception testing
     *
     */
    double getBool(
        int colId   ///< the zero-based column ID in the result row
        ) const
    {
      return (getInt(colId) != 0);
    }


    /** \brief Retrieves the value of a column in the statement result as string value
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as string
     *
     * Test case: yes, but without implicit conversion and only with partial exception testing
     *
     */
    string getString(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as LocalTimestamp instance;
     * requires the cell content to be an integer with "seconds since epoch".
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as LocalTimestamp
     *
     * Test case: yes
     *
     */
    LocalTimestamp getLocalTime(
        int colId,   ///< the zero-based column ID in the result row
        boost::local_time::time_zone_ptr tzp   ///< a pointer to the time zone for the local time
        ) const;

    /** \brief Retrieves the value of a column in the statement result as UTCTimestamp instance;
     * requires the cell content to be an integer with "seconds since epoch".
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as UTCTimestamp
     *
     * Test case: yes
     *
     */
    UTCTimestamp getUTCTime(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as a data blob.
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \throws All exceptions that the ctor of `Sloppy::MemArray` can produce (i. e., if we run out of memory)
     *
     * \returns the whole blob contents in one heap-allocated buffer
     *
     * Test case: yes
     *
     */
    Sloppy::MemArray getBlob(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the fundamental SQLite data type for a column in the result row
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the fundamental SQLite data type for the column
     *
     * Test case: yes
     *
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
     *
     * Test case: not yet
     *
     */
    string getColName(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \returns `true` if the requested column in the result row is empty (NULL)
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * Test case: yes, implicitly in the `getters` test case
     *
     */
    bool isNull(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Resets a statement so that it can be executed all over again
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes, implicitly in the `getters` test case
     *
     */
    void reset(
        bool clearBindings   ///< `true`: clear existing placeholder bindings; `false`: bindings keep their values
        ) const;

    /** \brief Forcefully finalized the statement, no matter what its current status is; this is similar
     * to calling the dtor on the statement.
     *
     * The function is useful if statements failed (e.g., because the database was busy) and they
     * should not be retried later. Finalizing the statement then releases the associated locks.
     *
     * Test case: yes,implicitly in the dtor and in the transactio test cases
     */
    void forceFinalize();

    /** \returns The expanded SQL statement with all bound parameters; un-bound parameters will
     * be treated as NULL as described [here](https://www.sqlite.org/c3ref/expanded_sql.html)
     *
     * Test case: yes, implicitly in the `bind` test case
     *
     */
    string getExpandedSQL() const
    {
      return string{sqlite3_expanded_sql(stmt)};
    }

  protected:
    /** \brief "Guard function" that checks preconditions for
     * the getXXX methods and throws exceptions if necessary
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * Test case: yes, implicitly in the `getters` test case
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
