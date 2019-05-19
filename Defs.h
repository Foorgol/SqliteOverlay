#ifndef SQLITE_OVERLAY_DEFINITIONS_H
#define SQLITE_OVERLAY_DEFINITIONS_H

#include <stdexcept>

#include <sqlite3.h>

namespace SqliteOverlay
{
  /** \brief SQLite's primary error codes as specified [here](https://www.sqlite.org/rescode.html)
   */
  enum class PrimaryResultCode
  {
    OK = 0,    ///> Successful result
    ERROR = 1,    ///> Generic error
    INTERNAL = 2,    ///> Internal logic error in SQLite
    PERM = 3,    ///> Access permission denied
    ABORT = 4,    ///> Callback routine requested an abort
    BUSY = 5,    ///> The database file is locked
    LOCKED = 6,    ///> A table in the database is locked
    NOMEM = 7,    ///> A `malloc()` failed
    READONLY = 8,    ///> Attempt to write a readonly database
    INTERRUPT = 9,    ///> Operation terminated by `sqlite3_interrupt()`
    IOERR = 10,    ///> Some kind of disk I/O error occurred
    CORRUPT = 11,    ///> The database disk image is malformed
    NOTFOUND = 12,    ///> Unknown opcode in `sqlite3_file_control()`
    FULL = 13,    ///> Insertion failed because database is full
    CANTOPEN = 14,    ///> Unable to open the database file
    PROTOCOL = 15,    ///> Database lock protocol error
    EMPTY = 16,   ///> Internal use only
    SCHEMA = 17,   ///> The database schema changed
    TOOBIG = 18,    ///> String or BLOB exceeds size limit
    CONSTRAINT = 19,    ///> Abort due to constraint violation
    MISMATCH = 20,    ///> Data type mismatch
    MISUSE = 21,    ///> Library used incorrectly
    NOLFS = 22,    ///> Uses OS features not supported on host
    AUTH = 23,    ///> Authorization denied
    FORMAT = 24,    ///> Not used
    RANGE = 25,    ///> 2nd parameter to `sqlite3_bind' out of range
    NOTADB = 26,    ///> File opened that is not a database file
    NOTICE = 27,    ///> Notifications from 'sqlite3_log()`
    WARNING = 28,    ///> Warnings from 'sqlite3_log()`
    ROW = 100,    ///> `sqlite3_step()` has another row ready
    DONE = 101    ///> `sqlite3_step()' has finished executing
  };

  //----------------------------------------------------------------------------

  /** \brief An enum for defining consistency actions for the
   * ON_DELETE and ON_UPDATE restrictions.
   *
   * See [here](https://www.sqlite.org/syntax/foreign-key-clause.html)
   */
  enum class ConsistencyAction
  {
    SetNull,   ///< set the value in the referencing table to NULL
    SetDefault,   ///< set the value in the referencing table to its default value
    Cascade,   ///< update / delete all referencing tables and their references and so on
    Restrict,   ///< FIX
    NoAction   ///< do nothing and break the referential consistency of the database
  };

  //----------------------------------------------------------------------------

  /** \brief An enum used for activating constraints like "unique" or "not null"
   * along with an action that is triggered if the constraint is violated.
   *
   * Actions are further explained [here](https://www.sqlite.org/lang_conflict.html)
   */
  enum class ConflictClause
  {
    Rollback,   ///< abort the current SQL statement and roll back the whole transaction
    Abort,   ///< abort and undo the current SQL statement but do not undo previous SQL statements in the same transaction
    Fail,   ///< stops the execution of the current SQL statement but does not revert changes done by the current SQL statement so far
    Ignore,   ///< ignores the row that violated the constraint and continues with the execution of the current SQL statement
    Replace,   ///< for UNIQUE and PRIMARY_KEY constraints: delete the already existing row that caused the violation
    NotUsed   ///< do not activate the constraint at all
  };

  //----------------------------------------------------------------------------

