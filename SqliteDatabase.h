#ifndef SQLITEDATABASE_H
#define	SQLITEDATABASE_H

#include <sqlite3.h>

#include <string>
#include <memory>
#include <vector>

#include "Logger.h"
#include "SqlStatement.h"

using namespace std;

namespace SqliteOverlay
{
  struct SqliteDeleter
  {
    void operator()(sqlite3* p);
  };

  typedef unique_ptr<sqlite3, SqliteDeleter> upSqlite3Db;

  class SqliteDatabase
  {
  public:
    //static unique_ptr<SqliteDatabase> get(string dbFileName = ":memory:", bool createNew=false);

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

    void enforceSynchronousWrites(bool syncOn);

    virtual void populateTables() {};
    virtual void populateViews() {};

    void tableCreationHelper(const string& tabName, const vector<string>& colDefs);
    void viewCreationHelper(const string& viewName, const string& selectStmt);

  protected:
    SqliteDatabase(string dbFileName = ":memory:", bool createNew=false);
    vector<string> foreignKeyCreationCache;

    /**
     * @brief dbPtr the internal database handle
     */
    upSqlite3Db dbPtr;

    /**
     * A counter for executed queries; for debugging purposes only
     */
    long queryCounter;

    /**
     * A logger instance for debug messages
     */
    unique_ptr<Logger> log;
  };

  typedef unique_ptr<SqliteDatabase> upSqliteDatabase;
}

#endif  /* SQLITEDATABASE_H */
