#include <sys/stat.h>

#include "SqliteDatabase.h"
#include "Logger.h"
#include "HelperFunc.h"
#include "DbTab.h"

using namespace std;

namespace SqliteOverlay
{
  SqliteDatabase::SqliteDatabase(string dbFileName, bool createNew)
  {
    // check if the filename is valid
    if (dbFileName.empty())
    {
      throw invalid_argument("Invalid database file name");
    }

    // check if we're only creating a memory database
    bool isInMem = (dbFileName == ":memory:");

    // check if the file exists
    struct stat buffer;
    bool hasFile = (stat(dbFileName.c_str(), &buffer) == 0);

    if (!isInMem && !hasFile && !createNew)
    {
      throw invalid_argument("Database file not existing and new file shall not be created");
    }

    // try to open the database
    sqlite3* tmpPtr;
    int err = sqlite3_open(dbFileName.c_str(), &tmpPtr);
    if (tmpPtr == nullptr)
    {
      throw runtime_error("No memory for allocating sqlite instance");
    }
    if (err != SQLITE_OK)
    {
      // delete the sqlite instance we've just created
      SqliteDeleter sd;
      sd(tmpPtr);

      throw invalid_argument(sqlite3_errmsg(tmpPtr));
    }

    // we're all set
    dbPtr = unique_ptr<sqlite3, SqliteDeleter>(tmpPtr);
    foreignKeyCreationCache.clear();
    tabCache.clear();

    // prepare a logger
    log = unique_ptr<Logger>(new Logger(dbFileName));
    log->info("Ready for use");

    // Explicitly enable support for foreign keys
    // and disable synchronous writes for better performance
    execNonQuery("PRAGMA foreign_keys = ON", &err);
    if (err != SQLITE_DONE)
    {
      // delete the sqlite instance we've just created
      SqliteDeleter sd;
      sd(tmpPtr);

      throw invalid_argument(sqlite3_errmsg(tmpPtr));
    }

    enforceSynchronousWrites(false);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQuery_prep(const upSqlStatement& stmt, int* errCodeOut) const
  {
    bool isOk = stmt->step(errCodeOut, log.get());
    if (!isOk) return false;
    if (!(stmt->hasData())) return false;

    return true;
  }

  //----------------------------------------------------------------------------

  upSqlStatement SqliteDatabase::execScalarQuery_prep(const string& sqlStatement, int* errCodeOut)
  {
    upSqlStatement stmt = execContentQuery(sqlStatement, errCodeOut);
    if (stmt == nullptr) return nullptr;
    if (!(stmt->hasData())) return nullptr;

    return stmt;
  }

  //----------------------------------------------------------------------------

//  unique_ptr<SqliteDatabase> SqliteDatabase::get(string dbFileName, bool createNew)
//  {
//    SqliteDatabase* tmpPtr;

//    try
//    {
//      tmpPtr = new SqliteDatabase(dbFileName, createNew);
//    } catch (exception e) {
//      return nullptr;
//    }

//    return unique_ptr<SqliteDatabase>(tmpPtr);
//  }

  //----------------------------------------------------------------------------

  SqliteDatabase::~SqliteDatabase()
  {
    resetTabCache();
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::close(bool forceDeleteTabObjects)
  {
    if (dbPtr == nullptr) return true;

    int result = sqlite3_close(dbPtr.get());
    if ((result != SQLITE_OK) && !forceDeleteTabObjects) return false;

    resetTabCache();

    dbPtr = nullptr;

    return (result == SQLITE_OK);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::resetTabCache()
  {
    auto it = tabCache.begin();
    while (it != tabCache.end())
    {
      delete it->second;
      ++it;
    }
    tabCache.clear();
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::isAlive() const
  {
    return (dbPtr != nullptr);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execNonQuery(const string& sqlStatement, int* errCodeOut)
  {
    upSqlStatement stmt = prepStatement(sqlStatement, errCodeOut);
    if (stmt == nullptr)
    {
      // invalid statement
      return false;
    }

    return execNonQuery(stmt, errCodeOut);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execNonQuery(const upSqlStatement& stmt, int* errCodeOut) const
  {
    // execute the statement
    if (stmt->step(errCodeOut, log.get()))
    {
      // if successful, execute further steps of the
      // statement until the execution is complete
      while (!(stmt->isDone()))
      {
        if (!(stmt->step(errCodeOut, log.get()))) return false;
      }
    } else {
      // initial execution raised a failure
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------

  upSqlStatement SqliteDatabase::execContentQuery(const string& sqlStatement, int* errCodeOut)
  {
    upSqlStatement stmt = prepStatement(sqlStatement, errCodeOut);
    if (stmt == nullptr)
    {
      // invalid statement
      return nullptr;
    }

    if (!(stmt->step(errCodeOut, log.get())))
    {
      return nullptr;
    }

    return stmt;
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execContentQuery(const upSqlStatement& stmt, int* errCodeOut)
  {
    return stmt->step(errCodeOut, log.get());
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryInt(const string& sqlStatement, int* out, int* errCodeOut)
  {
    auto stmt = execScalarQuery_prep<int>(sqlStatement, out, errCodeOut);
    return (stmt == nullptr) ? false : stmt->getInt(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryInt(const upSqlStatement& stmt, int* out, int* errCodeOut) const
  {
    return execScalarQuery_prep<int>(stmt, out, errCodeOut) ? stmt->getInt(0, out) : false;
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<int>> SqliteDatabase::execScalarQueryInt(const upSqlStatement& stmt, int* errCodeOut, bool skipPrep) const
  {
    if (!skipPrep)
    {
      if (!(execScalarQuery_prep(stmt, errCodeOut)))
      {
        return nullptr;
      }
    }

    ScalarQueryResult<int>* result;

    if (stmt->isNull(0))
    {
      result = new ScalarQueryResult<int>();
    } else {
      int i;
      if (!(stmt->getInt(0, &i)))
      {
        return nullptr;
      }
      result = new ScalarQueryResult<int>(i);
    }

    return unique_ptr<ScalarQueryResult<int>>(result);
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<int> > SqliteDatabase::execScalarQueryInt(const string& sqlStatement, int* errCodeOut)
  {
    auto stmt = execScalarQuery_prep(sqlStatement, errCodeOut);
    if (stmt == nullptr)
    {
      return nullptr;
    }

    return execScalarQueryInt(stmt, errCodeOut, true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryDouble(const string& sqlStatement, double* out, int* errCodeOut)
  {
    auto stmt = execScalarQuery_prep<double>(sqlStatement, out, errCodeOut);
    return (stmt == nullptr) ? false : stmt->getDouble(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryDouble(const upSqlStatement& stmt, double* out, int* errCodeOut) const
  {
    return execScalarQuery_prep<double>(stmt, out, errCodeOut) ? stmt->getDouble(0, out) : false;
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<double> > SqliteDatabase::execScalarQueryDouble(const upSqlStatement& stmt, int* errCodeOut, bool skipPrep) const
  {
    if (!skipPrep)
    {
      if (!(execScalarQuery_prep(stmt, errCodeOut)))
      {
        return nullptr;
      }
    }

    ScalarQueryResult<double>* result;

    if (stmt->isNull(0))
    {
      result = new ScalarQueryResult<double>();
    } else {
      double d;
      if (!(stmt->getDouble(0, &d)))
      {
        return nullptr;
      }
      result = new ScalarQueryResult<double>(d);
    }

    return unique_ptr<ScalarQueryResult<double>>(result);
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<double> > SqliteDatabase::execScalarQueryDouble(const string& sqlStatement, int* errCodeOut)
  {
    auto stmt = execScalarQuery_prep(sqlStatement, errCodeOut);
    if (stmt == nullptr)
    {
      return nullptr;
    }

    return execScalarQueryDouble(stmt, errCodeOut, true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryString(const string& sqlStatement, string* out, int* errCodeOut)
  {
    auto stmt = execScalarQuery_prep<string>(sqlStatement, out, errCodeOut);
    return (stmt == nullptr) ? false : stmt->getString(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryString(const upSqlStatement& stmt, string* out, int* errCodeOut) const
  {
    return execScalarQuery_prep<string>(stmt, out, errCodeOut) ? stmt->getString(0, out) : false;
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<string> > SqliteDatabase::execScalarQueryString(const upSqlStatement& stmt, int* errCodeOut, bool skipPrep) const
  {
    if (!skipPrep)
    {
      if (!(execScalarQuery_prep(stmt, errCodeOut)))
      {
        return nullptr;
      }
    }

    ScalarQueryResult<string>* result;

    if (stmt->isNull(0))
    {
      result = new ScalarQueryResult<string>();
    } else {
      string s;
      if (!(stmt->getString(0, &s)))
      {
        return nullptr;
      }
      result = new ScalarQueryResult<string>(s);
    }

    return unique_ptr<ScalarQueryResult<string>>(result);
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<string> > SqliteDatabase::execScalarQueryString(const string& sqlStatement, int* errCodeOut)
  {
    auto stmt = execScalarQuery_prep(sqlStatement, errCodeOut);
    if (stmt == nullptr)
    {
      return nullptr;
    }

    return execScalarQueryString(stmt, errCodeOut, true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::enforceSynchronousWrites(bool syncOn)
  {
    string sql = "PRAGMA synchronous = O";
    if (syncOn)
    {
      sql += "N";
    }
    else
    {
      sql += "FF";
    }

    int err;
    execNonQuery(sql, &err);
    return (err == SQLITE_OK);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::tableCreationHelper(const string& tabName, const vector<string>& colDefs, int* errCodeOut)
  {
    string sql = "CREATE TABLE IF NOT EXISTS " + tabName + " (";
    sql += "id INTEGER NOT NULL PRIMARY KEY ";

    sql += "AUTOINCREMENT";

    sql += ", " + commaSepStringFromStringList(colDefs);

    if (foreignKeyCreationCache.size() != 0) {
      sql += ", " + commaSepStringFromStringList(foreignKeyCreationCache);
      foreignKeyCreationCache.clear();
    }

    sql += ");";
    execNonQuery(sql, errCodeOut);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::viewCreationHelper(const string& viewName, const string& selectStmt, int* errCodeOut)
  {
    string sql = "CREATE VIEW IF NOT EXISTS";

    sql += " " + viewName + " AS ";
    sql += selectStmt;
    execNonQuery(sql, errCodeOut);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const StringList &colNames, bool isUnique, int* errCodeOut)
  {
    if (idxName.empty()) return;
    if (!(hasTable(tabName))) return;

    string sql = "CREATE ";
    if (isUnique) sql += "UNIQUE ";
    sql += "INDEX IF NOT EXISTS ";
    sql += idxName + " ON " + tabName + " (";
    sql += commaSepStringFromStringList(colNames) + ")";
    execNonQuery(sql, errCodeOut);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const string &colName, bool isUnique, int* errCodeOut)
  {
    StringList colList;
    colList.push_back(colName);
    indexCreationHelper(tabName, idxName, colList, isUnique, errCodeOut);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &colName, bool isUnique, int* errCodeOut)
  {
    if (tabName.empty()) return;
    if (colName.empty()) return;

    // auto-create a canonical index name
    string idxName = tabName + "_" + colName;

    indexCreationHelper(tabName, idxName, colName, isUnique, errCodeOut);
  }

  //----------------------------------------------------------------------------

  StringList SqliteDatabase::allTableNames(bool getViews)
  {
    StringList result;

    string sql = "SELECT name FROM sqlite_master WHERE type='";
    sql += getViews ? "view" : "table";
    sql +="';";
    auto stmt = execContentQuery(sql, nullptr);
    while (stmt->hasData())
    {
      string tabName;
      stmt->getString(0, &tabName);

      // skip a special, internal table
      if (tabName != "sqlite_sequence")
      {
        result.push_back(tabName);
      }

      stmt->step(nullptr, log.get());
    }

    return result;
  }

  //----------------------------------------------------------------------------

  StringList SqliteDatabase::allViewNames()
  {
    return allTableNames(true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::hasTable(const string& name, bool isView)
  {
    StringList allTabs = allTableNames(isView);
    for(string n : allTabs)
    {
      if (n == name) return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::hasView(const string& name)
  {
    return hasTable(name, true);
  }

  //----------------------------------------------------------------------------

  string SqliteDatabase::genForeignKeyClause(const string& keyName, const string& referedTable, CONSISTENCY_ACTION onDelete, CONSISTENCY_ACTION onUpdate)
  {
    // a little helper to translate a CONSISTENCY_ACTION value into a string
    auto ca2string = [](CONSISTENCY_ACTION ca) -> string {
      switch (ca)
      {
        case CONSISTENCY_ACTION::NO_ACTION:
          return "NO ACTION";
        case CONSISTENCY_ACTION::SET_NULL:
          return "SET NULL";
        case CONSISTENCY_ACTION::SET_DEFAULT:
          return "SET DEFAULT";
        case CONSISTENCY_ACTION::CASCADE:
          return "CASCADE";
        case CONSISTENCY_ACTION::RESTRICT:
          return "RESTRICT";
      }
      return "";
    };

    string ref = "FOREIGN KEY (" + keyName + ") REFERENCES " + referedTable + "(id)";
    ref += " ON DELETE " + ca2string(onDelete);
    ref += " ON UPDATE " + ca2string(onUpdate);

    foreignKeyCreationCache.push_back(ref);
    return keyName + " INTEGER";
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getLastInsertId()
  {
    return sqlite3_last_insert_rowid(dbPtr.get());
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getRowsAffected()
  {
    return sqlite3_changes(dbPtr.get());
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::isAutoCommit() const
  {
    return sqlite3_get_autocommit(dbPtr.get());
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::setLogLevel(int newLvl)
  {
    log->setLevel(newLvl);
  }

  //----------------------------------------------------------------------------

  DbTab* SqliteDatabase::getTab(const string& tabName)
  {
    // try to find the tabl object in the cache
    auto it = tabCache.find(tabName);

    // if we have a hit, return the table object
    if (it != tabCache.end())
    {
      return it->second;
    }

    // okay, we have a cache miss.
    //
    // check if the table exists
    if (!(hasTable(tabName)))
    {
      return nullptr;
    }

    // the table name is valid, but the table
    // has not yet been accessed. So we need to
    // create a new tab object
    DbTab* tab = new DbTab(this, tabName);
    tabCache[tabName] = tab;

    return tab;
  }

  //----------------------------------------------------------------------------

  upSqlStatement SqliteDatabase::prepStatement(const string& sqlText, int* errCodeOut)
  {
    return SqlStatement::get(dbPtr.get(), sqlText, errCodeOut, log.get());
  }

  //----------------------------------------------------------------------------

  void SqliteDeleter::operator()(sqlite3* p)
  {
    if (p != nullptr)
    {
      sqlite3_close_v2(p);
    }
  }

}
