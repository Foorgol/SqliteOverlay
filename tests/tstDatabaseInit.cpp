#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "BasicTestClass.h"
#include "SqliteDatabase.h"

using namespace SqliteOverlay;
namespace bfs = boost::filesystem;

TEST_F(BasicTestFixture, DatabaseInit)
{
  // create a new, empty database
  string dbFileName = genTestFilePath("ksfdjhskf.db");
  bfs::path dbPathObj(dbFileName);

  ASSERT_FALSE(bfs::exists(dbPathObj));
  SqliteDatabase* db;
  db = new SqliteDatabase(dbFileName, true);
  ASSERT_TRUE(bfs::exists(dbPathObj));

  // close the database connection
  delete db;
  db = nullptr;

  // open an existing database
  db = new SqliteDatabase(dbFileName, false);
  ASSERT_TRUE(db != nullptr);
  delete db;

  // clean-up
  ASSERT_TRUE(bfs::remove(dbPathObj));

  // try to open a non-existing file
  ASSERT_THROW(new SqliteDatabase(dbFileName, false), invalid_argument);

  // invalid / empty filename
  dbFileName = "";
  ASSERT_THROW(new SqliteDatabase(dbFileName, false), invalid_argument);
}

