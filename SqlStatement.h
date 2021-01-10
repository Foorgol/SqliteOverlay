#pragma once

#include <stdint.h>                       // for int64_t
#include <ctime>                          // for size_t
#include <optional>                       // for optional
#include <string>                         // for string, basic_string
#include <tuple>                          // for tuple, make_tuple
#include <type_traits>                    // for is_same
#include <vector>                         // for vector

#include <sqlite3.h>                      // for sqlite3, sqlite3_stmt

#include <Sloppy/CSV.h>                   // for CSV_Row, CSV_Table
#include <Sloppy/DateTime/DateAndTime.h>  // for WallClockTimepoint_secs
#include <Sloppy/Logger/Logger.h>         // for Logger
#include <Sloppy/Memory.h>                // for MemArray, MemView
#include <Sloppy/json.hpp>                // for json

#include "Defs.h"                         // for ColumnDataType
#include "SqliteExceptions.h"             // for NullValueException

namespace date { class time_zone; }

namespace SqliteOverlay
{  

  /**  \brief A wrapper class for SQL statements
   */
  class SqlStatement
  {
  public:
    /** \brief Default ctor for an SQL statement; the resulting object is
     * similar to a finalized statement and shouldn't be used for any operation.
     */
    SqlStatement();

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
        const std::string& sqlTxt   ///< the statement's SQL text
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

    /** \brief Binds a data of various types to a placeholder in the statement
     *
     * Original documentation [here](https://www.sqlite.org/c3ref/bind_blob.html), including
     * a specification how placeholders are defined in the SQLite language.
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     *
     * Test case: yes
     *
     */
    template<typename T>
    void bind(
        int argPos,   ///< the placeholder to bind to (1-based if you use "?")
        const T& val   ///< the value to bind to the placeholder
        ) const
    {
      static_assert (sizeof(int) == 4, "'int' has to be 32-bit!");

      int e{SQLITE_OK};
      if constexpr (std::is_same_v<T, int>) {
        e = sqlite3_bind_int(stmt, argPos, val);
      }
      else if constexpr (std::is_same_v<T, int64_t>) {
        e = sqlite3_bind_int64(stmt, argPos, val);
      }
      else if constexpr (std::is_same_v<T, double>) {
        e = sqlite3_bind_double(stmt, argPos, val);
      }
      else if constexpr (std::is_same_v<T, std::string>) {
        e = sqlite3_bind_text(stmt, argPos, val.c_str(), val.length(), SQLITE_TRANSIENT);
      }
      else if constexpr (std::is_same_v<T, bool>) {
        e = sqlite3_bind_int(stmt, argPos, val ? 1 : 0);
      }
      else if constexpr (std::is_same_v<T, Sloppy::DateTime::WallClockTimepoint_secs>) {
        // time is always stored in UTC seconds
        e = sqlite3_bind_int64(stmt, argPos, val.to_time_t());
      }
      else if constexpr (std::is_same_v<T, nlohmann::json>) {
        const std::string jsonData = val.dump();
        bind(argPos, jsonData);  // the actual data binding is handled as a string value
      }
      else if constexpr (std::is_same_v<T, Sloppy::MemView>) {
        /** \note SQLite makes an internal copy of the provided buffer; this is safer but
        * also more memory consuming. Bear this in mind when dealing with very large blobs.
        */
        bind(argPos, val.to_voidPtr(), val.byteSize());  // forward the call to the generic bindBlob
      }

      else {
        // a literal 'false' is not possible here because it would
        // trigger the static_assert even if the template has never
        // been instantiated.
        //
        // Thus, we construct a "fake false" that depends on `T` and
        // that is therefore only triggered if we actually instantiate
        // this template.s
        static_assert (!std::is_same<T,T>::value, "SqlStatement: call to bind() with a unsupported value type!");
      }

      if (e != SQLITE_OK)
      {
        throw GenericSqliteException{e, "call to bindInt() of a SqlStatement"};
      }
    }


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
        int argPos,   ///< the placeholder to bind to (1-based if you use "?")
        const char* val   ///< the value to bind to the placeholder
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
    void bind(
        int argPos,   ///< the placeholder to bind to (1-based if you use "?")
        const void* ptr,   ///< a pointer to blob data
        size_t nBytes   ///< number of bytes in the blob data
        ) const;

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
        int argPos   ///< the placeholder to bind to (1-based if you use "?")
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
     * This is just a an alias for `step()`.
     *
     * \returns after the first step: always `true` because we either have result rows
     * or we successfully executed a no-data-query
     *
     * \returns after any subsequent step: `true` if the step produced more data rows; `false` if we're done
     */
    bool operator++()
    {
      return step();
    }

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

