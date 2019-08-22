#ifndef SQLITE_OVERLAY_SQLITEDATABASE_H
#define	SQLITE_OVERLAY_SQLITEDATABASE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <type_traits>

#include <sqlite3.h>

#include <Sloppy/Utils.h>

#include "SqlStatement.h"
#include "SqliteExceptions.h"
#include "Defs.h"
#include "Changelog.h"

namespace SqliteOverlay
{
  // a forward definition
  class KeyValueTab;

  // some free functions, mostly for creating SQL strings

  /** \brief Creates a column constraint string for CREATE TABLE statements
   * according to [this description](https://www.sqlite.org/syntax/column-constraint.html).
   *
   * Test case: implicit in `addColumn_foreignKey` of `DbTab`
   *
   * \returns a string with a column constraint clause for use in a CREATE TABLE statement.
   */
  std::string buildColumnConstraint(
      ConflictClause uniqueConflictClause,   ///< unless set to `NotUsed` include a UNIQUE constraint and define what shall happen in violation
      ConflictClause notNullConflictClause   ///< unless set to `NotUsed` include a NOT NULL constraint and define what shall happen in violation
      );

  /** \brief Creates a column constraint string for CREATE TABLE statements
   * according to [this description](https://www.sqlite.org/syntax/column-constraint.html).
   *
   * Test case: implicit in `addColumn_foreignKey` of `DbTab`
   *
   * \returns a string with a column constraint clause for use in a CREATE TABLE statement.
   */
  template <typename T>
  std::string buildColumnConstraint(
      ConflictClause uniqueConflictClause,   ///< unless set to `NotUsed` include a UNIQUE constraint and define what shall happen in violation
      ConflictClause notNullConflictClause,   ///< unless set to `NotUsed` include a NOT NULL constraint and define what shall happen in violation
      const T& defaultVal   ///< if `true`, we add a placeholder "?" for the default column value
      )
  {
    std::string result = buildColumnConstraint(uniqueConflictClause, notNullConflictClause);

    if (!(result.empty())) result += " ";

    result += "DEFAULT " + std::to_string(defaultVal);

    return result;
  }

  /** \brief Creates a column constraint string for CREATE TABLE statements
   * according to [this description](https://www.sqlite.org/syntax/column-constraint.html).
   *
   * Test case: implicit in `addColumn_foreignKey` of `DbTab`
   *
   * \returns a string with a column constraint clause for use in a CREATE TABLE statement.
   */
  std::string buildColumnConstraint(
      ConflictClause uniqueConflictClause,   ///< unless set to `NotUsed` include a UNIQUE constraint and define what shall happen in violation
      ConflictClause notNullConflictClause,   ///< unless set to `NotUsed` include a NOT NULL constraint and define what shall happen in violation
      const std::string& defaulVal   ///< if `true`, we add a placeholder "?" for the default column value
      );

  /** \brief Creates a column constraint string for CREATE TABLE statements
   * according to [this description](https://www.sqlite.org/syntax/column-constraint.html).
   *
   * Test case: implicit in `addColumn_foreignKey` of `DbTab`
   *
   * \returns a string with a column constraint clause for use in a CREATE TABLE statement.
   */
  std::string buildColumnConstraint(
      ConflictClause uniqueConflictClause,   ///< unless set to `NotUsed` include a UNIQUE constraint and define what shall happen in violation
      ConflictClause notNullConflictClause,   ///< unless set to `NotUsed` include a NOT NULL constraint and define what shall happen in violation
      const char* defaulVal   ///< if `true`, we add a placeholder "?" for the default column value
      );


  //----------------------------------------------------------------------------

  /** \brief Creates a foreign key clause for CREATE TABLE statements
   * according to [this description](https://www.sqlite.org/syntax/foreign-key-clause.html).
   *
   * Test case: not yet
   *
   * \returns a string with a foreign key clause for use in a CREATE TABLE statement.
   */
  std::string buildForeignKeyClause(
      const std::string& referedTable,   ///< the name of the table that this column refers to
      ConsistencyAction onDelete,   ///< the action to be taken if the refered row is being deleted
      ConsistencyAction onUpdate,   ///< the action to be taken if the refered row is updated
      std::string referedColumn="id"   ///< the name of the column we're pointing to in the refered table
      );

  //----------------------------------------------------------------------------

