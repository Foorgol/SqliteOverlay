#include <sys/stat.h>

#include <boost/date_time/local_time/local_time.hpp>

#include <Sloppy/String.h>
#include <Sloppy/Utils.h>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "Transaction.h"

using namespace std;

namespace SqliteOverlay
{
  SqliteDatabase::SqliteDatabase(string dbFileName, bool createNew)
    :dbPtr{nullptr}, log{nullptr}, changeCounter_reset(0)
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

  bool SqliteDatabase::close(PrimaryResultCode* errCode)
  {
    if (dbPtr == nullptr) return true;

    int result = sqlite3_close(dbPtr.get());
    Sloppy::assignIfNotNull<PrimaryResultCode>(errCode, static_cast<PrimaryResultCode>(result));

    if (result != SQLITE_OK) return false;

    resetTabCache();
    dbPtr = nullptr;

    return true;
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

  void SqliteDatabase::execNonQuery(const string& sqlStatement) const
  {
    SqlStatement stmt = prepStatement(sqlStatement);
    return execNonQuery(stmt);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::execNonQuery(SqlStatement& stmt) const
  {
    // execute the statement
    while (stmt.step()) {};
  }

  //----------------------------------------------------------------------------

  SqlStatement SqliteDatabase::execContentQuery(const string& sqlStatement) const
  {
    SqlStatement stmt = prepStatement(sqlStatement);
    stmt.step();

    return stmt;
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::execScalarQueryInt(const string& sqlStatement) const
  {
    optional<int> v = execScalarQueryIntOrNull(sqlStatement);
    if (!(v.has_value()))
    {
      throw NullValueException("SQL = " + sqlStatement);
    }

    return v.value();
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::execScalarQueryInt(SqlStatement& stmt) const
  {
    optional<int> v = execScalarQueryIntOrNull(stmt);
    if (!(v.has_value()))
    {
      throw NullValueException();
    }

    return v.value();
  }

  //----------------------------------------------------------------------------

  optional<int> SqliteDatabase::execScalarQueryIntOrNull(const string& sqlStatement) const
  {
    // throws invalid_argument or SqlStatementCreationError
    SqlStatement stmt = prepStatement(sqlStatement);

    // throws BusyException or GenericSqliteException
    stmt.step();

    // throws NoDataException
    return stmt.isNull(0) ? optional<int>{} : stmt.getInt(0);
  }

  //----------------------------------------------------------------------------

  optional<int> SqliteDatabase::execScalarQueryIntOrNull(SqlStatement& stmt) const
  {
    // throws BusyException or GenericSqliteException
    stmt.step();

    // throws NoDataException
    return stmt.isNull(0) ? optional<int>{} : stmt.getInt(0);
  }

  //----------------------------------------------------------------------------

  double SqliteDatabase::execScalarQueryDouble(const string& sqlStatement) const
  {
    optional<double> v = execScalarQueryDoubleOrNull(sqlStatement);
    if (!(v.has_value()))
    {
      throw NullValueException("SQL = " + sqlStatement);
    }

    return v.value();
  }

  //----------------------------------------------------------------------------

  double SqliteDatabase::execScalarQueryDouble(SqlStatement& stmt) const
  {
    optional<double> v = execScalarQueryDoubleOrNull(stmt);
    if (!(v.has_value()))
    {
      throw NullValueException();
    }

    return v.value();
  }

  //----------------------------------------------------------------------------

  optional<double> SqliteDatabase::execScalarQueryDoubleOrNull(const string& sqlStatement) const
  {
    // throws invalid_argument or SqlStatementCreationError
    SqlStatement stmt = prepStatement(sqlStatement);

    // throws BusyException or GenericSqliteException
    stmt.step();

    // throws NoDataException
    return stmt.isNull(0) ? optional<double>{} : stmt.getDouble(0);
  }

  //----------------------------------------------------------------------------

  optional<double> SqliteDatabase::execScalarQueryDoubleOrNull(SqlStatement& stmt) const
  {
    // throws BusyException or GenericSqliteException
    stmt.step();

    // throws NoDataException
    return stmt.isNull(0) ? optional<double>{} : stmt.getDouble(0);
  }

  //----------------------------------------------------------------------------

  string SqliteDatabase::execScalarQueryString(const string& sqlStatement) const
  {
    optional<string> v = execScalarQueryStringOrNull(sqlStatement);
    if (!(v.has_value()))
    {
      throw NullValueException("SQL = " + sqlStatement);
    }

    return v.value();
  }

  //----------------------------------------------------------------------------

  string SqliteDatabase::execScalarQueryString(SqlStatement& stmt) const
  {
    optional<string> v = execScalarQueryStringOrNull(stmt);
    if (!(v.has_value()))
    {
      throw NullValueException();
    }

    return v.value();
  }

  //----------------------------------------------------------------------------

  optional<string> SqliteDatabase::execScalarQueryStringOrNull(const string& sqlStatement) const
  {
    // throws invalid_argument or SqlStatementCreationError
    SqlStatement stmt = prepStatement(sqlStatement);

    // throws BusyException or GenericSqliteException
    stmt.step();

    // throws NoDataException
    return stmt.isNull(0) ? optional<string>{} : stmt.getString(0);
  }

  //----------------------------------------------------------------------------

  optional<string> SqliteDatabase::execScalarQueryStringOrNull(SqlStatement& stmt) const
  {
    // throws BusyException or GenericSqliteException
    stmt.step();

    // throws NoDataException
    return stmt.isNull(0) ? optional<string>{} : stmt.getString(0);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::enforceSynchronousWrites(bool syncOn) const
  {
    string sql = "PRAGMA synchronous = ";
    if (syncOn)
    {
      sql += "ON";
    }
    else
    {
      sql += "OFF";
    }

    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::viewCreationHelper(const string& viewName, const string& selectStmt)
  {
    string sql = "CREATE VIEW IF NOT EXISTS ";

    sql += " " + viewName + " AS ";
    sql += selectStmt;
    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const Sloppy::StringList &colNames, bool isUnique)
  {
    if (tabName.empty()) return;
    if (idxName.empty()) return;
    if (colNames.empty()) return;

    if (!(hasTable(tabName)))
    {
      throw NoSuchTableException("indexCreationHelper() called with table name " + tabName);
    }

    Sloppy::estring sql{"CREATE %1 INDEX IF NOT EXISTS %2 ON %3 (%4)"};
    if (isUnique)
    {
      sql.arg("UNIQUE");
    } else {
      sql.arg("");
    }
    sql.arg(idxName);
    sql.arg(tabName);
    sql.arg(Sloppy::estring{colNames, ","});
    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName, const string &colName, bool isUnique)
  {
    if (colName.empty()) return;

    Sloppy::StringList colList;
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

  unique_ptr<Transaction> SqliteDatabase::startTransaction(TransactionType tt, TransactionDtorAction dtorAct, int* errCodeOut)
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
    auto tr = startTransaction(TransactionType::IMMEDIATE, TransactionDtorAction::ROLLBACK, &err);
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

  SqlStatement SqliteDatabase::prepStatement(const string& sqlText) const
  {
    return SqlStatement{dbPtr.get(), sqlText};
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

  string conflictClause2String(ConflictClause cc)
  {
    switch (cc)
    {
    case ConflictClause::Abort:
      return "ABORT";

    case ConflictClause::Fail:
      return "FAIL";

    case ConflictClause::Ignore:
      return "IGNORE";

    case ConflictClause::Replace:
      return "REPLACE";

    case ConflictClause::Rollback:
      return "ROLLBACK";

    default:
      return "";
    }

    return "";  // includes NoAction
  }

  //----------------------------------------------------------------------------

  string buildColumnConstraint(ConflictClause uniqueConflictClause, ConflictClause notNullConflictClause, optional<string> defaultVal)
  {
    string result;

    if (uniqueConflictClause != ConflictClause::NotUsed)
    {
      result += "UNIQUE ON CONFLICT " + conflictClause2String(uniqueConflictClause);
    }

    if (notNullConflictClause != ConflictClause::NotUsed)
    {
      if (!(result.empty())) result += " ";

      result += "NOT NULL ON CONFLICT " + conflictClause2String(notNullConflictClause);
    }

    if (defaultVal.has_value())
    {
      if (!(result.empty())) result += " ";

      result += "DEFAULT '" + defaultVal.value() + "'";
    }

    return result;
  }

  //----------------------------------------------------------------------------

  string buildForeignKeyClause(const string& referedTable, ConsistencyAction onDelete, ConsistencyAction onUpdate, string referedColumn)
  {
    // a little helper to translate a CONSISTENCY_ACTION value into a string
    auto ca2string = [](ConsistencyAction ca) -> string {
      switch (ca)
      {
        case ConsistencyAction::NoAction:
          return "NO ACTION";
        case ConsistencyAction::SetNull:
          return "SET NULL";
        case ConsistencyAction::SetDefault:
          return "SET DEFAULT";
        case ConsistencyAction::Cascade:
          return "CASCADE";
        case ConsistencyAction::Restrict:
          return "RESTRICT";
        default:
          return "";
      }
      return "";
    };

    string result = "REFERENCES " + referedTable + "(" + referedColumn + ")";

    result += " ON DELETE " + ca2string(onDelete);

    result += " ON UPDATE " + ca2string(onUpdate);

    return result;
  }

  //----------------------------------------------------------------------------


}