    /** \brief Retrieves the value of a column in the statement result in various types
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws NullValueException if the column contains NULL
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column in the requested format
     *
     * Test case: FIX
     *
     */
    template<typename T>
    T get(int colId) const {
      if (isNull(colId))  // implies check and exceptions for assertColumnDataAccess()
      {
        throw NullValueException();
      }

      if constexpr (std::is_same_v<T, int>) {
        return sqlite3_column_int(stmt, colId);
      }
      else if constexpr (std::is_same_v<T, int64_t>) {
        return sqlite3_column_int64(stmt, colId);
      }
      else if constexpr  (std::is_same_v<T, double>) {
        return sqlite3_column_double(stmt, colId);
      }
      else if constexpr (std::is_same_v<T, std::string>) {
        return std::string{reinterpret_cast<const char*>(sqlite3_column_text(stmt, colId))};
      }
      else if constexpr (std::is_same_v<T, nlohmann::json>) {
        return nlohmann::json::parse(sqlite3_column_text(stmt, colId));
      }
      else if constexpr (std::is_same_v<T, Sloppy::MemArray>) {
        // get the number of bytes in the blob
        const size_t nBytes = sqlite3_column_bytes(stmt, colId);
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
        const Sloppy::MemView fakeView{static_cast<const char*>(srcPtr), nBytes};

        return Sloppy::MemArray{fakeView};  // creates a deep copy
      }
      else if constexpr (std::is_same_v<T, Sloppy::DateTime::WallClockTimepoint_secs>) {
        /** \note Creates a timestamp with out time zone (--> UTC is used for
         *  output operations. There is a special overload that takes a time zone
         *  pointer as second argument for local timestamps.
         */
        const time_t rawTime = sqlite3_column_int64(stmt, colId);
        return Sloppy::DateTime::WallClockTimepoint_secs(rawTime);
      }
      else if constexpr (std::is_same_v<T, date::year_month_day>) {
        /** \note The date has to be stored as an integer value, e.g.,
        * 2018-10-28 should be stored as the integer value "20181028".
        */
        return Sloppy::DateTime::ymdFromInt(get<int>(colId));
      }

      else {
        // a literal 'false' is not possible here because it would
        // trigger the static_assert even if the template has never
        // been instantiated.
        //
        // Thus, we construct a "fake false" that depends on `T` and
        // that is therefore only triggered if we actually instantiate
        // this template
        static_assert (!std::is_same_v<T, T>, "Unsupported template parameter for SqlStatement::get<T>");
      }
    }

    /** \brief Retrieves the value of a column in the statement result as custom data type
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column or an empty optional in case of NULL
     *
     * Test case: yes, but without implicit conversion and only with partial exception testing
     */
    template<typename T>
    std::optional<T> get2(int colId) const {
      assertColumnDataAccess(colId);

      if (isNull_NoGuards(colId)) return std::nullopt;

      return get<T>(colId);
    }

