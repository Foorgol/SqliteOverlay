#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "HelperFunc.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, DbTab_Insert)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  // insert default values
  int oldLen = t1->length();
  int newId = t1->insertRow();
  ASSERT_EQ(oldLen+1, newId);
  ASSERT_EQ(oldLen+1, t1->length());

  ++oldLen;

  // insert specific values
  ColumnValueClause cvc;
  cvc.addIntCol("i", 1234);
  cvc.addDoubleCol("f", 99.88);
  cvc.addNullCol("s");
  newId = t1->insertRow(cvc);
  ASSERT_EQ(oldLen+1, newId);
  ASSERT_EQ(oldLen+1, t1->length());
  string sql = "SELECT i FROM t1 WHERE id=" + to_string(newId);
  int iResult;
  ASSERT_TRUE(db->execScalarQueryInt(sql, &iResult));
  ASSERT_EQ(1234, iResult);
  sql = "SELECT f FROM t1 WHERE id=" + to_string(newId);
  double dResult;
  ASSERT_TRUE(db->execScalarQueryDouble(sql, &dResult));
  ASSERT_EQ(99.88, dResult);
  sql = "SELECT s FROM t1 WHERE id=" + to_string(newId);
  auto sqr = db->execScalarQueryString(sql);
  ASSERT_TRUE(sqr->isNull());

  // insert nonsense values
  ++oldLen;
  cvc.clear();
  cvc.addIntCol("sdkjfsfd", 88);
  ASSERT_EQ(-1, t1->insertRow(cvc));
  ASSERT_EQ(oldLen, t1->length());
}

//----------------------------------------------------------------


TEST_F(DatabaseTestScenario, DbTab_DeleteByWhere)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  // define where clause
  WhereClause w;
  w.addIntCol("id", ">", 2);

  // exec delete
  int oldLen = t1->length();
  int delCnt = t1->deleteRowsByWhereClause(w);
  ASSERT_EQ(3, delCnt);
  ASSERT_EQ(2, t1->length());

  // try where clause that matches no rows
  w.clear();
  w.addIntCol("id", ">", 2222);
  delCnt = t1->deleteRowsByWhereClause(w);
  ASSERT_EQ(0, delCnt);
  ASSERT_EQ(2, t1->length());

  // delete by nonsense values
  w.clear();
  w.addIntCol("sdkjfsfd", 88);
  ASSERT_EQ(-1, t1->deleteRowsByWhereClause(w));
  ASSERT_EQ(2, t1->length());
}

//----------------------------------------------------------------

