#ifndef SQLITE_OVERLAY_SQLITEDATABASE_H
#define	SQLITE_OVERLAY_SQLITEDATABASE_H

#include <sqlite3.h>

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <type_traits>

#include <Sloppy/Utils.h>
#include <Sloppy/Logger/Logger.h>

#include "SqlStatement.h"
#include "SqliteExceptions.h"
#include "Defs.h"

using namespace std;

namespace SqliteOverlay
{
  // some free functions, mostly for creating SQL strings

  /** \brief Converts a ConflictClause enum into a string that can be used
   * for constructing a SQL statement.
   *
   * Example: `ConflictClause::Abort' --> "ABORT"
   *
   * \returns a string representing the provided ConflictClause enum value or "" for `ConflictClause::NotUsed`
   */
  string conflictClause2String(
      ConflictClause cc   ///< the enum to convert to a string
      );

  /** \brief Creates a column constraint string for CREATE TABLE statements
   * according to [this description](https://www.sqlite.org/syntax/column-constraint.html).
   *
   * \returns a string with a column constraint clause for use in a CREATE TABLE statement.
   */
  string buildColumnConstraint(
      ConflictClause uniqueConflictClause,   ///< unless set to `NotUsed` include a UNIQUE constraint and define what shall happen in violation
      ConflictClause notNullConflictClause,   ///< unless set to `NotUsed` include a NOT NULL constraint and define what shall happen in violation
      optional<string> defaultVal = optional<string>{}   ///< an optional default value for the column
      );

  /** \brief Creates a foreign key clause for CREATE TABLE statements
   * according to [this description](https://www.sqlite.org/syntax/foreign-key-clause.html).
   *
   * \returns a string with a foreign key clause for use in a CREATE TABLE statement.
   */
  string buildForeignKeyClause(
      const string& referedTable,   ///< the name of the table that this column refers to
      ConsistencyAction onDelete,   ///< the action to be taken if the refered row is being deleted
      ConsistencyAction onUpdate,   ///< the action to be taken if the refered row is updated
      string referedColumn="id"   ///< the name of the column we're pointing to in the refered table
      );

  //
  // forward definitions
  //

  class DbTab;
  class Transaction;

  template<class T>
  class ScalarQueryResult;

  //----------------------------------------------------------------------------

  struct SqliteDeleter
  {
    void operator()(sqlite3* p);
  };

  typedef unique_ptr<sqlite3, SqliteDeleter> upSqlite3Db;

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
    /** \brief A convenience wrapper that opens a database file and instantiates
     * a class derived from `SqliteDatabase` for it.
     *
     * The template parameter `T` is the type of the derived class.
     *
     * \note This function also calls the `populateTables()` and
     * `populateViews()` functions of the (derived) database.
     *
     * \returns an heap allocated database instance of type `T` (with tables and views populated) or `nullptr` on error
     */
    template<class T>
    static unique_ptr<T> get(
        string dbFileName = ":memory:",   ///< the name of the database file
        bool createNew=false   ///< set to `true` if a new, empty database shall be created if the file doesn't exist
        )
    {
      static_assert (is_base_of<SqliteDatabase, T>(), "Template parameter must be a class derived from SqliteDatabase");
      unique_ptr<T> tmpPtr;

      try
      {
        // we can't use make_unique here because the
        // ctor is not public
        T* p = new T(dbFileName, createNew);
        tmpPtr.reset(p);
      } catch (...) {
        return nullptr;
      }

      tmpPtr->populateTables();
      tmpPtr->populateViews();

      return tmpPtr;
    }

    /** \brief Dtor; cleans up table cache and that's it.
     */
    virtual ~SqliteDatabase();

    /** \brief Closes the database connection. The instance shouldn't be
     * used anymore after calling this function.
     *
     * \warning On success, this function also deletes all cached `DbTab` instances!
     * In case you stored any pointers to 'DbTab' instances anywhere in your code
     * these pointers become invalid after calling `close()` and the call was successful!
     *
     * \returns `true` if the database connection could be closed successfully or
     * if the database has already been closed before.
     */
    bool close(PrimaryResultCode* errCode = nullptr);

    /** \brief Deletes all cached `DbTab` instances. */
    void resetTabCache();

    /** \returns `true` if the database connection is still open and `false` otherwise */
    bool isAlive() const;

    /** \brief Disabled copy ctor */
    SqliteDatabase(const SqliteDatabase&) = delete;

    /** \brief Disabled copy assignment */
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    /** \brief Creates a new SQL statement for this database connection
     *
     * \throws std::invalid_argument if the provided SQL string is empty or if the connection has been closed before calling this method
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \returns a SqlStatement instance for the provided SQL text
     */
    SqlStatement prepStatement(
        const string& sqlText   ///< the SQL text for which to create the statement
        );

    bool execNonQuery(const string& sqlStatement, int* errCodeOut=nullptr);
    bool execNonQuery(const upSqlStatement& stmt, int* errCodeOut=nullptr) const;
    upSqlStatement execContentQuery(const string& sqlStatement, int* errCodeOut=nullptr);
    bool execContentQuery(const upSqlStatement& stmt, int* errCodeOut=nullptr);

