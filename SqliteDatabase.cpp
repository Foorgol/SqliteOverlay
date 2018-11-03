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

  bool SqliteDatabase::copyDatabaseContents(sqlite3 *srcHandle, sqlite3 *dstHandle)
  {
    // check parameters
    if ((srcHandle == nullptr) || (dstHandle == nullptr))
    {
      throw std::invalid_argument("copyDatabaseContents(): called with nullptr for database handles");
    }
    if (srcHandle == dstHandle)
    {
      throw std::invalid_argument("copyDatabaseContents(): identical source and destination DB");
    }

    // initialize backup procedure
    sqlite3_backup* bck = sqlite3_backup_init(dstHandle, "main", srcHandle, "main");
    if (bck == nullptr)
    {
      int err = sqlite3_errcode(dstHandle);
      if (err == SQLITE_BUSY)
      {
        throw BusyException("copyDatabaseContents(): destination database is locked");
      }
      throw GenericSqliteException(err, "copyDatabaseContents(): error in the destination database during backup_init()");
    }

    // copy all data at once
    //
    // Regardless of the result, we call backup_finish because the
    // manual demands to call the finish-function even after failures
    // to release ressources
    int errStep = sqlite3_backup_step(bck, -1);
    int errFinish = sqlite3_backup_finish(bck);

    if (errStep != SQLITE_DONE)
    {
      if (errStep == SQLITE_BUSY)
      {
        throw BusyException("copyDatabaseContents(): source or destination database is locked, backup_step() failed");
      }
      throw GenericSqliteException(errStep, "copyDatabaseContents(): backup_step() failed");
    }

    return (errFinish == SQLITE_OK); // should always be `true`, otherwise we would have thrown after `backup_step()`
  }

  //----------------------------------------------------------------------------

  SqliteDatabase::SqliteDatabase()
    :SqliteDatabase(":memory:", OpenMode::OpenOrCreate_RW, true)
  {}

  //----------------------------------------------------------------------------

  SqliteDatabase::SqliteDatabase(string dbFileName, OpenMode om, bool populate)
  {
    // check if the filename is valid
    if (dbFileName.empty())
    {
      throw invalid_argument("Invalid database file name");
    }

    // check if we're only creating a memory database
    bool isInMem = (dbFileName == ":memory:");

    // check if the file exists
    if (!isInMem)
    {
      struct stat buffer;
      bool hasFile = (stat(dbFileName.c_str(), &buffer) == 0);

      if (!hasFile && ((om == OpenMode::OpenExisting_RO) || (om == OpenMode::OpenExisting_RW)))
      {
        throw invalid_argument("Database file not existing and new file shall not be created");
      }

      if (hasFile && (om == OpenMode::ForceNew))
      {
        throw invalid_argument("Database existing but creation of a new file is mandatory");
      }
    }
    if (isInMem && ((om == OpenMode::OpenExisting_RO) || (om == OpenMode::OpenExisting_RW)))
    {
      throw invalid_argument("In-memory file name for database, but opening of an existing file requested");
    }

    // determine the flags for the call to sqlite_open
    int oFlags = 0;
    switch (om)
    {
    case OpenMode::ForceNew:
      oFlags = SQLITE_OPEN_CREATE;
      break;

    case OpenMode::OpenExisting_RO:
      oFlags = SQLITE_OPEN_READONLY;
      break;

    case OpenMode::OpenExisting_RW:
      oFlags = SQLITE_OPEN_READWRITE;
      break;

    case OpenMode::OpenOrCreate_RW:
      oFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
      break;

    default:
      oFlags = SQLITE_OPEN_READONLY;  // most conservative setting
    }

    // try to open the database
    int err = sqlite3_open_v2(dbFileName.c_str(), &dbPtr, oFlags, nullptr);
    if (dbPtr == nullptr)
    {
      throw std::runtime_error("No memory for allocating sqlite instance");
    }
    if (err != SQLITE_OK)
    {
      // delete the connection we've just created
      sqlite3_close(dbPtr);
      dbPtr = nullptr;

      throw GenericSqliteException(err, "SqliteDatabase ctor");
    }

    // we're all set
    //
    // Explicitly enable support for foreign keys
    // and disable synchronous writes for better performance.
    //
    // If requested, populate tables and views.
    //
    // Should anything go wrong, close the connection and
    // re-throw the original exception
    try
    {
      execNonQuery("PRAGMA foreign_keys = ON");
      enforceSynchronousWrites(false);
      if (populate && (om != OpenMode::OpenExisting_RO))
      {
        Transaction t = Transaction(this, TransactionType::Deferred, TransactionDtorAction::Rollback);
        populateTables();
        populateViews();
        t.commit();
      }
      resetDirtyFlag();
    }
    catch (...)
    {
      // delete the connection we've just created
      sqlite3_close(dbPtr);
      dbPtr = nullptr;

      throw;
    }
  }

  //----------------------------------------------------------------------------

  SqliteDatabase::~SqliteDatabase()
  {
    // clear all cached DbTab objects
    resetTabCache();

    // close the database, if not already done so
    // no need to react to errors here because we're in
    // the dtor anyway....
    if (dbPtr != nullptr) sqlite3_close(dbPtr);
  }

  //----------------------------------------------------------------------------

  SqliteDatabase& SqliteDatabase::operator=(SqliteDatabase&& other)
  {
    dbPtr = other.dbPtr;
    other.dbPtr = nullptr;

    // move, not copy the table cache
    tabCache = std::move(other.tabCache);

    // transfer the dirty counters; we don't need to
    // reset them in 'other` because it becomes unusual
    // anyway
    changeCounter_reset = other.changeCounter_reset;
    dataVersion_reset = other.changeCounter_reset;
  }

  //----------------------------------------------------------------------------

  SqliteDatabase::SqliteDatabase(SqliteDatabase&& other)
  {
    operator=(std::move(other));
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::close()
  {
    if (dbPtr == nullptr) return;

    int result = sqlite3_close(dbPtr);

    if (result != SQLITE_OK)
    {
      if (result == SQLITE_BUSY)
      {
        throw BusyException("close()");
      }
      throw GenericSqliteException(result, "close()");
    }

    resetTabCache();
    dbPtr = nullptr;
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

  void SqliteDatabase::viewCreationHelper(const string& viewName, const string& selectStmt) const
  {
    string sql = "CREATE VIEW IF NOT EXISTS ";

    sql += " " + viewName + " AS ";
    sql += selectStmt;
    execNonQuery(sql);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName,
                                           const Sloppy::StringList &colNames, bool isUnique) const
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

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &idxName,
                                           const string &colName, bool isUnique) const
  {
    if (colName.empty()) return;

    Sloppy::StringList colList;
    colList.push_back(colName);
    indexCreationHelper(tabName, idxName, colList, isUnique);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::indexCreationHelper(const string &tabName, const string &colName, bool isUnique) const
  {
    if (tabName.empty()) return;
    if (colName.empty()) return;

    // auto-create a canonical index name
    string idxName = tabName + "_" + colName;

    indexCreationHelper(tabName, idxName, colName, isUnique);
  }

  //----------------------------------------------------------------------------

  Sloppy::StringList SqliteDatabase::allTableNames(bool getViews) const
  {
    Sloppy::StringList result;

    string sql = "SELECT name FROM sqlite_master WHERE type='";
    sql += getViews ? "view" : "table";
    sql +="';";
    auto stmt = execContentQuery(sql);
    while (stmt.hasData())
    {
      string tabName = stmt.getString(0);

      // skip a special, internal table
      if (tabName != "sqlite_sequence")
      {
        result.push_back(tabName);
      }

      stmt.step();
    }

    return result;
  }

  //----------------------------------------------------------------------------

  Sloppy::StringList SqliteDatabase::allViewNames() const
  {
    return allTableNames(true);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::hasTable(const string& name, bool isView) const
  {
    SqlStatement stmt = prepStatement("SELECT COUNT(name) FROM sqlite_master WHERE type='?1' AND name='?2'");
    if (isView)
    {
      stmt.bindString(1, "view");
    } else {
      stmt.bindString(1, "view");
    }
    stmt.bindString(2, name);

    return (execScalarQueryInt(stmt) != 0);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::hasView(const string& name) const
  {
    return hasTable(name, true);
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getLastInsertId() const
  {
    return sqlite3_last_insert_rowid(dbPtr);
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getRowsAffected() const
  {
    return sqlite3_changes(dbPtr);
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::isAutoCommit() const
  {
    return (sqlite3_get_autocommit(dbPtr) != 0);
  }

  //----------------------------------------------------------------------------

  Transaction SqliteDatabase::startTransaction(TransactionType tt, TransactionDtorAction _dtorAct) const
  {
    return Transaction(this, tt, _dtorAct);
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

  bool SqliteDatabase::copyTable(const string& srcTabName, const string& dstTabName, bool copyStructureOnly) const
  {
    // ensure validity of parameters
    if (srcTabName.empty()) return false;
    if (dstTabName.empty()) return false;
    if (!(hasTable(srcTabName))) return false;
    if (hasTable(dstTabName)) return false;

    // retrieve the CREATE TABLE statement that describes the source table's structure
    auto stmt = prepStatement("SELECT sql FROM sqlite_master WHERE type='table' AND name=?");
    stmt.bindString(1, srcTabName);
    string sqlCreate = execScalarQueryString(stmt);
    if (sqlCreate.empty()) return false;

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
    Transaction tr = startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);

    // actually create the new table
    try
    {
      execNonQuery(sqlCreate);
    }
    catch (...)
    {
      tr.rollback();
      throw;
    }

    // if we're only requested to copy the schema, skip
    // the conent copying
    if (!copyStructureOnly)
    {
      string sqlCopy = "INSERT INTO " + dstTabName + " SELECT * FROM " + srcTabName;
      SqlStatement cpyStmt = prepStatement(sqlCopy);

      execNonQuery(cpyStmt);
      try
      {
        execNonQuery(cpyStmt);
      }
      catch (...)
      {
        tr.rollback();
        throw;
      }
    }

    // we're done. Commit all changes at once
    tr.commit();

    return true;
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::backupToFile(const string &dstFileName) const
  {
    // check parameter validity
    if (dstFileName.empty())
    {
      throw std::invalid_argument("backupToFile: called without filename");
    }

    // open the destination database
    sqlite3* dstDb;
    int err = sqlite3_open(dstFileName.c_str(), &dstDb);
    if (err == SQLITE_BUSY)
    {
      throw BusyException("backupToFile(): weird... received SQLITE_BUSY when opening the destination database...");
    }
    if (err != SQLITE_OK)
    {
      throw GenericSqliteException(err, "backupToFile(): opening destination database");
    }
    if (dstDb == nullptr)
    {
      throw GenericSqliteException(SQLITE_ERROR, "backupToFile(): sqlite3_open() for destination database returned nullptr");
    }

    // copy the contents
    bool isOkay = copyDatabaseContents(dbPtr, dstDb);

    // close the destination database
    //
    // no error checking here, because close() only fails with
    // unfinalized prepared statements or unfinished sqlite3_backup objects.
    // And we can guarantee that this is impossible on the destination DB handle
    // because we only opened it a few lines of code above and because
    // `copyDatabaseContents()` always calls 'backup_finish()'
    sqlite3_close(dstDb);

    return isOkay;
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::restoreFromFile(const string &srcFileName)
  {
    // check parameter validity
    if (srcFileName.empty())
    {
      throw std::invalid_argument("restoreFromFile: called without filename");
    }

    // open the source database
    sqlite3* srcDb;
    int err = sqlite3_open(srcFileName.c_str(), &srcDb);
    if (err == SQLITE_BUSY)
    {
      throw BusyException("restoreFromFile(): weird... received SQLITE_BUSY when opening the destination database...");
    }
    if (err != SQLITE_OK)
    {
      throw GenericSqliteException(err, "restoreFromFile(): opening destination database");
    }
    if (srcDb == nullptr)
    {
      throw GenericSqliteException(SQLITE_ERROR, "restoreFromFile(): sqlite3_open() for destination database returned nullptr");
    }

    // copy the contents and clear all cached DbTab pointers
    bool isOkay = copyDatabaseContents(srcDb, dbPtr);
    resetTabCache();

    // close the source database
    //
    // no error checking here, because close() only fails with
    // unfinalized prepared statements or unfinished sqlite3_backup objects.
    // And we can guarantee that this is impossible on the source DB handle
    // because we only opened it a few lines of code above and because
    // `copyDatabaseContents()` always calls 'backup_finish()'
    sqlite3_close(srcDb);

    return isOkay;
  }

  //----------------------------------------------------------------------------

  bool SqliteDatabase::isDirty() const
  {
    int dv = execScalarQueryInt("PRAGMA data_version;");
    return ((sqlite3_total_changes(dbPtr) != changeCounter_reset) || (dv != dataVersion_reset));
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::resetDirtyFlag()
  {
    changeCounter_reset = sqlite3_total_changes(dbPtr);
    dataVersion_reset = execScalarQueryInt("PRAGMA data_version;");
  }

  //----------------------------------------------------------------------------

  int SqliteDatabase::getLocalChangeCounter() const
  {
    return sqlite3_total_changes(dbPtr);
  }

  //----------------------------------------------------------------------------

  void SqliteDatabase::setBusyTimeout(int ms)
  {
    sqlite3_busy_timeout(dbPtr, ms);
  }

  //----------------------------------------------------------------------------

  SqliteDatabase SqliteDatabase::duplicateConnection(bool readOnly)
  {
    const char* fName = sqlite3_db_filename(dbPtr, "main");
    if (fName == nullptr)
    {
      throw std::invalid_argument("cloneConnection(): called on a temporary or in-memory database");
    }
    string fn{fName};

    OpenMode om = readOnly ? OpenMode::OpenExisting_RO : OpenMode::OpenExisting_RW;

    return SqliteDatabase{fn, om, false};
  }

  //----------------------------------------------------------------------------

  SqlStatement SqliteDatabase::prepStatement(const string& sqlText) const
  {
    return SqlStatement{dbPtr, sqlText};
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

  ColumnDataType string2Affinity(const string& colType)
  {
    Sloppy::estring t{colType};
    t.toUpper();

    //
    // implement the "Column Affinity Rules" stated in the
    // SQLite documentation
    //

    // Rule 1, integer affinity
    if (t.contains("INT"))
    {
      return ColumnDataType::Integer;
    }

    // Rule 2, text affinity
    for (const string& s : {"CHAR", "CLOB", "TEXT"})
    {
      if (t.contains(s))
      {
        return ColumnDataType::Text;
      }
    }

    // Rule 3, blob affinity
    if (colType.empty() || (t.contains("BLOB")))
    {
      return ColumnDataType::Blob;
    }

    // Rule 4, real affinity
    for (const string& s : {"REAL", "FLOA", "DOUB"})
    {
      if (t.contains(s))
      {
        return ColumnDataType::Float;
      }
    }

    // Rule 5, numeric affinity is the default
    return ColumnDataType::NumericAffinity;
  }


}