    /** \brief Retrieves the value of a column in the statement result as bool value
     *
     * \warning This function assumes that the column content can be converted to an
     * integer value! This is only a wrapper around `get<int>()` that compares the
     * result with `0`.
     *
     * \throws NullValueException if the column contains NULL
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
    bool getBool(
        int colId   ///< the zero-based column ID in the result row
        ) const
    {
      return (get<int>(colId) != 0);
    }


    /** \brief Retrieves the value of a column in the statement result as LocalTimestamp instance;
     * requires the cell content to be an integer with "seconds since epoch".
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws NullValueException if the column contains NULL
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as LocalTimestamp
     *
     * Test case: yes
     *
     */
    Sloppy::DateTime::WallClockTimepoint_secs get(
        int colId,   ///< the zero-based column ID in the result row
        const date::time_zone* tzp   ///< a pointer to the time zone passed to the ctor of the WallClockTimepoint
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
    ColumnDataType getColDataType(
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
    std::string getColName(
        int colId   ///< the zero-based column ID in the result row
        ) const;

    /** \brief Retrieves the value of a column in the statement result as timestamp instance;
     * requires the cell content to be an integer with "seconds since epoch".
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws NullValueException if the column contains NULL
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * \returns the value in the requested result column as LocalTimestamp or an empty optional in case of NULL
     *
     * Test case: yes
     *
     */
    std::optional<Sloppy::DateTime::WallClockTimepoint_secs> get2(
        int colId,   ///< the zero-based column ID in the result row
        const date::time_zone* tzp = nullptr   ///< a pointer to the time zone for the WallClockTimepoint ctor
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
        );

    /** \brief Forcefully finalized the statement, no matter what its current status is; this is similar
     * to calling the dtor on the statement.
     *
     * The function is useful if statements failed (e.g., because the database was busy) and they
     * should not be retried later. Finalizing the statement then releases the associated locks.
     *
     * Test case: yes,implicitly in the dtor and in the transaction test cases
     */
    void forceFinalize();

    /** \returns The expanded SQL statement with all bound parameters; un-bound parameters will
     * be treated as NULL as described [here](https://www.sqlite.org/c3ref/expanded_sql.html)
     *
     * Test case: yes, implicitly in the `bind` test case
     *
     */
    std::string getExpandedSQL() const;



    /** \brief Simple wrapper for retrieving two column values in a tuple.
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * Test case: yes
     *
     */
    template<typename T1, typename T2>
    std::tuple<T1, T2> tupleGet(int col1, int col2) const
    {
      return std::tuple{
            get<T1>(col1),
            get<T2>(col2),
      };
    }

    /** \brief Simple wrapper for retrieving three column values in a tuple.
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * Test case: no
     *
     */
    template<typename T1, typename T2, typename T3>
    std::tuple<T1, T2, T3> tupleGet(int col1, int col2, int col3) const
    {
      return std::tuple{
            get<T1>(col1),
            get<T2>(col2),
            get<T3>(col3),
      };
    }

    /** \brief Simple wrapper for retrieving four column values in a tuple.
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \throws InvalidColumnException if the requested column does not exist
     *
     * Test case: no
     *
     */
    template<typename T1, typename T2, typename T3, typename T4>
    std::tuple<T1, T2, T3, T4> tupleGet(int col1, int col2, int col3, int col4) const
    {
      return std::tuple{
            get<T1>(col1),
            get<T2>(col2),
            get<T3>(col3),
            get<T4>(col4),
      };
    }

    /** \returns the number of data columns in the result data set or -1 if the statement
     * does not contain any data.
     *
     * \note You must call `step()` at least once before this function returns a reasonable
     * value. If `hasData()` is `false` then this function always returns -1.
     */
    int nDataColumns() const { return resultColCount; }

    /** \brief Exports all data that is yielded by this statement as a CSV_Table; the statement
     * will be fully "exhausted": internally we call `step()` until the statement is finalized.
     *
     * If `step()` has been called before on this statement, we return everything from the
     * current row (= before the next `step()`) until the statement is finalized.
     *
     * In order to leave the statement in a consistent state we will always (force-)finalize
     * the statement before we return, even if an error or an exception occurs.
     *
     * \note The implementation of this function is not optimized for large quantities of data.
     *
     * \warning If the SQL statement does not yield any data rows, we'll return a completely empty
     * table WITHOUT headers, even if the inclusion of headers was requested by the caller!
     *
     * \throws InvalidColumnException if any of the result columns contains BLOB data because
     * that can't be properly exported to CSV.
     *
     * \throws InvalidColumnException if any of the result columns has an invalid name (not unique,
     * contains commas or contains quotation marks) and headers names shall be included in the CSV table.
     *
     * \throws NoDataException if the statement was already finalized when calling this function.
     *
     * \returns a (potentially empty) CSV_Table instance with the results of the statement
     */
    Sloppy::CSV_Table toCSV(
        bool includeHeaders   ///< include column headers in the first CSV table row yes/no
        );

    /** \brief Exports the current data row as a CSV_Row; the statement itself is not modified
     * (i.e., we do not call `step()` or 'forceFinalize()` on the statement at any time).
     *
     * \throws InvalidColumnException if any of the result columns contains BLOB data because
     * that can't be properly exported to CSV.
     *
     * \throws NoDataException if the statement was already finalized when calling this function or if
     * the statement didn't point to a data row or if the result set does not contain at least one column.
     *
     * \returns a CSV_Row instance with the results of the statement
     */
    Sloppy::CSV_Row toCSV_currentRowOnly() const;

    /** \brief Returns a list of strings that contains the names of the
     * column in the statement's result set.
     *
     * The statement has to point to a valid data row otherwise this
     * function will fail.
     *
     * \throws NoDataException if the statement was already finalized when calling this function or if
     * the statement didn't point to a data row.
     *
     * \returns a string list containing the column names
     */
    std::vector<std::string> columnHeaders() const;



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

    /** \brief Just like `isNull` but without parameter validity checking
     *
     * For lib-internal use only.
     *
     */
    bool isNull_NoGuards(
        int colId   ///< the zero-based column ID in the result row
        ) const;

  private:
    sqlite3_stmt* stmt{nullptr};
    bool _hasData{false};
    bool _isDone;
    int resultColCount{-1};
    int stepCount{0};
  };
}
