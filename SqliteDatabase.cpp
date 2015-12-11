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
    execNonQuery("PRAGMA foreign_keys = ON");
    enforceSynchronousWrites(false);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQuery_prep(const upSqlStatement& stmt) const
  {
    bool isOk = stmt->step(log.get());
    if (!isOk) return false;
    if (!(stmt->hasData())) return false;

    return true;
  }

  //----------------------------------------------------------------------------

  upSqlStatement SqliteDatabase::execScalarQuery_prep(const string& sqlStatement)
  {
    upSqlStatement stmt = execContentQuery(sqlStatement);
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
    auto it = tabCache.begin();
    while (it != tabCache.end())
    {
      delete it->second;
      ++it;
    }
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::isAlive() const
  {
    return (dbPtr != nullptr);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execNonQuery(const string& sqlStatement)
  {
    upSqlStatement stmt = prepStatement(sqlStatement);
    if (stmt == nullptr)
    {
      // invalid statement
      return false;
    }

    return execNonQuery(stmt);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execNonQuery(const upSqlStatement& stmt) const
  {
    // execute the statement
    if (stmt->step(log.get()))
    {
      // if successful, execute further steps of the
      // statement until the execution is complete
      while (!(stmt->isDone())) stmt->step(log.get());
    } else {
      // initial execution raised a failure
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------

  upSqlStatement SqliteDatabase::execContentQuery(const string& sqlStatement)
  {
    upSqlStatement stmt = prepStatement(sqlStatement);
    if (stmt == nullptr)
    {
      // invalid statement
      return nullptr;
    }

    if (!(stmt->step(log.get())))
    {
      return nullptr;
    }

    return stmt;
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execContentQuery(const upSqlStatement& stmt)
  {
    return stmt->step(log.get());
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryInt(const string& sqlStatement, int* out)
  {
    auto stmt = execScalarQuery_prep<int>(sqlStatement, out);
    return (stmt == nullptr) ? false : stmt->getInt(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryInt(const upSqlStatement& stmt, int* out) const
  {
    return execScalarQuery_prep<int>(stmt, out) ? stmt->getInt(0, out) : false;
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<int>> SqliteDatabase::execScalarQueryInt(const upSqlStatement& stmt, bool skipPrep) const
  {
    if (!skipPrep)
    {
      if (!(execScalarQuery_prep(stmt)))
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

  unique_ptr<ScalarQueryResult<int> > SqliteDatabase::execScalarQueryInt(const string& sqlStatement)
  {
    auto stmt = execScalarQuery_prep(sqlStatement);
    if (stmt == nullptr)
    {
      return nullptr;
    }

    return execScalarQueryInt(stmt, true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryDouble(const string& sqlStatement, double* out)
  {
    auto stmt = execScalarQuery_prep<double>(sqlStatement, out);
    return (stmt == nullptr) ? false : stmt->getDouble(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryDouble(const upSqlStatement& stmt, double* out) const
  {
    return execScalarQuery_prep<double>(stmt, out) ? stmt->getDouble(0, out) : false;
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<double> > SqliteDatabase::execScalarQueryDouble(const upSqlStatement& stmt, bool skipPrep) const
  {
    if (!skipPrep)
    {
      if (!(execScalarQuery_prep(stmt)))
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

  unique_ptr<ScalarQueryResult<double> > SqliteDatabase::execScalarQueryDouble(const string& sqlStatement)
  {
    auto stmt = execScalarQuery_prep(sqlStatement);
    if (stmt == nullptr)
    {
      return nullptr;
    }

    return execScalarQueryDouble(stmt, true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryString(const string& sqlStatement, string* out)
  {
    auto stmt = execScalarQuery_prep<string>(sqlStatement, out);
    return (stmt == nullptr) ? false : stmt->getString(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryString(const upSqlStatement& stmt, string* out) const
  {
    return execScalarQuery_prep<string>(stmt, out) ? stmt->getString(0, out) : false;
  }

  //----------------------------------------------------------------------------

  unique_ptr<ScalarQueryResult<string> > SqliteDatabase::execScalarQueryString(const upSqlStatement& stmt, bool skipPrep) const
  {
    if (!skipPrep)
    {
      if (!(execScalarQuery_prep(stmt)))
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

  unique_ptr<ScalarQueryResult<string> > SqliteDatabase::execScalarQueryString(const string& sqlStatement)
  {
    auto stmt = execScalarQuery_prep(sqlStatement);
    if (stmt == nullptr)
    {
      return nullptr;
    }

    return execScalarQueryString(stmt, true);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::enforceSynchronousWrites(bool syncOn)
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

    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::tableCreationHelper(const string& tabName, const vector<string>& colDefs)
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
    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::viewCreationHelper(const string& viewName, const string& selectStmt)
  {
    string sql = "CREATE VIEW IF NOT EXISTS";

    sql += " " + viewName + " AS ";
    sql += selectStmt;
    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const StringList &colNames, bool isUnique)
  {
    if (idxName.empty()) return;
    if (!(hasTable(tabName))) return;

    string sql = "CREATE ";
    if (isUnique) sql += "UNIQUE ";
    sql += "INDEX IF NOT EXISTS ";
    sql += idxName + " ON " + tabName + " (";
    sql += commaSepStringFromStringList(colNames) + ")";
    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const string &colName, bool isUnique)
  {
    StringList colList;
    colList.push_back(colName);
    indexCreationHelper(tabName, idxName, colList, isUnique);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &colName, bool isUnique)
  {
    if (tabName.empty()) return;
    if (colName.empty()) return;

    // auto-create a canonical index name
    string idxName = tabName + "_" + colName;

    indexCreationHelper(tabName, idxName, colName, isUnique);
  }

  //----------------------------------------------------------------------------

  StringList SqliteDatabase::allTableNames(bool getViews)
  {
    StringList result;

    string sql = "SELECT name FROM sqlite_master WHERE type='";
    sql += getViews ? "view" : "table";
    sql +="';";
    auto stmt = execContentQuery(sql);
    while (stmt->hasData())
    {
      string tabName;
      stmt->getString(0, &tabName);

      // skip a special, internal table
      if (tabName != "sqlite_sequence")
      {
        result.push_back(tabName);
      }

      stmt->step(log.get());
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

  string SqliteDatabase::genForeignKeyClause(const string& keyName, const string& referedTable)
  {
    string ref = "FOREIGN KEY (" + keyName + ") REFERENCES " + referedTable + "(id)";
    foreignKeyCreationCache.push_back(ref);
    return keyName + " INTEGER";
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getLastInsertId()
  {
    int result = -1;
    execScalarQueryInt("SELECT last_insert_rowid()", &result);

    return result;
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getRowsAffected()
  {
    return sqlite3_changes(dbPtr.get());
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

  upSqlStatement SqliteDatabase::prepStatement(const string& sqlText)
  {
    return SqlStatement::get(dbPtr.get(), sqlText, log.get());
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
