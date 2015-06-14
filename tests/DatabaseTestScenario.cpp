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

#include "DatabaseTestScenario.h"
#include "DbTab.h"

#include <QtSql/QSqlQuery>
#include <QFile>
#include <QtSql/QSqlError>

QSqlDatabase DatabaseTestScenario::getDbConn(dbOverlay::GenericDatabase::DB_ENGINE t)
{
  if (QSqlDatabase::contains(DB_CONNECTION_NAME))
  {
    QSqlDatabase db = QSqlDatabase::database(DB_CONNECTION_NAME, true);

    // despite providing the database name using setDatabaseName,
    // you must issue a USE SQL-command before actually accessing the database
    if (t == dbOverlay::GenericDatabase::MYSQL)
    {
      QSqlQuery qry(db);
      execQueryAndDumpError(qry, "USE " + MYSQL_DB);
    }
    
    return db;

  }

  QSqlDatabase db;
  
  if (t == dbOverlay::GenericDatabase::SQLITE)
  {
    db = QSqlDatabase::addDatabase("QSQLITE", DB_CONNECTION_NAME);

    db.setDatabaseName(getSqliteFileName());
    
  } else {
    
    db = QSqlDatabase::addDatabase("QMYSQL", DB_CONNECTION_NAME);
    db.setHostName(MYSQL_HOST);
    db.setDatabaseName(MYSQL_DB);
    db.setPort(MYSQL_PORT);
    db.setUserName(DB_USER);
    db.setPassword(DB_PASSWD);
  }
  
  if (!db.open())
  {
    QSqlError err = db.lastError();
    log.warn("db.open() for " + db.hostName() + " using driver " + db.driverName() + " failed with: " + err.text());
    CPPUNIT_ASSERT(false);
  } else {
    log.info("db.open() for " + db.hostName() + " using driver " + db.driverName() + " succeeded!");
  }
  
  // despite providing the database name using setDatabaseName,
  // you must issue a USE SQL-command before actually accessing the database
  if (t == dbOverlay::GenericDatabase::MYSQL)
  {
    QSqlQuery qry(db);
    execQueryAndDumpError(qry, "USE " + MYSQL_DB);
  }
  
  return db;
}

//----------------------------------------------------------------------------
    
QSqlDatabase DatabaseTestScenario::getDbConn()
{
  if (QSqlDatabase::contains(DB_CONNECTION_NAME))
  {
    QSqlDatabase db = QSqlDatabase::database(DB_CONNECTION_NAME, true);

    // despite providing the database name using setDatabaseName,
    // you must issue a USE SQL-command before actually accessing the database
    if (db.driverName() == "QMYSQL")
    {
      QSqlQuery qry(db);
      execQueryAndDumpError(qry, "USE " + MYSQL_DB);
    }
    
    return db;

  }
  
  return getDbConn(dbOverlay::GenericDatabase::SQLITE);

}

//----------------------------------------------------------------------------

