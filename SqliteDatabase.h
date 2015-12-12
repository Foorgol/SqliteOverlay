#ifndef SQLITE_OVERLAY_SQLITEDATABASE_H
#define	SQLITE_OVERLAY_SQLITEDATABASE_H

#include <sqlite3.h>

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Logger.h"
#include "SqlStatement.h"
#include "HelperFunc.h"

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

  class DbTab;

  template<class T>
  class ScalarQueryResult;

  struct SqliteDeleter
  {
    void operator()(sqlite3* p);
  };

  typedef unique_ptr<sqlite3, SqliteDeleter> upSqlite3Db;

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

    void tableCreationHelper(const string& tabName, const vector<string>& colDefs, int* errCodeOut=nullptr);
    void viewCreationHelper(const string& viewName, const string& selectStmt, int* errCodeOut=nullptr);
    void indexCreationHelper(const string& tabName, const string& idxName, const StringList& colNames, bool isUnique=false, int* errCodeOut=nullptr);
    void indexCreationHelper(const string& tabName, const string& idxName, const string& colName, bool isUnique=false, int* errCodeOut=nullptr);
    void indexCreationHelper(const string& tabName, const string& colName, bool isUnique=false, int* errCodeOut=nullptr);

    StringList allTableNames(bool getViews=false);
    StringList allViewNames();

    bool hasTable(const string& name, bool isView=false);
    bool hasView(const string& name);
    string genForeignKeyClause(const string& keyName, const string& referedTable,
                               CONSISTENCY_ACTION onDelete=CONSISTENCY_ACTION::NO_ACTION,
                               CONSISTENCY_ACTION onUpdate=CONSISTENCY_ACTION::NO_ACTION);

    int getLastInsertId();
    int getRowsAffected();

    void setLogLevel(int newLvl);

    DbTab* getTab (const string& tabName);

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

    /**
     * @brief dbPtr the internal database handle
     */
    upSqlite3Db dbPtr;

    /**
     * A logger instance for debug messages
     */
    unique_ptr<Logger> log;

    unordered_map<string, DbTab*> tabCache;

  };

  typedef unique_ptr<SqliteDatabase> upSqliteDatabase;
}

#endif  /* SQLITEDATABASE_H */
