#include <gtest/gtest.h>

#include <Sloppy/Timer.h>
#include <Sloppy/Crypto/Crypto.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "SqliteDatabase.h"
#include "TabRow.h"
#include "DbTab.h"

using namespace SqliteOverlay;

struct ModExpectationData
{
  int modType;
  string dbName;
  string tabName;
  int64_t id;
  bool checkPerformed;
};

void dataChangeCallbackFunc(void* customPtr, int modType, char const * _dbName, char const* _tabName, sqlite3_int64 id)
{
  string dbName{_dbName};
  string tabName{_tabName};

  // default action: print a message
  string msg = "INSERT";
  if (modType == SQLITE_UPDATE) msg = "UPDATE";
  if (modType == SQLITE_DELETE) msg = "DELETE";
  msg += " on " + dbName + "." + tabName + ", row = " + to_string(id);
  cout << msg << endl;

  // extended action:
  // if we have a ModExpectationData pointer, compare
  // actual with expected results
  if (customPtr != nullptr)
  {
    ModExpectationData* med = (ModExpectationData *) customPtr;

    ASSERT_EQ(med->modType, modType);
    ASSERT_EQ(med->dbName, dbName);
    ASSERT_EQ(med->tabName, tabName);
    ASSERT_EQ(med->id, id);

    med->checkPerformed = true;
  }
}

TEST_F(DatabaseTestScenario, DataChangeCallback)
{
  auto db = getScenario01();

  // create a (local) ModExpectationData struct
  // for transmitting the expected callback parameters
  // to the callback function
  ModExpectationData med;

  // set the CALLBACK
  db.setDataChangeNotificationCallback(dataChangeCallbackFunc, &med);

  // get t1
  DbTab t1{db, "t1", false};

  // test one:
  // expect an INSERT in t1
  med.modType = SQLITE_INSERT;
  med.dbName = "main";
  med.tabName = "t1";
  med.id = 6;  // 5 rows already exist, the new one will be 6
  med.checkPerformed = false;
  ColumnValueClause cvc;
  cvc.addCol("i", 9999);
  int row = t1.insertRow(cvc);
  ASSERT_TRUE(row > 0);
  ASSERT_TRUE(med.checkPerformed);

  // test two:
  // expect a DELETE in t1
  med.modType = SQLITE_DELETE;
  med.id = 2;
  med.checkPerformed = false;
  ASSERT_TRUE(t1.deleteRowsByColumnValue("rowid", 2));
  ASSERT_TRUE(med.checkPerformed);

  // test three:
  // expect an UPDATE in t1
  med.modType = SQLITE_UPDATE;
  med.id = 3;
  med.checkPerformed = false;
  TabRow r = t1[3];
  r.update("i", 0);
  ASSERT_TRUE(med.checkPerformed);

  // disable checks and do a bulk delete
  // just to see in the output log that
  // a bulk of messages (== calls) appears
  void* oldPtr = db.setDataChangeNotificationCallback(dataChangeCallbackFunc, nullptr);
  ASSERT_EQ(oldPtr, &med);
  WhereClause w;
  w.addCol("rowid", ">", 0);
  int cnt = t1.deleteRowsByWhereClause(w);
  ASSERT_TRUE(cnt > 0);
  ASSERT_EQ(0, t1.length());

}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, ChangeLog)
{
  auto db = getScenario01();

  // we start with an empty log
  ASSERT_EQ(0, db.getChangeLogLength());

  // get t1
  DbTab t1{db, "t1", false};

  // enable logging
  db.enableChangeLog(false);

  // test one:
  // trigger an INSERT in t1
  ColumnValueClause cvc;
  cvc.addCol("i", 9999);
  int row = t1.insertRow(cvc);
  ASSERT_TRUE(row > 0);

  ASSERT_EQ(1, db.getChangeLogLength());
  ChangeLogList l = db.getAllChangesAndClearQueue();
  ASSERT_EQ(0, db.getChangeLogLength());
  ASSERT_EQ(1, l.size());
  ChangeLogEntry e = l.front();
  ASSERT_EQ(RowChangeAction::Insert, e.action);
  ASSERT_EQ(row, e.rowId);
  ASSERT_EQ("t1", e.tabName);
  ASSERT_TRUE(e.dbName.empty());

  // test two and three
  // DELETE and UPDATE
  ASSERT_TRUE(t1.deleteRowsByColumnValue("rowid", 2));
  ASSERT_EQ(1, db.getChangeLogLength());
  TabRow r = t1[3];
  r.update("i", 0);
  ASSERT_EQ(2, db.getChangeLogLength());
  l = db.getAllChangesAndClearQueue();
  ASSERT_EQ(0, db.getChangeLogLength());
  ASSERT_EQ(2, l.size());

  e = l.front();
  ASSERT_EQ(RowChangeAction::Delete, e.action);
  ASSERT_EQ(2, e.rowId);
  ASSERT_EQ("t1", e.tabName);
  ASSERT_TRUE(e.dbName.empty());

  l.erase(l.begin());
  e = l.front();
  ASSERT_EQ(RowChangeAction::Update, e.action);
  ASSERT_EQ(3, e.rowId);
  ASSERT_EQ("t1", e.tabName);
  ASSERT_TRUE(e.dbName.empty());

  // disable logging
  db.disableChangeLog(true);
  ASSERT_EQ(0, db.getChangeLogLength());
  row = t1.insertRow();
  ASSERT_TRUE(row > 0);
  ASSERT_EQ(0, db.getChangeLogLength());
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, ChangeLogSpeedImpact)
{
  auto db = getScenario01();

  // we start with an empty log
  ASSERT_EQ(0, db.getChangeLogLength());

  // get t1
  DbTab t1{db, "t1", false};

  // insert 1000 random strings without logging
  constexpr int nIter = 100000;
  constexpr int sLen = 100;
  Sloppy::Timer t;
  for (int i=0; i < nIter; ++i)
  {
    ColumnValueClause cvc;
    cvc.addCol("s", Sloppy::Crypto::getRandomAlphanumString(sLen));
    int row = t1.insertRow(cvc);
    ASSERT_TRUE(row > 0);
  }
  t.stop();
  int elapsedTime1 = t.getTime__us();
  cerr << nIter << " random strings without logging: " << elapsedTime1 << " µs" << endl;

  db.enableChangeLog(true);
  t.restart();
  for (int i=0; i < nIter; ++i)
  {
    ColumnValueClause cvc;
    cvc.addCol("s", Sloppy::Crypto::getRandomAlphanumString(sLen));
    int row = t1.insertRow(cvc);
    ASSERT_TRUE(row > 0);
  }
  t.stop();
  int elapsedTime2 = t.getTime__us();
  cerr << nIter << " random strings WITH logging: " << elapsedTime2 << " µs" << endl;
  cerr << "Ratio: " << (elapsedTime2 * 1.0)/elapsedTime1 << endl;
}

