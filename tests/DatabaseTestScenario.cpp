/*
 * Copyright © 2014 Volker Knollmann
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file or visit
 * http://www.wtfpl.net/ for more details.
 * 
 * This program comes without any warranty. Use it at your own risk or
 * don't use it at all.
 */

#include <filesystem>

#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"

namespace fs = std::filesystem;

const string DatabaseTestScenario::DB_TEST_FILE_NAME{"SqliteTestDB.db"};

RawSqlitePtr DatabaseTestScenario::getRawDbHandle() const
{
  sqlite3* tmpPtr = nullptr;
  int err = sqlite3_open(getSqliteFileName().c_str(), &tmpPtr);

  // cannot use gTest's assert-function here, because they only
  // work if the function returns void
  assert(err == SQLITE_OK);
  assert(tmpPtr != nullptr);
  assert(sqliteFileExists());

  return RawSqlitePtr{tmpPtr, &closeRawSqliteDb};
}

//----------------------------------------------------------------------------

RawSqlitePtr DatabaseTestScenario::getRawMemDb() const
{
  sqlite3* tmpPtr{nullptr};
  int err = sqlite3_open(":memory:", &tmpPtr);

  assert(err == SQLITE_OK);
  assert(tmpPtr != nullptr);

  return RawSqlitePtr{tmpPtr, &closeRawSqliteDb};
}

//----------------------------------------------------------------------------

RawSqliteStmt DatabaseTestScenario::prepStatement(const RawSqlitePtr& db, const string& sql)
{
  sqlite3_stmt* stmt = nullptr;
  int err = sqlite3_prepare_v2(db.get(), sql.c_str(), -1, &stmt, nullptr);
  assert(err == SQLITE_OK);
  assert(stmt != nullptr);

  return RawSqliteStmt{stmt, &closeRawSqliteStmt};
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::TearDown()
{
  BasicTestFixture::TearDown();

  // delete the test database, if still existing
  string dbFileName = getSqliteFileName();
  fs::path dbPathObj(dbFileName);
  if (fs::exists(dbPathObj))
  {
    ASSERT_TRUE(fs::remove(dbPathObj));
  }
  ASSERT_FALSE(fs::exists(dbPathObj));

}

//----------------------------------------------------------------------------

string DatabaseTestScenario::getSqliteFileName() const
{
  return genTestFilePath(DB_TEST_FILE_NAME);
}

//----------------------------------------------------------------------------
    
bool DatabaseTestScenario::sqliteFileExists() const
{
  fs::path dbPathObj(getSqliteFileName());

  return fs::exists(dbPathObj);
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::prepScenario01()
{
  ASSERT_FALSE(sqliteFileExists());
  RawSqlitePtr db = getRawDbHandle();

  string nowStr = "date('now')";
  string viewStr = "CREATE VIEW IF NOT EXISTS";

  auto execStmt = [&](string _sql) {
    RawSqliteStmt stmt = prepStatement(db, _sql);
    ASSERT_EQ(SQLITE_DONE, sqlite3_step(stmt.get()));

    // no need to call finalize() here, this will be done
    // by the deleter of RawSqliteStmt
  };

  string sql = "CREATE TABLE IF NOT EXISTS t1 (";
  sql += " i INT, f DOUBLE, s VARCHAR(40), d DATETIME)";
  execStmt(sql);

  sql = "CREATE TABLE IF NOT EXISTS t2 (";
  sql += " i INT, f DOUBLE, s VARCHAR(40), d DATETIME)";
  execStmt(sql);

  execStmt("INSERT INTO t1 VALUES (42, 23.23, 'Hallo', " + nowStr + ")");
  execStmt("INSERT INTO t1 VALUES (NULL, 666.66, 'Hi', " + nowStr + ")");
  sql = "INSERT INTO t1 VALUES (84, NULL, '";
  sql += u8"äöüÄÖÜ";
  sql += "', " + nowStr + ")";
  execStmt(sql);
  execStmt("INSERT INTO t1 VALUES (84, NULL, 'Ho', " + nowStr + ")");
  execStmt("INSERT INTO t1 VALUES (84, 42.42, 'Ho', " + nowStr + ")");

  execStmt(viewStr + " v1 AS SELECT i, f, s FROM t1 WHERE i=84");

  // no need to call sqlite3_close() here because it will be called
  // by the deleter of RawSqlitePtr

}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

SampleDB DatabaseTestScenario::getScenario01()
{
  prepScenario01();

  return SampleDB(getSqliteFileName(), SqliteOverlay::OpenMode::OpenExisting_RW);
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::SetUp()
{
  BasicTestFixture::SetUp();

  // delete the test database, if still existing,
  // so that we have a clean start
  string dbFileName = getSqliteFileName();
  fs::path dbPathObj(dbFileName);
  if (fs::exists(dbPathObj))
  {
    ASSERT_TRUE(fs::remove(dbPathObj));
  }
  ASSERT_FALSE(fs::exists(dbPathObj));

}

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

void closeRawSqliteDb(sqlite3* ptr)
{
  if (ptr != nullptr) sqlite3_close(ptr);
}

void closeRawSqliteStmt(sqlite3_stmt* ptr)
{
  if (ptr != nullptr) sqlite3_finalize(ptr);
}
