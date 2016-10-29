#ifndef SQLITE_OVERLAY_SQLITEDATABASE_H
#define	SQLITE_OVERLAY_SQLITEDATABASE_H

#include <sqlite3.h>

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include <Sloppy/libSloppy.h>
#include <Sloppy/Logger/Logger.h>

#include "SqlStatement.h"

using namespace std;

namespace SqliteOverlay
{
  enum class CONSISTENCY_ACTION
  {
    SET_NULL,
    SET_DEFAULT,
    CASCADE,
    RESTRICT,
    NO_ACTION,
    __NOT_SET
  };

  enum class CONFLICT_CLAUSE
  {
    ROLLBACK,
    ABORT,
    FAIL,
    IGNORE,
    REPLACE,
    __NOT_SET
  };


  //----------------------------------------------------------------------------

  // some free functions, mostly for creating SQL strings

  string conflictClause2String(CONFLICT_CLAUSE cc);
  string buildColumnConstraint(bool isUnique, CONFLICT_CLAUSE uniqueConflictClause,
                               bool notNull, CONFLICT_CLAUSE notNullConflictClause,
                               bool hasDefault, const string& defVal="");
  string buildForeignKeyClause(const string& referedTable, CONSISTENCY_ACTION onDelete,
                               CONSISTENCY_ACTION onUpdate, string referedColumn="id");

  //----------------------------------------------------------------------------

  enum class TRANSACTION_TYPE
  {
    DEFERRED,
    IMMEDIATE,
    EXCLUSIVE
  };

  enum class TRANSACTION_DESTRUCTOR_ACTION
  {
    COMMIT,
    ROLLBACK,
  };

  //----------------------------------------------------------------------------

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

  class SqliteDatabase
  {
  public:
    template<class T>
    static unique_ptr<T> get(string dbFileName = ":memory:", bool createNew=false) {
      T* tmpPtr;

      try
      {
        tmpPtr = new T(dbFileName, createNew);
      } catch (exception e) {
        return nullptr;
      }
      tmpPtr->populateTables();
      tmpPtr->populateViews();
      return unique_ptr<T>(tmpPtr);
    }
    virtual ~SqliteDatabase();

    bool close(bool forceDeleteTabObjects=false);
    void resetTabCache();
    bool isAlive() const;

    // disable copying of the database object
    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    // statements and queries
    upSqlStatement prepStatement(const string& sqlText, int* errCodeOut=nullptr);
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
    unique_ptr<Transaction> startTransaction(TRANSACTION_TYPE tt = TRANSACTION_TYPE::IMMEDIATE,
                                             TRANSACTION_DESTRUCTOR_ACTION dtorAct = TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK,
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
