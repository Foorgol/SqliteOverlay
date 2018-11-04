#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"

using namespace SqliteOverlay;
namespace bfs = boost::filesystem;

TEST_F(DatabaseTestScenario, DatabaseCtor)
{
  // create a new, empty database
  string dbFileName = getSqliteFileName();
  bfs::path dbPathObj(dbFileName);

  ASSERT_FALSE(bfs::exists(dbPathObj));
  ASSERT_THROW(SqliteDatabase(dbFileName, OpenMode::OpenExisting_RW, false), std::invalid_argument);
  ASSERT_THROW(SqliteDatabase(dbFileName, OpenMode::OpenExisting_RO, false), std::invalid_argument);
  SqliteDatabase db{dbFileName, OpenMode::OpenOrCreate_RW, false};
  ASSERT_TRUE(bfs::exists(dbPathObj));
  ASSERT_TRUE(db.isAlive());

  // close the database connection
  db.close();
  ASSERT_FALSE(db.isAlive());

  // open an existing database
  ASSERT_THROW(SqliteDatabase(dbFileName, OpenMode::ForceNew, false), std::invalid_argument);
  db = SqliteDatabase{dbFileName, OpenMode::OpenExisting_RO, false};
  db.close();
  db = SqliteDatabase{dbFileName, OpenMode::OpenExisting_RW, false};
  db.close();
  db = SqliteDatabase{dbFileName, OpenMode::OpenOrCreate_RW, false};
  db.close();

  // clean-up and test "force new"
  ASSERT_TRUE(bfs::remove(dbPathObj));
  ASSERT_FALSE(bfs::exists(dbPathObj));
  db = SqliteDatabase{dbFileName, OpenMode::ForceNew, false};
  ASSERT_TRUE(bfs::exists(dbPathObj));
  ASSERT_TRUE(bfs::remove(dbPathObj));
  ASSERT_FALSE(bfs::exists(dbPathObj));

  // try to open a non-existing / invalid / empty file
  ASSERT_THROW(SqliteDatabase("", OpenMode::OpenOrCreate_RW, false), std::invalid_argument);

  // try invalid flag combinations
  ASSERT_THROW(SqliteDatabase(":memory:", OpenMode::OpenExisting_RW, false), std::invalid_argument);
  ASSERT_THROW(SqliteDatabase(":memory:", OpenMode::OpenExisting_RO, false), std::invalid_argument);
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
