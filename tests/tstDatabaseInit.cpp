
#include <filesystem>

#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "Changelog.h"

namespace fs = std::filesystem;
using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, DatabaseCtor)
{
  // create a new, empty database
  string dbFileName = getSqliteFileName();
  fs::path dbPathObj(dbFileName);

  ASSERT_FALSE(fs::exists(dbPathObj));
  ASSERT_THROW(SqliteDatabase(dbFileName, OpenMode::OpenExisting_RW), std::invalid_argument);
  ASSERT_THROW(SqliteDatabase(dbFileName, OpenMode::OpenExisting_RO), std::invalid_argument);
  SqliteDatabase db{dbFileName, OpenMode::OpenOrCreate_RW};
  ASSERT_TRUE(fs::exists(dbPathObj));
  ASSERT_TRUE(db.isAlive());

  // close the database connection
  db.close();
  ASSERT_FALSE(db.isAlive());

  // open an existing database, implicitly tests
  // basic move assignment
  ASSERT_THROW(SqliteDatabase(dbFileName, OpenMode::ForceNew), std::invalid_argument);
  db = SqliteDatabase{dbFileName, OpenMode::OpenExisting_RO};
  db.close();
  db = SqliteDatabase{dbFileName, OpenMode::OpenExisting_RW};
  db.close();
  db = SqliteDatabase{dbFileName, OpenMode::OpenOrCreate_RW};
  db.close();

  // clean-up and test "force new"
  ASSERT_TRUE(fs::remove(dbPathObj));
  ASSERT_FALSE(fs::exists(dbPathObj));
  db = SqliteDatabase{dbFileName, OpenMode::ForceNew};
  ASSERT_TRUE(fs::exists(dbPathObj));
  ASSERT_TRUE(fs::remove(dbPathObj));
  ASSERT_FALSE(fs::exists(dbPathObj));

  // try to open a non-existing / invalid / empty file
  ASSERT_THROW(SqliteDatabase("", OpenMode::OpenOrCreate_RW), std::invalid_argument);

  // try invalid flag combinations
  ASSERT_THROW(SqliteDatabase(":memory:", OpenMode::OpenExisting_RW), std::invalid_argument);
  ASSERT_THROW(SqliteDatabase(":memory:", OpenMode::OpenExisting_RO), std::invalid_argument);
}

TEST_F(DatabaseTestScenario, MoveOps)
{
  auto db = getScenario01();

  // enable the changelog and execute an insert
  db.enableChangeLog(true);
  db.execContentQuery("INSERT INTO t1(i) VALUES (12345)");
  ASSERT_EQ(1, db.getChangeLogLength());

  // move-construct a new instance
  SampleDB newDb{std::move(db)};
  ASSERT_FALSE(db.isAlive());
  ASSERT_EQ(0, db.getChangeLogLength());  // content has been moved
  ASSERT_EQ(1, newDb.getChangeLogLength());

  // execute another insert
  newDb.execContentQuery("INSERT INTO t1(i) VALUES (9876)");

  // the changelog should now have TWO entries!
  ASSERT_EQ(2, newDb.getChangeLogLength());
  auto cLog = newDb.getAllChangesAndClearQueue();
  newDb.disableChangeLog(true);
  ASSERT_EQ(2, cLog.size());
  ASSERT_EQ(SqliteOverlay::RowChangeAction::Insert, cLog[0].action);
  ASSERT_EQ(6, cLog[0].rowId);
  ASSERT_EQ(SqliteOverlay::RowChangeAction::Insert, cLog[1].action);
  ASSERT_EQ(7, cLog[1].rowId);

  //
  // Same for move assignment
  //

  newDb.enableChangeLog(true);
  newDb.execContentQuery("INSERT INTO t1(i) VALUES (5656)");
  ASSERT_EQ(1, newDb.getChangeLogLength());
  db = std::move(newDb);
  ASSERT_FALSE(newDb.isAlive());
  ASSERT_EQ(0, newDb.getChangeLogLength());  // content has been moved
  ASSERT_EQ(1, db.getChangeLogLength());

  db.execContentQuery("INSERT INTO t1(i) VALUES (2222222)");

  ASSERT_EQ(2, db.getChangeLogLength());
  cLog = db.getAllChangesAndClearQueue();
  db.disableChangeLog(true);
  ASSERT_EQ(2, cLog.size());
  ASSERT_EQ(SqliteOverlay::RowChangeAction::Insert, cLog[0].action);
  ASSERT_EQ(8, cLog[0].rowId);
  ASSERT_EQ(SqliteOverlay::RowChangeAction::Insert, cLog[1].action);
  ASSERT_EQ(9, cLog[1].rowId);
}

//----------------------------------------------------------------

/*
TEST_F(DatabaseTestScenario, PopulateTablesAndViews)
{
  // create a new, empty database
  string dbFileName = getSqliteFileName();
  bfs::path dbPathObj(dbFileName);

  ASSERT_FALSE(bfs::exists(dbPathObj));
  auto db = SqliteDatabase::get<SampleDB>(dbFileName, true);
  ASSERT_TRUE(bfs::exists(dbPathObj));

  // close the database connection
  db.reset(nullptr);

  // re-open the file and check that all
  // tables and views have been created
  auto raw = getRawDbHandle();
  string sql = "SELECT name FROM sqlite_master WHERE type='table';";
  vector<string> expectedTables;
  expectedTables.push_back("t1");
  expectedTables.push_back("t2");
  sqlite3_stmt* stmt = prepStatement(raw, sql);
  while (true)
  {
    int err = sqlite3_step(stmt);
    if (err == SQLITE_DONE) break;
    string tabName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

    auto i = expectedTables.begin();
    while (i != expectedTables.end())
    {
      if (*i == tabName)
      {
        i = expectedTables.erase(i);
      } else {
        ++i;
      }
    }
  }
  sqlite3_finalize(stmt);

  ASSERT_TRUE(expectedTables.empty());
}

*/
