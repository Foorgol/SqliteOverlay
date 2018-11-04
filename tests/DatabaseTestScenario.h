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

#ifndef DATABASETESTSCENARIO_H
#define	DATABASETESTSCENARIO_H

#include <sqlite3.h>
#include <memory>
#include <iostream>

#include "BasicTestClass.h"
#include "SqliteDatabase.h"
#include "SampleDB.h"

using namespace std;

void closeRawSqliteDb(sqlite3* ptr);

void closeRawSqliteStmt(sqlite3_stmt* ptr);

using RawSqlitePtr = std::unique_ptr<sqlite3, decltype (&closeRawSqliteDb)>;
using RawSqliteStmt = std::unique_ptr<sqlite3_stmt, decltype (&closeRawSqliteStmt)>;


class DatabaseTestScenario : public BasicTestFixture
{
public:

protected:
  static const string DB_TEST_FILE_NAME;

  RawSqlitePtr getRawDbHandle() const;
  RawSqlitePtr getRawMemDb() const;
  RawSqliteStmt prepStatement(const RawSqlitePtr& db, const string& sql);

  void prepScenario01();
  SampleDB getScenario01();
  
  void SetUp () override;
  void TearDown () override;
  string getSqliteFileName() const;
  bool sqliteFileExists() const;
};

#endif	/* DATABASETESTSCENARIO_H */

