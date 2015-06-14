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

#include "BasicTestClass.h"
#include "SqliteDatabase.h"
#include "SampleDB.h"

class DatabaseTestScenario : public BasicTestFixture
{
public:

protected:
  static constexpr char SQLITE_DB[] = "SqliteTestDB.db";

  upSqlite3Db getRawDbHandle() const;

  void prepScenario01();
  upSqliteDatabase getScenario01();
  
  //void execQueryAndDumpError(QSqlQuery& qry, const QString& sqlStatement="");

public:
  virtual void TearDown ();
  string getSqliteFileName() const;
  bool sqliteFileExists() const;
};

#endif	/* DATABASETESTSCENARIO_H */

