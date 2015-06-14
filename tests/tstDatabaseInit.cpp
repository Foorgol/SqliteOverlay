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