    bool execScalarQueryInt(const string& sqlStatement, int* out, int* errCodeOut);
    bool execScalarQueryInt(const upSqlStatement& stmt, int* out, int* errCodeOut) const;
    unique_ptr<ScalarQueryResult<int>> execScalarQueryInt(const upSqlStatement& stmt, int* errCodeOut=nullptr, bool skipPrep=false) const;
    unique_ptr<ScalarQueryResult<int>> execScalarQueryInt(const string& sqlStatement, int* errCodeOut=nullptr);

    bool execScalarQueryDouble(const string& sqlStatement, double* out, int* errCodeOut);
    bool execScalarQueryDouble(const upSqlStatement& stmt, double* out, int* errCodeOut) const;
    unique_ptr<ScalarQueryResult<double>> execScalarQueryDouble(const upSqlStatement& stmt, int* errCodeOut=nullptr, bool skipPrep=false) const;
    unique_ptr<ScalarQueryResult<double>> execScalarQueryDouble(const string& sqlStatement, int* errCodeOut=nullptr);

    bool execScalarQueryString(const string& sqlStatement, string* out, int* errCodeOut);
    bool execScalarQueryString(const upSqlStatement& stmt, string* out, int* errCodeOut) const;
    unique_ptr<ScalarQueryResult<string>> execScalarQueryString(const upSqlStatement& stmt, int* errCodeOut=nullptr, bool skipPrep=false) const;
    unique_ptr<ScalarQueryResult<string>> execScalarQueryString(const string& sqlStatement, int* errCodeOut=nullptr);

    bool enforceSynchronousWrites(bool syncOn);

    virtual void populateTables() {}
    virtual void populateViews() {}

    void viewCreationHelper(const string& viewName, const string& selectStmt, int* errCodeOut=nullptr);
    void indexCreationHelper(const string& tabName, const string& idxName, const Sloppy::StringList& colNames, bool isUnique=false, int* errCodeOut=nullptr);
    void indexCreationHelper(const string& tabName, const string& idxName, const string& colName, bool isUnique=false, int* errCodeOut=nullptr);
    void indexCreationHelper(const string& tabName, const string& colName, bool isUnique=false, int* errCodeOut=nullptr);

    Sloppy::StringList allTableNames(bool getViews=false);
    Sloppy::StringList allViewNames();

    bool hasTable(const string& name, bool isView=false);
    bool hasView(const string& name);
    string genForeignKeyClause(const string& keyName, const string& referedTable,
                               CONSISTENCY_ACTION onDelete=CONSISTENCY_ACTION::NO_ACTION,
                               CONSISTENCY_ACTION onUpdate=CONSISTENCY_ACTION::NO_ACTION);

    int getLastInsertId();
    int getRowsAffected();
    bool isAutoCommit() const;
    unique_ptr<Transaction> startTransaction(TransactionType tt = TransactionType::IMMEDIATE,
                                             TransactionDtorAction dtorAct = TransactionDtorAction::ROLLBACK,
                                             int* errCodeOut=nullptr);

    void setLogLevel(SeverityLevel newMinLvl);

    DbTab* getTab (const string& tabName);

    // table copies and database backups
    bool copyTable(const string& srcTabName, const string& dstTabName, int* errCodeOut=nullptr, bool copyStructureOnly=false);
    bool backupToFile(const string& dstFileName, int* errCodeOut=nullptr);
    bool restoreFromFile(const string& srcFileName, int* errCodeOut=nullptr);

    // the dirty flag
    bool isDirty();
    void resetDirtyFlag();
    int getDirtyCounter();

  protected:
    SqliteDatabase(string dbFileName = ":memory:", bool createNew=false);
    vector<string> foreignKeyCreationCache;

    template<class T>
    upSqlStatement execScalarQuery_prep(const string& sqlStatement, T* out, int* errCodeOut)
    {
      if (out == nullptr) return nullptr;

      upSqlStatement stmt = execContentQuery(sqlStatement, errCodeOut);
      if (stmt == nullptr) return nullptr;
      if (!(stmt->hasData())) return nullptr;

      return stmt;
    }

    template<class T>
    bool execScalarQuery_prep(const upSqlStatement& stmt, T* out, int* errCodeOut) const
    {
      if (out == nullptr) return false;
      bool isOk = stmt->step(errCodeOut, log.get());
      if (!isOk) return false;
      if (!(stmt->hasData())) return false;

      return true;
    }

    bool execScalarQuery_prep(const upSqlStatement& stmt, int* errCodeOut) const;
    upSqlStatement execScalarQuery_prep(const string& sqlStatement, int* errCodeOut);

    static int copyDatabaseContents(sqlite3* srcHandle, sqlite3* dstHandle);

    /**
     * @brief dbPtr the internal database handle
     */
    upSqlite3Db dbPtr;

    /**
     * A logger instance for debug messages
     */
    unique_ptr<Logger> log;

    unordered_map<string, DbTab*> tabCache;

    // handling of a "dirty flag" that indicates whether the database has changed
    // after a certain point in time
    int changeCounter_reset;

  };

  typedef unique_ptr<SqliteDatabase> upSqliteDatabase;
}

#endif  /* SQLITEDATABASE_H */