void DatabaseTestScenario::cleanupMysql()
{
  QSqlDatabase db = getDbConn(dbOverlay::GenericDatabase::MYSQL);
  
  db.exec("DROP DATABASE " + MYSQL_DB);
  db.exec("CREATE DATABASE " + MYSQL_DB);
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::removeDbConn()
{
  if (QSqlDatabase::contains(DB_CONNECTION_NAME))
  {
    QSqlDatabase::removeDatabase(DB_CONNECTION_NAME);
    CPPUNIT_ASSERT(!QSqlDatabase::contains(DB_CONNECTION_NAME));
    qDebug() << "Removed database " + DB_CONNECTION_NAME + " from connection pool";
  } else {
    qDebug() << "Weird: could not remove database " + DB_CONNECTION_NAME + " from connection pool, because it's not existing";
  }
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::tearDown()
{
  BasicTestClass::tearDown();
  removeDbConn();
  DbTab::clearTabCache();
  
  qDebug() << "----------- DatabaseTestScenario tearDown() finished! -----------";
}

//----------------------------------------------------------------------------

QString DatabaseTestScenario::getSqliteFileName()
{
  return genTestFilePath(SQLITE_DB);
}

//----------------------------------------------------------------------------
    
bool DatabaseTestScenario::sqliteFileExists()
{
  QFile f(getSqliteFileName());
  return f.exists();
}

//----------------------------------------------------------------------------

void DatabaseTestScenario::prepScenario01(dbOverlay::GenericDatabase::DB_ENGINE t)
{
  QSqlDatabase db;
  
  if (t == dbOverlay::GenericDatabase::MYSQL)
  {
    cleanupMysql();
    db = getDbConn(t);
  } else {
    CPPUNIT_ASSERT(!sqliteFileExists());
    db = getDbConn(t);
    CPPUNIT_ASSERT(sqliteFileExists());
  }

  QString aiStr = (t == dbOverlay::GenericDatabase::MYSQL) ? "AUTO_INCREMENT" : "AUTOINCREMENT";
  QString nowStr = (t == dbOverlay::GenericDatabase::MYSQL) ? "NOW()" : "date('now')";
  QString viewStr = (t == dbOverlay::GenericDatabase::MYSQL) ? "CREATE OR REPLACE VIEW" : "CREATE VIEW IF NOT EXISTS";

  db.exec("CREATE TABLE IF NOT EXISTS t1 (id INTEGER NOT NULL PRIMARY KEY " + aiStr + "," +
          " i INT, f DOUBLE, s VARCHAR(40), d DATETIME)");
  db.exec("CREATE TABLE IF NOT EXISTS t2 (id INTEGER NOT NULL PRIMARY KEY " + aiStr + "," +
          " i INT, f DOUBLE, s VARCHAR(40), d DATETIME)");

  db.exec("INSERT INTO t1 VALUES (NULL, 42, 23.23, 'Hallo', " + nowStr + ")");
  db.exec("INSERT INTO t1 VALUES (NULL, NULL, 666.66, 'Hi', " + nowStr + ")");
  db.exec("INSERT INTO t1 VALUES (NULL, 84, NULL, 'Ho', " + nowStr + ")");
  db.exec("INSERT INTO t1 VALUES (NULL, 84, NULL, 'Hoi', " + nowStr + ")");
  db.exec("INSERT INTO t1 VALUES (NULL, 84, 42.42, 'Ho', " + nowStr + ")");

  db.exec(viewStr + " v1 AS SELECT i, f, s FROM t1 WHERE i=84");

}

//----------------------------------------------------------------------------

void DatabaseTestScenario::execQueryAndDumpError(QSqlQuery& qry, const QString& sqlStatement)
{
  bool ok;
  if (sqlStatement.length() != 0)
  {
    // simple statement
    ok = qry.exec(sqlStatement);
  } else {
    // prepared statement, prepared externally
    ok = qry.exec();
  }
  
  if (!ok)
  {
    QString msg = "The following SQL query failed: " + QString("\n");
    msg += "     " + qry.lastQuery() + QString("\n");
    msg += "     Error: " + qry.lastError().text()  + QString("\n");
    log.warn(msg);
    CPPUNIT_ASSERT(false);
  } else {
    
    int result;
    if (qry.isSelect())
    {
      result = qry.size();
    }
    else
    {
      result = qry.numRowsAffected();
    }
    
    QString msg = "The following SQL query was successfully executed: " + QString("\n");
    msg += "     " + qry.lastQuery() + QString("\n");
    msg += "     Rows affected: " + QString::number(result)  + QString("\n");
    log.info(msg);  

  }
}

//----------------------------------------------------------------------------

SampleDB DatabaseTestScenario::getScenario01(dbOverlay::GenericDatabase::DB_ENGINE t)
{
  prepScenario01(t);
  
  if (t == dbOverlay::GenericDatabase::MYSQL)
  {
    SampleDB db(t, "localhost", 3306, "unittest", "unittest", "unittest");
    db.populateTables();
    db.populateViews();
    return db;
  }

  SampleDB db(getSqliteFileName(), false);
  db.populateTables();
  db.populateViews();
  return db;
}

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    
