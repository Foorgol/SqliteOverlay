#ifndef SQLITEDATABASE_H
#define	SQLITEDATABASE_H

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

    bool isAlive() const;

    // disable copying of the database object
    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    // statements and queries
    upSqlStatement prepStatement(const string& sqlText);
    bool execNonQuery(const string& sqlStatement);
    bool execNonQuery(const upSqlStatement& stmt) const;
    upSqlStatement execContentQuery(const string& sqlStatement);
    bool execContentQuery(const upSqlStatement& stmt);

    bool execScalarQueryInt(const string& sqlStatement, int* out);
    bool execScalarQueryInt(const upSqlStatement& stmt, int* out) const;
    unique_ptr<ScalarQueryResult<int>> execScalarQueryInt(const upSqlStatement& stmt, bool skipPrep=false) const;
    unique_ptr<ScalarQueryResult<int>> execScalarQueryInt(const string& sqlStatement);

    bool execScalarQueryDouble(const string& sqlStatement, double* out);
    bool execScalarQueryDouble(const upSqlStatement& stmt, double* out) const;
    unique_ptr<ScalarQueryResult<double>> execScalarQueryDouble(const upSqlStatement& stmt, bool skipPrep=false) const;
    unique_ptr<ScalarQueryResult<double>> execScalarQueryDouble(const string& sqlStatement);

    bool execScalarQueryString(const string& sqlStatement, string* out);
    bool execScalarQueryString(const upSqlStatement& stmt, string* out) const;
    unique_ptr<ScalarQueryResult<string>> execScalarQueryString(const upSqlStatement& stmt, bool skipPrep=false) const;
    unique_ptr<ScalarQueryResult<string>> execScalarQueryString(const string& sqlStatement);

    void enforceSynchronousWrites(bool syncOn);

    virtual void populateTables() {};
    virtual void populateViews() {};

    void tableCreationHelper(const string& tabName, const vector<string>& colDefs);
    void viewCreationHelper(const string& viewName, const string& selectStmt);

    StringList allTableNames(bool getViews=false);
    StringList allViewNames();

    bool hasTable(const string& name, bool isView=false);
    bool hasView(const string& name);
    string genForeignKeyClause(const string& keyName, const string& referedTable);

    int getLastInsertId();
    int getRowsAffected();

    void setLogLevel(int newLvl);

    DbTab* getTab (const string& tabName);

  protected:
    SqliteDatabase(string dbFileName = ":memory:", bool createNew=false);
    vector<string> foreignKeyCreationCache;

    template<class T>
    upSqlStatement execScalarQuery_prep(const string& sqlStatement, T* out)
    {
      if (out == nullptr) return nullptr;

      upSqlStatement stmt = execContentQuery(sqlStatement);
      if (stmt == nullptr) return nullptr;
      if (!(stmt->hasData())) return nullptr;

      return stmt;
    }

    template<class T>
    bool execScalarQuery_prep(const upSqlStatement& stmt, T* out) const
    {
      if (out == nullptr) return false;
      bool isOk = stmt->step(log.get());
      if (!isOk) return false;
      if (!(stmt->hasData())) return false;

      return true;
    }

    bool execScalarQuery_prep(const upSqlStatement& stmt) const;
    upSqlStatement execScalarQuery_prep(const string& sqlStatement);

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
