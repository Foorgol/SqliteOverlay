#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "SqliteDatabase.h"
#include "TabRow.h"

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
  db->setDataChangeNotificationCallback(dataChangeCallbackFunc, &med);

  // get a pointer to t1
  DbTab* t1 = db->getTab("t1");

  // test one:
  // expect an INSERT in t1
  med.modType = SQLITE_INSERT;
  med.dbName = "main";
  med.tabName = "t1";
  med.id = 6;  // 5 rows already exist, the new one will be 6
  med.checkPerformed = false;
  ColumnValueClause cvc;
  cvc.addIntCol("i", 9999);
  int err;
  int row = t1->insertRow(&err);
  ASSERT_TRUE(row > 0);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(med.checkPerformed);

  // test two:
  // expect a DELETE in t1
  med.modType = SQLITE_DELETE;
  med.id = 2;
  med.checkPerformed = false;
  ASSERT_TRUE(t1->deleteRowsByColumnValue("id", 2, &err));
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(med.checkPerformed);

  // test three:
  // expect an UPDATE in t1
  med.modType = SQLITE_UPDATE;
  med.id = 3;
  med.checkPerformed = false;
  TabRow r = (*t1)[3];
  r.update("i", 0, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(med.checkPerformed);

  // disable checks and do a bulk delete
  // just to see in the output log that
  // a bulk of messages (== calls) appears
  void* oldPtr = db->setDataChangeNotificationCallback(dataChangeCallbackFunc, nullptr);
  ASSERT_EQ(oldPtr, &med);
  WhereClause w;
  w.addIntCol("id", ">", 0);
  int cnt = t1->deleteRowsByWhereClause(w, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(cnt > 0);
  ASSERT_EQ(0, t1->length());

}
