#include <sys/stat.h>

#include <boost/date_time/local_time/local_time.hpp>

#include <Sloppy/libSloppy.h>
#include <Sloppy/Logger/Logger.h>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "Transaction.h"

using namespace std;

namespace SqliteOverlay
{
  SqliteDatabase::SqliteDatabase(string dbFileName, bool createNew)
    :dbPtr{nullptr}, log{nullptr}, changeCounter_reset(0), curTrans{nullptr}
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
      throw std::runtime_error("No memory for allocating sqlite instance");
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
    log->trace("Ready for use");

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

  int SqliteDatabase::copyDatabaseContents(sqlite3 *srcHandle, sqlite3 *dstHandle)
  {
    // check parameters
    if ((srcHandle == nullptr) || (dstHandle == nullptr)) return SQLITE_ERROR;

    // initialize backup procedure
    auto bck = sqlite3_backup_init(dstHandle, "main", srcHandle, "main");
    if (bck == nullptr)
    {
      return SQLITE_ERROR;
    }

    // copy all data at once
    int err = sqlite3_backup_step(bck, -1);
    if (err != SQLITE_DONE)
    {
      // clean up and free ressources, but do not
      // overwrite the error code returned by
      // sqlite3_backup_step() above
      sqlite3_backup_finish(bck);

      return err;
    }

    // clean up and return
    err = sqlite3_backup_finish(bck);
    return err;
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

    // Here, the SQLite result code can be SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if ((errCodeOut != nullptr) && (*errCodeOut == SQLITE_ROW)) *errCodeOut = SQLITE_DONE;

    return (stmt == nullptr) ? false : stmt->getInt(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryInt(const upSqlStatement& stmt, int* out, int* errCodeOut) const
  {
    bool isSuccess = execScalarQuery_prep<int>(stmt, out, errCodeOut);

    // Here, the SQLite result code can be SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if ((errCodeOut != nullptr) && (*errCodeOut == SQLITE_ROW)) *errCodeOut = SQLITE_DONE;

    return isSuccess ? stmt->getInt(0, out) : false;
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

    // Here, the SQLite result code is SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if (errCodeOut != nullptr) *errCodeOut = SQLITE_DONE;

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

    // Here, the SQLite result code can be SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if ((errCodeOut != nullptr) && (*errCodeOut == SQLITE_ROW)) *errCodeOut = SQLITE_DONE;

    return (stmt == nullptr) ? false : stmt->getDouble(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryDouble(const upSqlStatement& stmt, double* out, int* errCodeOut) const
  {
    bool isSuccess = execScalarQuery_prep<double>(stmt, out, errCodeOut);

    // Here, the SQLite result code can be SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if ((errCodeOut != nullptr) && (*errCodeOut == SQLITE_ROW)) *errCodeOut = SQLITE_DONE;

    return isSuccess ? stmt->getDouble(0, out) : false;
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

    // Here, the SQLite result code is SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if (errCodeOut != nullptr) *errCodeOut = SQLITE_DONE;

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

    // Here, the SQLite result code can be SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if ((errCodeOut != nullptr) && (*errCodeOut == SQLITE_ROW)) *errCodeOut = SQLITE_DONE;

    return (stmt == nullptr) ? false : stmt->getString(0, out);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::execScalarQueryString(const upSqlStatement& stmt, string* out, int* errCodeOut) const
  {
    bool isSuccess = execScalarQuery_prep<string>(stmt, out, errCodeOut);

    // Here, the SQLite result code can be SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if ((errCodeOut != nullptr) && (*errCodeOut == SQLITE_ROW)) *errCodeOut = SQLITE_DONE;

    return isSuccess ? stmt->getString(0, out) : false;
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

    // Here, the SQLite result code is SQLITE_ROW, because we
    // called step() on the statememnt only once. If the statement
    // would retrieve only one row, the next call to step() would yield
    // SQLITE_DONE. If it would retrieve multiple rows, the next call
    // to step() would yield SQLITE_ROW again.
    //
    // So we can't predict the result of the next call to step().
    //
    // But since all other queries always return SQLITE_DONE, we
    // manually overwrite SQLite's last result code here
    if (errCodeOut != nullptr) *errCodeOut = SQLITE_DONE;

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

  void SqliteDatabase::viewCreationHelper(const string& viewName, const string& selectStmt, int* errCodeOut)
  {
    string sql = "CREATE VIEW IF NOT EXISTS";

    sql += " " + viewName + " AS ";
    sql += selectStmt;
    execNonQuery(sql, errCodeOut);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const Sloppy::StringList &colNames, bool isUnique, int* errCodeOut)
  {
    if (idxName.empty()) return;
    if (!(hasTable(tabName))) return;

    string sql = "CREATE ";
    if (isUnique) sql += "UNIQUE ";
    sql += "INDEX IF NOT EXISTS ";
    sql += idxName + " ON " + tabName + " (";
    sql += Sloppy::commaSepStringFromStringList(colNames) + ")";
    execNonQuery(sql, errCodeOut);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const string &colName, bool isUnique, int* errCodeOut)
  {
    Sloppy::StringList colList;
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

  Sloppy::StringList SqliteDatabase::allTableNames(bool getViews)
  {
    Sloppy::StringList result;

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

  Sloppy::StringList SqliteDatabase::allViewNames()
  {
    return allTableNames(true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::hasTable(const string& name, bool isView)
  {
    Sloppy::StringList allTabs = allTableNames(isView);
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
        default:
          return "";
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
    return (sqlite3_get_autocommit(dbPtr.get()) != 0);
  }

  //----------------------------------------------------------------------------

  unique_ptr<Transaction> SqliteDatabase::startTransaction(TRANSACTION_TYPE tt, TRANSACTION_DESTRUCTOR_ACTION dtorAct, int* errCodeOut)
  {
    return Transaction::startNew(this, tt, dtorAct, errCodeOut);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::setLogLevel(Sloppy::Logger::SeverityLevel newMinLvl)
  {
    log->setMinLogLevel(newMinLvl);
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

  bool SqliteDatabase::copyTable(const string& srcTabName, const string& dstTabName, int* errCodeOut, bool copyStructureOnly)
  {
    // ensure validity of parameters
    if (srcTabName.empty()) return false;
    if (dstTabName.empty()) return false;
    if (!(hasTable(srcTabName))) return false;
    if (hasTable(dstTabName)) return false;

    // retrieve the CREATE TABLE statement that describes the source table's structure
    int err;
    auto stmt = prepStatement("SELECT sql FROM sqlite_master WHERE type='table' AND name=?", &err);
    if (errCodeOut != nullptr) *errCodeOut = err;
    if (err != SQLITE_OK) return false;
    stmt->bindString(1, srcTabName);
    auto _sqlCreate = execScalarQueryString(stmt, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;
    if (_sqlCreate == nullptr) return false;
    if (err != SQLITE_DONE) return false;
    if (_sqlCreate->isNull()) return false;
    string sqlCreate = _sqlCreate->get();

    // Modify the string to be applicable to the new database name
    //
    // The string starts with "CREATE TABLE srcName (id ...". We search
    // for the opening parenthesis and replace everthing before that
    // character with "CREATE TABLE dstName ".
    auto posParenthesis = sqlCreate.find_first_of("(");
    if (posParenthesis == string::npos) return false;
    sqlCreate.replace(0, posParenthesis-1, "CREATE TABLE " + dstTabName);

    // before we create the new table and copy the contents, we
    // explicitly start a transaction to be able to restore the
    // original database state in case of errors
    auto tr = startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
    if (tr == nullptr) return false;
    if (err != SQLITE_DONE) return false;

    // actually create the new table
    bool isSuccess = execNonQuery(sqlCreate, &err);
    if (errCodeOut != nullptr) *errCodeOut = err;
    if (!isSuccess)
    {
      tr->rollback();
      return false;
    }

    // if we're only requested to copy the schema, skip
    // the conent copying
    if (!copyStructureOnly)
    {
      string sqlCopy = "INSERT INTO " + dstTabName + " SELECT * FROM " + srcTabName;
      stmt = prepStatement(sqlCopy, &err);
      if (errCodeOut != nullptr) *errCodeOut = err;
      if (err != SQLITE_OK) return false;

      isSuccess = execNonQuery(stmt, &err);
      if (errCodeOut != nullptr) *errCodeOut = err;
      if (!isSuccess)
      {
        tr->rollback();
        return false;
      }
    }

    // we're done. Commit all changes at once
    tr->commit(&err);
    if (errCodeOut != nullptr) *errCodeOut = err;

    return (err == SQLITE_DONE);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::backupToFile(const string &dstFileName, int* errCodeOut)
  {
    // check parameter validity
    if (dstFileName.empty())
    {
      if (errCodeOut != nullptr) *errCodeOut = SQLITE_ERROR;
      return false;
    }

    // open the destination database
    sqlite3* dstDb;
    int err = sqlite3_open(dstFileName.c_str(), &dstDb);
    if (err != SQLITE_OK)
    {
      if (errCodeOut != nullptr) *errCodeOut = err;
      return false;
    }
    if (dstDb == nullptr)
    {
      if (errCodeOut != nullptr) *errCodeOut = SQLITE_ERROR;
      return false;
    }

    // copy the contents
    err = copyDatabaseContents(dbPtr.get(), dstDb);

    // close the destination database and return the
    // error code of the copy process or of the
    // close procedure
    int closeErr = sqlite3_close(dstDb);
    if (err != SQLITE_OK)
    {
      // error during copying ==> return the copy error
      if (errCodeOut != nullptr) *errCodeOut = err;
      return false;
    }

    // error during closing ==> return the close error
    if (errCodeOut != nullptr) *errCodeOut = closeErr;
    return (closeErr == SQLITE_OK);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::restoreFromFile(const string &srcFileName, int *errCodeOut)
  {
    // check parameter validity
    if (srcFileName.empty())
    {
      if (errCodeOut != nullptr) *errCodeOut = SQLITE_ERROR;
      return false;
    }

    // open the source database
    sqlite3* srcDb;
    int err = sqlite3_open(srcFileName.c_str(), &srcDb);
    if (err != SQLITE_OK)
    {
      if (errCodeOut != nullptr) *errCodeOut = err;
      return false;
    }
    if (srcDb == nullptr)
    {
      if (errCodeOut != nullptr) *errCodeOut = SQLITE_ERROR;
      return false;
    }

    // copy the contents
    err = copyDatabaseContents(srcDb, dbPtr.get());

    // close the source database and return the
    // error code of the copy process or of the
    // close procedure
    int closeErr = sqlite3_close(srcDb);
    if (err != SQLITE_OK)
    {
      // error during copying ==> return the copy error
      if (errCodeOut != nullptr) *errCodeOut = err;
      return false;
    }

    // error during closing ==> return the close error
    if (errCodeOut != nullptr) *errCodeOut = closeErr;
    return (closeErr == SQLITE_OK);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::isDirty()
  {
    return (sqlite3_total_changes(dbPtr.get()) != changeCounter_reset);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::resetDirtyFlag()
  {
    changeCounter_reset = sqlite3_total_changes(dbPtr.get());
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getDirtyCounter()
  {
    return sqlite3_total_changes(dbPtr.get());
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

  //----------------------------------------------------------------------------

  string conflictClause2String(CONFLICT_CLAUSE cc)
  {
    switch (cc)
    {
    case CONFLICT_CLAUSE::ABORT:
      return "ABORT";

    case CONFLICT_CLAUSE::FAIL:
      return "FAIL";

    case CONFLICT_CLAUSE::IGNORE:
      return "IGNORE";

    case CONFLICT_CLAUSE::REPLACE:
      return "REPLACE";

    case CONFLICT_CLAUSE::ROLLBACK:
      return "ROLLBACK";

    default:
      return "";
    }

    return "";  // includes __NOT_SET
  }

  //----------------------------------------------------------------------------

  string buildColumnConstraint(bool isUnique, CONFLICT_CLAUSE uniqueConflictClause, bool notNull, CONFLICT_CLAUSE notNullConflictClause,
                               bool hasDefault, const string& defVal)
  {
    string result;

    if (isUnique)
    {
      result += "UNIQUE";
      if (uniqueConflictClause != CONFLICT_CLAUSE::__NOT_SET)
      {
        result += " ON CONFLICT " + conflictClause2String(uniqueConflictClause);
      }
    }

    if (notNull)
    {
      if (!(result.empty())) result += " ";

      result += "NOT NULL";
      if (notNullConflictClause != CONFLICT_CLAUSE::__NOT_SET)
      {
        result += " ON CONFLICT " + conflictClause2String(notNullConflictClause);
      }
    }

    if (hasDefault)
    {
      if (!(result.empty())) result += " ";

      result += "DEFAULT '";
      if (!(defVal.empty())) result += defVal;
      result += "'";
    }

    return result;
  }

  //----------------------------------------------------------------------------

  string buildForeignKeyClause(const string& referedTable, CONSISTENCY_ACTION onDelete, CONSISTENCY_ACTION onUpdate, string referedColumn)
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
        default:
          return "";
      }
      return "";
    };

    string result = "REFERENCES " + referedTable + "(" + referedColumn + ")";

    if (onDelete != CONSISTENCY_ACTION::__NOT_SET)
    {
      result += " ON DELETE " + ca2string(onDelete);
    }

    if (onUpdate != CONSISTENCY_ACTION::__NOT_SET)
    {
      if (!(result.empty())) result += " ";
      result += " ON UPDATE " + ca2string(onUpdate);
    }

    return result;
  }

}
