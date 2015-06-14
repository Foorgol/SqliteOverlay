#include <sys/stat.h>

#include "SqliteDatabase.h"
#include "Logger.h"
#include "HelperFunc.h"

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
    queryCounter = 0;
    foreignKeyCreationCache.clear();

    // prepare a logger
    log = unique_ptr<Logger>(new Logger(dbFileName));
    log->info("Ready for use");
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
    if (stmt->step())
    {
      // if successful, execute further steps of the
      // statement until the execution is complete
      while (!(stmt->isDone())) stmt->step();
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

    if (!(stmt->step()))
    {
      return nullptr;
    }

    return stmt;
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execContentQuery(const upSqlStatement& stmt)
  {
    return stmt->step();
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryInt(const string& sqlStatement, int* out)
  {
    if (out == nullptr) return false;

    upSqlStatement stmt = execContentQuery(sqlStatement);
    if (stmt == nullptr) return false;
    if (!(stmt->hasData())) return false;

    return stmt->getInt(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryInt(const upSqlStatement& stmt, int* out) const
  {
    bool isOk = stmt->step();
    if (!isOk) return false;
    if (!(stmt->hasData())) return false;

    return stmt->getInt(0, out);
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
