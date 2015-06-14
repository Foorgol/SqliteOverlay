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

#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
//#include "DbTab.h"

namespace bfs = boost::filesystem;

constexpr char DatabaseTestScenario::SQLITE_DB[];

upSqlite3Db DatabaseTestScenario::getRawDbHandle() const
{
  sqlite3* tmpPtr = nullptr;
  int err = sqlite3_open(getSqliteFileName().c_str(), &tmpPtr);

  // cannot use gTest's assert-function here, because they only
  // work if the function returns void
  assert(err == SQLITE_OK);
  assert(tmpPtr != nullptr);
  assert(sqliteFileExists());

  return upSqlite3Db(tmpPtr);
}

//----------------------------------------------------------------------------

sqlite3_stmt* DatabaseTestScenario::prepStatement(upSqlite3Db& db, const string& sql)
{
  sqlite3_stmt* stmt = nullptr;
  int err = sqlite3_prepare_v2(db.get(), sql.c_str(), -1, &stmt, nullptr);
  assert(err == SQLITE_OK);
  assert(stmt != nullptr);

  return stmt;
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::TearDown()
{
  BasicTestFixture::TearDown();

  // delete the test database, if still existing
  string dbFileName = getSqliteFileName();
  bfs::path dbPathObj(dbFileName);
  if (bfs::exists(dbPathObj))
  {
    ASSERT_TRUE(bfs::remove(dbPathObj));
  }
  ASSERT_FALSE(bfs::exists(dbPathObj));

  //DbTab::clearTabCache();
  
  //qDebug() << "----------- DatabaseTestScenario tearDown() finished! -----------";
}

//----------------------------------------------------------------------------

string DatabaseTestScenario::getSqliteFileName() const
{
  return genTestFilePath(SQLITE_DB);
}

//----------------------------------------------------------------------------
    
bool DatabaseTestScenario::sqliteFileExists() const
{
  bfs::path dbPathObj(getSqliteFileName());

  return bfs::exists(dbPathObj);
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::prepScenario01()
{
  ASSERT_FALSE(sqliteFileExists());
  upSqlite3Db db = getRawDbHandle();

  string aiStr = "AUTOINCREMENT";
  string nowStr = "date('now')";
  string viewStr = "CREATE VIEW IF NOT EXISTS";

  auto execStmt = [&](string _sql) {
    sqlite3_stmt* stmt = prepStatement(db, _sql);
    ASSERT_EQ(SQLITE_DONE, sqlite3_step(stmt));
    ASSERT_EQ(SQLITE_OK, sqlite3_finalize(stmt));
  };

  string sql = "CREATE TABLE IF NOT EXISTS t1 (id INTEGER NOT NULL PRIMARY KEY " + aiStr + ",";
  sql += " i INT, f DOUBLE, s VARCHAR(40), d DATETIME)";
  execStmt(sql);

  sql = "CREATE TABLE IF NOT EXISTS t2 (id INTEGER NOT NULL PRIMARY KEY " + aiStr + ",";
  sql += " i INT, f DOUBLE, s VARCHAR(40), d DATETIME)";
  execStmt(sql);

  execStmt("INSERT INTO t1 VALUES (NULL, 42, 23.23, 'Hallo', " + nowStr + ")");
  execStmt("INSERT INTO t1 VALUES (NULL, NULL, 666.66, 'Hi', " + nowStr + ")");
  execStmt("INSERT INTO t1 VALUES (NULL, 84, NULL, 'Ho', " + nowStr + ")");
  execStmt("INSERT INTO t1 VALUES (NULL, 84, NULL, 'Hoi', " + nowStr + ")");
  execStmt("INSERT INTO t1 VALUES (NULL, 84, 42.42, 'Ho', " + nowStr + ")");

  execStmt(viewStr + " v1 AS SELECT i, f, s FROM t1 WHERE i=84");

  ASSERT_EQ(SQLITE_OK, sqlite3_close(db.get()));

}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

upSqliteDatabase DatabaseTestScenario::getScenario01()
{
  prepScenario01();

  SampleDB* tmpPtr = new SampleDB(getSqliteFileName(), false);
  tmpPtr->populateTables();
  tmpPtr->populateViews();

  return upSqliteDatabase(tmpPtr);
}

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    