  /** \returns the type affinity for a given column type string according to the
   * type affinity rules [specified here](https://www.sqlite.org/datatype3.html)
   *
   * Test case: not yet
   *
   */
  ColumnAffinity string2Affinity(const std::string& colType);

  //----------------------------------------------------------------------------


  //
  // forward definitions
  //

  class DbTab;
  class Transaction;

  //----------------------------------------------------------------------------

  /** \brief A class that handles a single database connection.
   *
   * \note If an instance of this class is shared by multiple threads you have to make sure on
   * application level that no bad things happen. From SQLite's perspective, all
   * statements are coming in over the same connection, no matter which thread
   * actually sent them.
   *
   * \note This object is not inherently thread-safe. It uses no locking, mutexes
   * or other magic before reading/writing its internal state.
   */
  class SqliteDatabase
  {
  public:
    /** \brief Default ctor, creates a blank in-memory database.
     *
     * \note `populateTables()` and `populateViews()` are NOT CALLED from
     * this constructor. Since they are virtual methods that should be refined
     * by derived classes, the base class ctor is not able to properly call them!
     *
     * Test case: not yet
     *
     */
    SqliteDatabase();

    /** \brief Standard dtor for creating or opening a database file.
     *
     * \note `populateTables()` and `populateViews()` are NOT CALLED from
     * this constructor. Since they are virtual methods that should be refined
     * by derived classes, the base class ctor is not able to properly call them!
     *
     * \throws std::invalid_argument if the filename is empty or if a
     * parameter combination makes no sense (e.g., read-only access to a
     * blank in-memory database, force-create an existing file or strict open
     * on a non-existing file).
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     * with SQLite
     *
     * Test case: yes
     *
     */
    SqliteDatabase(
        std::string dbFilename, ///< the name of the database file to open or create
        OpenMode om   ///< the opening mode (e.g., read-only)
        );

    /** \brief Dtor; closes the database connections and cleans up table cache
     *
     * Test case: not yet
     *
     */
    virtual ~SqliteDatabase();

    /** \brief Disabled copy ctor */
    SqliteDatabase(const SqliteDatabase&) = delete;

    /** \brief Disabled copy assignment */
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    /** \brief Move ctor
     *
     * \warning Do not move database objects (neither as
     * source nor target) if they are shared between threads or
     * mysterious things might happen.
     *
     * \note If the changelog was enabled before the move operation
     * we try to flawlessly move that over to the new object. But
     * this has not been well tested so far. Good luck.
     */
    SqliteDatabase(
        SqliteDatabase&& other   ///< the exising object from we construct a new one
        );

    /** \brief Move assignment operator
     *
     * For the possible exceptions see also the description
     * of `close()` because internally we call `close()` on
     * the existing connection before overwriting it with the
     * other connection.
     *
     * \warning Do not move database objects (neither as
     * source nor target) if they are shared between threads or
     * mysterious things might happen.
     *
     * \note If the changelog was enabled before the move operation
     * we try to flawlessly move that over to the new object. But
     * this has not been well tested so far. Good luck.
     *
     * \throws BusyException if the target instance could not
     * successfully `close()` its connection.
     *
     * \throws BusyException if the target instance could not
     * successfully `close()` its connection.
     *
     */
    SqliteDatabase& operator=(
        SqliteDatabase&&   ///< the source that shall be moved
        );


    /** \brief Closes the database connection. The instance (or any
     * dangling references to it) shouldn't be used anymore after calling this function.
     *
     * \warning Pending transactions will be rolled back by calling `close()`!
     *
     * \throws BusyException if the database has still un-finalized prepared statements
     * or ongoing backup operations pending.
     *
     * \throws GenericSqliteException if anything else should go wrong.
     *
     * Test case: partially, as part of the ctor tests
     *
     */
    void close();

    /** \returns `true` if the database connection is still open and `false` otherwise
     *
     * Test case: partially, as part of the ctor tests
     *
     */
    bool isAlive() const;

    /** \brief Creates a new SQL statement for this database connection
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \returns a SqlStatement instance for the provided SQL text
     *
     * Test case: partially, as part of the database query tests
     *
     */
    SqlStatement prepStatement(
        const std::string& sqlText   ///< the SQL text for which to create the statement
        ) const;