  /** \brief Fundamental data types as defined [here](https://www.sqlite.org/c3ref/c_blob.html);
   * they are used to determine the initial data type of a result column.
   *
   * \note The `ColumnDataType` is relevant for the type of *result columns* in a SELECT
   * statement while the `ColumnAffinity` is derived from the *declared table column type*
   * that was used in the CREATE TABLE statement.
   */
  enum class ColumnDataType
  {
    Integer = 1,   ///< 64-bit signed integer
    Float = 2,   ///< 64-bit IEEE floating point number
    Text = 3,   ///< plain string
    Blob = 4,   ///< blob of binary data
    Null = 5,   ///< Null
  };

  //----------------------------------------------------------------------------

  /** \brief The fundamental type affinity of a table column as explained [here](https://www.sqlite.org/datatype3.html).
   *
   * \note The `ColumnAffinity` is derived from the *declared table column type* that was used
   * in the CREATE TABLE statement while the `ColumnDataType` is relevant for the type
   * of *result columns* in a SELECT statement.
   */
  enum class ColumnAffinity
  {
    Integer,
    Real,
    Text,
    Blob,
    Numeric
  };

  //----------------------------------------------------------------------------

  /** \returns the ColumnDataType representation for an int
   *
   * \throws std::invalid argument if the provided int was invalid
   */
  ColumnDataType int2ColumnDataType(int i);

  //----------------------------------------------------------------------------

  /** \brief An enum that defines the locking type of a transaction
   *
   * See [here](https://www.sqlite.org/lang_transaction.html)
   */
  enum class TransactionType
  {
    Deferred,   ///< do not acquire any locks before the first actual read or write operation
    Immediate,   ///< immediately acquire a lock for writing to the database; other users can continue to read from the database
    Exclusive   ///< basically, no other connection will be able to read or write the database
  };

  //----------------------------------------------------------------------------

  /** \brief The action to be taken if the destructor of a Transaction object is called and the transaction
   * is still active (not yet committed or not yet rolled back).
   *
   * If the transaction is not active anymore, the dtor does nothing.
   */
  enum class TransactionDtorAction
  {
    Commit,   ///< commit the transaction
    Rollback,   ///< rollback the transcation
  };

  //----------------------------------------------------------------------------

  /** \brief The mode in which to open a new database connection
   */
  enum class OpenMode
  {
    ForceNew,   ///< create a new database and fail if the file already exists
    OpenOrCreate_RW,  ///< open an existing database in read/write mode and create a new one if it doesn't exist
    OpenExisting_RW,  ///< open an existing database in read/write mode and fail if it doesn't exist
    OpenExisting_RO   ///< open an existing database in read-only mode and fail if it doesn't exist
  };

}

namespace std
{
  /** \brief Converts a ColumnDataType enum into a string that can be used
   * for constructing a SQL statement.
   *
   * Example: `ColumnDataType::Integer' --> "INTEGER"
   *
   * \returns a string representing the provided ColumnDataType enum value or "" for ColumnDataType::Null
   */
  std::string to_string(SqliteOverlay::ColumnDataType cdt);

  //----------------------------------------------------------------------------

  /** \brief Converts a ConsistencyAction enum into a string that can be used
   * for constructing a SQL statement.
   *
   * Example: `ConsistencyAction::SetNull' --> "SET NULL"
   *
   * \returns a string representing the provided ConsistencyAction enum value
   */
  std::string to_string(SqliteOverlay::ConsistencyAction ca);

  //----------------------------------------------------------------------------

  /** \brief Converts a ConflictClause enum into a string that can be used
   * for constructing a SQL statement.
   *
   * Example: `ConflictClause::Abort' --> "ABORT"
   *
   * \returns a string representing the provided ConflictClause enum value or "" for `ConflictClause::NotUsed` or `ConflictClause::NoAction`
   */
  std::string to_string(SqliteOverlay::ConflictClause cc);

}

#endif
