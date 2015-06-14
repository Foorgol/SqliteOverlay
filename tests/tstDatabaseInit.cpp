#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"

using namespace SqliteOverlay;
namespace bfs = boost::filesystem;

TEST_F(DatabaseTestScenario, DatabaseInit)
{
  // create a new, empty database
  string dbFileName = getSqliteFileName();
  bfs::path dbPathObj(dbFileName);

  ASSERT_FALSE(bfs::exists(dbPathObj));
  auto db = SqliteDatabase::get<SampleDB>(dbFileName, true);
  ASSERT_TRUE(bfs::exists(dbPathObj));

  // close the database connection
  db.reset(nullptr);

  // open an existing database
  db = SqliteDatabase::get<SampleDB>(dbFileName, false);
  ASSERT_TRUE(db != nullptr);
  db.reset(nullptr);

  // clean-up
  ASSERT_TRUE(bfs::remove(dbPathObj));
  ASSERT_FALSE(bfs::exists(dbPathObj));

  // try to open a non-existing file
  db = SqliteDatabase::get<SampleDB>(dbFileName, false);
  ASSERT_TRUE(db == nullptr);

  // invalid / empty filename
  dbFileName = "";
  db = SqliteDatabase::get<SampleDB>(dbFileName, false);
  ASSERT_TRUE(db == nullptr);
}

//----------------------------------------------------------------

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