    /** \brief Executes a SQL statement that isn't expected to return any data.
     *
     * If the SQL statement consists of multiple steps, all steps are executed
     * until the whole statement is finished.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: implicitly in many other test cases
     *
     */
    void execNonQuery(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a SQL statement that isn't expected to return any data.
     *
     * If the SQL statement consists of multiple steps, all steps are executed
     * until the whole statement is finished.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: implicitly in many other test cases
     *
     */
    void execNonQuery(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement that returns arbitrary data
     *
     * The first call to `step()` of the resulting SqlStatement is already
     * executed by this function. Thus, the caller can directly start evaluating
     * the first query results.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes, but no exception testing
     *
     */
    SqlStatement execContentQuery(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The column should contain a real value and not NULL
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the first result row as int
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    int execScalarQueryInt(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the result row as int
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    int execScalarQueryInt(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<int>` containing the first value in the first result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<int> execScalarQueryIntOrNull(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<int>` containing the first value in the result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<int> execScalarQueryIntOrNull(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The column should contain a real value and not NULL
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the first result row as `long`
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    long execScalarQueryLong(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the result row as `long`
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    long execScalarQueryLong(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<long>` containing the first value in the first result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<long> execScalarQueryLongOrNull(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<int>` containing the first value in the result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<long> execScalarQueryLongOrNull(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the first result row as double
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    double execScalarQueryDouble(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the result row as double
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    double execScalarQueryDouble(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<double>` containing the first value in the first result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<double> execScalarQueryDoubleOrNull(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<double>` containing the first value in the result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<double> execScalarQueryDoubleOrNull(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the first result row as string
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::string execScalarQueryString(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws NullValueException if the query returned NULL instead of a regular value
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the first value in the result row as string
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::string execScalarQueryString(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;

    /** \brief Executes a SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<string>` containing the first value in the first result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<std::string> execScalarQueryStringOrNull(
        const std::string& sqlStatement   ///< the SQL statement to execute
        ) const;

    /** \brief Executes a prepared SQL statement by calling `step()` once and returns the value
     * in the first of column (index 0) of the returned row; all other rows and columns are ignored.
     *
     * \note The provided statement is modified in place and can be used in subsequent calls
     * to `execScalarQueryInt` (or other) for retrieving more rows.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws NoDataException if the SQL statement didn't return any data
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an `optional<string>` containing the first value in the result row; if the cell
     * contained NULL the return value is empty.
     *
     * Test case: yes, but only with partial exception testing
     *
     */
    std::optional<std::string> execScalarQueryStringOrNull(
        SqlStatement& stmt   ///< a prepared statement, ready for execution
        ) const;


    /** \brief Switches synchronous writes on or off
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: not explicitly, but it is called/tested implicitly in the ctor
     *
     */
    void enforceSynchronousWrites(
        bool syncOn   ///< `true`: activate sync write, `false`: deactivate sync writes
        ) const;

    /** \brief Hook for derived database classes to fill in their code for populating
     * all database tables.
     *
     * This hook is called whenever a database file is opened or created.
     */
    virtual void populateTables() {}

    /** \brief Hook for derived database classes to fill in their code for populating
     * all database tables.
     *
     * This hook is called whenever a database file is opened or created.
     */
    virtual void populateViews() {}

    /** \brief Convenience function for easy creation of new views.
     *
     * \note If a view of the same name already exists, nothing happens.
     *
     * See `execNonQuery()` for possible exceptions.
     *
     * Test case: not yet
     *
     */
    void viewCreationHelper(
        const std::string& viewName,   ///< the name for the new view
        const std::string& selectStmt   ///< the SQL statement that represents the view's contents
        ) const;

    /** \brief Convenience function for creating a new index that combines several columns
     * of one table.
     *
     * If a view of the same name already exists, nothing happens.
     *
     * If any of the provided arguments (table name, index name, column list) is empty,
     * the function returns without error and without doing anything.
     *
     * \throws NoSuchTableException if no table of the provided name exists
     *
     * See `execNonQuery()` for other possible exceptions.
     *
     * Test case: not yet
     *
     */
    void indexCreationHelper(
        const std::string& tabName,   ///< the table on which to create the new index
        const std::string& idxName,   ///< the name of the new index
        const Sloppy::StringList& colNames,   ///< a list of column names that shall be ANDed for the index
        bool isUnique=false   ///< determines whether the index shall enforce unique combinations of the column values
        ) const;

    /** \brief Convenience function for creating a new index on a single column
     * of one table.
     *
     * If a view of the same name already exists, nothing happens.
     *
     * If any of the provided arguments (table name, index name, column name) is empty,
     * the function returns without error and without doing anything.
     *
     * \throws NoSuchTableException if no table of the provided name exists
     *
     * See `execNonQuery()` for other possible exceptions.
     *
     * Test case: not yet
     *
     */
    void indexCreationHelper(
        const std::string& tabName,   ///< the table on which to create the new index
        const std::string& idxName,   ///< the name of the new index
        const std::string& colName,   ///< the name of the column which should serve as an index
        bool isUnique=false   ///< determines whether the index shall enforce unique column values
        ) const;

    /** \brief Convenience function for creating a new index with a canonically created name
     * on a single column of one table.
     *
     * If a view of the same name already exists, nothing happens.
     *
     * If any of the provided arguments (table name, index name, column name) is empty,
     * the function returns without error and without doing anything.
     *
     * \throws NoSuchTableException if no table of the provided name exists
     *
     * See `execNonQuery()` for other possible exceptions.
     *
     * Test case: not yet
     *
     */
    void indexCreationHelper(
        const std::string& tabName,   ///< the table on which to create the new index
        const std::string& colName,   ///< the name of the column which should serve as an index
        bool isUnique=false   ///< determines whether the index shall enforce unique column values
        ) const;

    /** \returns a list of names of all tables or views in the DB
     *
     * Test case: yes
     *
     */
    Sloppy::StringList allTableNames(
        bool getViews=false   ///< `false`: get table names (default); `true`: get view names
        ) const;

    /** \returns a list of names of all views in the DB
     *
     * Test case: yes
     *
     */
    Sloppy::StringList allViewNames() const;

    /** \returns `true` if a table or view of the given name exists in the database
     *
     * Test case: implicitly in `StmtBlob` and also explicitly
     *
     */
    bool hasTable(
        const std::string& name,   ///< the name to search for (case-sensitive)
        bool isView=false   ///< `false`: search among all tables (default); `true`: search among all views
        ) const;

    /** \returns `true` if a view of the given name exists in the database
     *
     * Test case: yes
     *
     */
    bool hasView(
        const std::string& name   ///< the name to search for (case-sensitive)
        ) const;

    /** \returns the ID of the last inserted row
     *
     * See also [here](https://www.sqlite.org/c3ref/last_insert_rowid.html)
     *
     * Test case: yes
     *
     */
    int getLastInsertId() const;

    /** \returns the number of rows modified, inserted or deleted by the most recently completed INSERT, UPDATE or DELETE statement
     *
     * See also [here](https://www.sqlite.org/c3ref/changes.html)
     *
     * Test case: yes
     *
     */
    int getRowsAffected() const;

    /** \returns `true` if the database is in autocommit mode (read: no active transaction running), `false` otherwise.
     *
     * See also [here](https://www.sqlite.org/c3ref/get_autocommit.html)
     *
     * Test case: yes, included in the transaction tests
     *
     */
    bool isAutoCommit() const;

    /** \brief Starts a new (and possibly nested) transaction on the database.
     *
     * The transaction is automatically finished when the transaction object's dtor is called.
     *
     * \throws BusyException if the DB was busy and the required lock could not be acquired
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns A new transaction object.
     *
     * Test case: yes
     *
     */
    Transaction startTransaction(
        TransactionType tt=TransactionType::Immediate,   ///< the type of transaction, see [here](https://www.sqlite.org/lang_transaction.html)
        TransactionDtorAction _dtorAct = TransactionDtorAction::Rollback   ///< what to do when the dtor is called
        ) const;

    /** \brief Copies structure and, optionally, content of an exising table
     * into a newly created table.
     *
     * The source table must exist and a table with the name of the destination table
     * may not be existing.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns `true` if the operation was successful and `false` if it failed or if the
     * parameters where invalid
     *
     * Test case: yes, but only happy path
     *
     */
    bool copyTable(
        const std::string& srcTabName,   ///< name of the source table for the copy-operation
        const std::string& dstTabName,   ///< name of the destination table (may not yet exist)
        bool copyStructureOnly=false   ///< `true`: copy on the table structure/schema; `false`: deep copy the table's contents
        ) const;

    /** \brief Copies the content of whole database to a file on disk
     *
     * \note Only the `main` database is copied, not any attached database.
     *
     * \warning All content of the destination file will be overwritten!
     *
     * \throws std::invalid_argument if the provided destination file name is empty
     *
     * \throws BusyException if the source (the caller) or destination database (the filename) is busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns almost always `true` because most errors should be caught by exceptions; if `true`, the backup
     * is guaranteed to be successful
     *
     * Test case: yes, but only happy path
     *
     */
    bool backupToFile(
        const std::string& dstFileName   ///< the name of the database file to copy the contents to
        ) const;

    /** \brief Copies the content of a database file into the current database
     *
     * \note Only the `main` database is restored, not any attached database.
     *
     * \warning All content of the database will be overwritten!
     *
     * \throws std::invalid_argument if the provided source file name is empty
     *
     * \throws BusyException if the source (the caller) or destination database (the filename) is busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns almost always `true` because most errors should be caught by exceptions; if `true`, the backup
     * is guaranteed to be successful
     *
     * Test case: yes, but only happy path
     *
     */
    bool restoreFromFile(
        const std::string& srcFileName    ///< the name of the database file to read from
        );

    /** \returns `true` if the database contents have been modified by this or any other database connection
     *
     * Test case: yes
     *
     */
    bool isDirty() const;

    /** \brief Resets the internal "dirty flag", so that only future modifications will be reported by
     * `isDirty()`. This call resets the flags for all local and global/external changes.
     *
     * Test case: yes
     *
     */
    void resetDirtyFlag();

    /** \returns the total number of rows inserted, modified or deleted by all INSERT,
     * UPDATE or DELETE statements completed since the database connection was opened
     *
     * \note Modifications caused by other database connections are *not* reported
     * by this call!
     *
     * Test case: yes
     *
     */
    int getLocalChangeCounter_total() const;

    /** \brief Resets the internal flag for local data changes (local = on this connection);
     * this does not affect the flag for external data changes through other connections.
     *
     * Test case: yes
     *
     */
    void resetLocalChangeCounter();

    /** \returns `true` if the database contents have been modified by this database connection since
     * the last call to `resetLocalChangeCounter()` (or since opening the connection, resp.); modifications
     * applied through other connections are not reported here.
     *
     * Test case: yes
     *
     */
    bool hasLocalChanges() const;

    /** \brief Resets the internal flag for external data changes (external = other than on this connection);
     * this does not affect the flag for local data changes through this connections.
     *
     * Test case: yes
     *
     */
    void resetExternalChangeCounter();

    /** \returns `true` if the database contents have been modified by other connections since
     * the last call to `resetExternalChangeCounter()` (or since opening the connection, resp.); modifications
     * applied through this connection are not reported here.
     *
     * Test case: yes
     *
     */
    bool hasExternalChanges() const;

    /** \brief Sets a callback function that is called by SQLite whenever
     * the database is modified.
     *
     * See also [here](https://www.sqlite.org/c3ref/update_hook.html)
     *
     * \returns the custom data pointer that was previously set or nullptr if there was none
     */
    void* setDataChangeNotificationCallback(
        void(*)(void*, int, const char*, const char*, sqlite3_int64),   ///< the pointer to the callback function
        void* customPtr   ///< a pointer that will be passed to the callback function
        );

    /** \returns the current number of entries in the changelog
     */
    size_t getChangeLogLength();

    /** \returns all entries in the changelog and clears the log afterwards
     */
    ChangeLogList getAllChangesAndClearQueue();

    /** \brief Enables a built-in callback function that implements a simple
     * changelog.
     *
     * \note This overrides any previously set callback function, e.g. when
     * using `setDataChangeNotificationCallback` with a user-provided function.
     */
    void enableChangeLog(
        bool clearLog   ///< indicates wheter the internal change log shall be cleared before activating the logging
        );

    /** \brief Disables any change log callback functions.
     */
    void disableChangeLog(
        bool clearLog   ///< indicates wheter the internal change log shall be cleared after disabling the logging
        );

    /** \brief Defines the timeout for requests if the database is locked by another
     * process (more precisely: another database connection); see [here](https://www.sqlite.org/c3ref/busy_timeout.html)
     * for details.
     *
     * After the timeout has elapsed and the database has not become available,
     * SQLite return SQLITE_BUSY which is translated
     * into a BusyException. While waiting for the timeout, the triggering call blocks.
     *
     * Test case: not yet
     *
     */
    void setBusyTimeout(
        int ms   ///< the timeout in milliseconds; if less or equal to 0, we don't wait an throw BusyException immediately
        );

    /** \brief Creates a new SqliteDatabase object that works on the same database
     * file as the current connection.
     *
     * The result is similar to calling the SqliteDatabase ctor with the database's
     * filename. However, we do not call `populateTables()` or `populateViews()` on the new connection
     * because we assume that the schema has been fully set up before.
     *
     * \note Only the main database ("main") will be cloned, not any other attached
     * database.
     *
     * \throws std::invalid_argument if this method was called on a temporary or an
     * in-memory database
     *
     * \throws GenericSqliteException incl. error code if anything goes wrong
     * with SQLite
     *
     * \returns a new SqliteDatabase instance that works on the same database file
     * as the current instance
     *
     * Test case: not yet
     *
     */
    template<class DB_CLASS = SqliteDatabase>
    DB_CLASS duplicateConnection(
        bool readOnly   ///< `true`: open the new connection in read-only mode; `false`: open in r/w mode
        )
    {
      static_assert (std::is_base_of_v<SqliteDatabase, DB_CLASS>);

      std::string fn = filename();
      if (fn.empty())
      {
        throw std::invalid_argument("duplicateConnection(): called on a temporary or in-memory database");
      }

      OpenMode om = readOnly ? OpenMode::OpenExisting_RO : OpenMode::OpenExisting_RW;

      return DB_CLASS{fn, om};
    }

    /** \brief Function for creating a new, empty key-value-table in the database
     *
     * \throws std::invalid_argument if the provided table name is empty or already in use
     *
     * \returns a KeyValueTab instance for the newly created table
     */
    KeyValueTab createNewKeyValueTab(
        const std::string& tabName   ///< the table name
        );

    /** \returns a string containing the path to the database file (empty for in-memory databases)
     *
     * \note Only the path for the main database ("main") will be returned! Other attached
     * databases are ignored!
     *
     * Test case: implicitly as part of the `operator==()` and of `duplicateConnection()`
     */
    std::string filename() const;

    /** \brief Overloaded operator for "is equal", compares the path
     * to the database file and the raw `sqlite3*` handle.
     *
     * If the values of the `sqlite3*` handles are identical, we
     * return `true`. In all other cases, we compare the file paths
     * of the underlying database files.
     *
     * \note The check applies only to the main database ("main"). Attached databases
     * are ignored for this check.
     *
     * Test case: yes
     */
    bool operator==(const SqliteDatabase& other) const;

    /** \brief Overloaded operator for "is not equal", compares the path
     * to the database file and the raw `sqlite3*` handle.
     *
     * \note The check applies only to the main database ("main"). Attached databases
     * are ignored for this check.
     *
     * Test case: yes
     */
    bool operator!=(const SqliteDatabase& other) const;

  protected:
    /** \brief Copies the contents of the source database into the destination database,
     * original documentation of the backup process [here](https://www.sqlite.org/c3ref/backup_finish.html)
     *
     * \note Only the `main` database is copied, not any attached database.
     *
     * \warning All content of the destination database will be overwritten!
     *
     * \throws std::invalid_argument if source or destination database are `nullptr`
     *
     * \throws std::invalid_argument if source and destination are identical
     *
     * \throws BusyException if the destination database is busy and cannot be written
     *
     * \throws GenericSqliteException incl. error code (especially SQLITE_READONLY!) if anything else goes wrong
     *
     * \returns almost always `true` because most errors should be caught by exceptions; if `true`, the backup
     * is guaranteed to be successful
     *
     * Test case: yes, but only happy path
     *
     */
    static bool copyDatabaseContents(
        sqlite3* srcHandle,   ///< the raw SQLite handle of the source database
        sqlite3* dstHandle   ///< the raw SQLite handle of the destination database
        );

    /**
     * \brief the internal database handle
     */
    sqlite3* dbPtr{nullptr};

    // handling of a "dirty flag" that indicates whether the database has changed
    // after a certain point in time
    int localChangeCounter_resetValue;  // used for calls to sqlite3_total_changes(), reporting changes on the local connection
    int externalChangeCounter_resetValue;   // used for calls to "PRAGMA data_version" which reports changes other than on the local connection

    // a queue of changes
    bool isChangeLogEnabled{false};
    ChangeLogList changeLog;
    std::mutex changeLogMutex;
    ChangeLogCallbackContext logCallbackContext;
  };

}

#endif  /* SQLITEDATABASE_H */
