#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"
#include "TabRow.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, DbTab_Insert)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  // insert default values
  int oldLen = t1->length();
  int newId = t1->insertRow(nullptr);
  ASSERT_EQ(oldLen+1, newId);
  ASSERT_EQ(oldLen+1, t1->length());

  ++oldLen;

  // insert specific values
  ColumnValueClause cvc;
  cvc.addIntCol("i", 1234);
  cvc.addDoubleCol("f", 99.88);
  cvc.addNullCol("s");
  newId = t1->insertRow(cvc, nullptr);
  ASSERT_EQ(oldLen+1, newId);
  ASSERT_EQ(oldLen+1, t1->length());
  string sql = "SELECT i FROM t1 WHERE id=" + to_string(newId);
  int iResult;
  ASSERT_TRUE(db->execScalarQueryInt(sql, &iResult, nullptr));
  ASSERT_EQ(1234, iResult);
  sql = "SELECT f FROM t1 WHERE id=" + to_string(newId);
  double dResult;
  ASSERT_TRUE(db->execScalarQueryDouble(sql, &dResult, nullptr));
  ASSERT_EQ(99.88, dResult);
  sql = "SELECT s FROM t1 WHERE id=" + to_string(newId);
  auto sqr = db->execScalarQueryString(sql, nullptr);
  ASSERT_TRUE(sqr->isNull());

  // insert nonsense values
  ++oldLen;
  cvc.clear();
  cvc.addIntCol("sdkjfsfd", 88);
  ASSERT_EQ(-1, t1->insertRow(cvc, nullptr));
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
  int delCnt = t1->deleteRowsByWhereClause(w, nullptr);
  ASSERT_EQ(3, delCnt);
  ASSERT_EQ(2, t1->length());

  // try where clause that matches no rows
  w.clear();
  w.addIntCol("id", ">", 2222);
  delCnt = t1->deleteRowsByWhereClause(w, nullptr);
  ASSERT_EQ(0, delCnt);
  ASSERT_EQ(2, t1->length());

  // delete by nonsense values
  w.clear();
  w.addIntCol("sdkjfsfd", 88);
  ASSERT_EQ(-1, t1->deleteRowsByWhereClause(w, nullptr));
  ASSERT_EQ(2, t1->length());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_GetTabRow)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);
  WhereClause w;
  w.addIntCol("id", 4);

  // operator []
  TabRow r = (*t1)[2];
  ASSERT_EQ(2, r.getId());
  r = (*t1)[w];
  ASSERT_EQ(4, r.getId());

  // by where clause
  r = t1->getSingleRowByWhereClause(w);
  ASSERT_EQ(4, r.getId());

  // by column value
  r = t1->getSingleRowByColumnValue("i", 84);
  ASSERT_EQ(3, r.getId());
  r = t1->getSingleRowByColumnValue("f", 23.23);
  ASSERT_EQ(1, r.getId());
  r = t1->getSingleRowByColumnValue("s", "Hi");
  ASSERT_EQ(2, r.getId());
  r = t1->getSingleRowByColumnValueNull("f");
  ASSERT_EQ(3, r.getId());

  // no test for illegal arguments here
  // they are covered by the constructor tests for TabRow,
  // because getSingleRowByXXX is mostly a wrapper for the
  // TabRow ctor
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_HasRow)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  ASSERT_TRUE(t1->hasRowId(1));
  ASSERT_FALSE(t1->hasRowId(100));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_GetAllRows)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  DbTab::CachingRowIterator it = t1->getAllRows();
  ASSERT_EQ(5, it.length());
  ASSERT_FALSE(it.isEmpty());

  int id=0;
  while (!(it.isEnd()))
  {
    ++id;
    TabRow r = *it;
    ASSERT_EQ(id, r.getId());
    ++it;
  }

  // test an empty table
  auto t2 = db->getTab("t2");
  it = t2->getAllRows();
  ASSERT_EQ(0, it.length());
  ASSERT_TRUE(it.isEmpty());
  ASSERT_THROW(*it,  std::runtime_error);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_GetRowsByWhereClause)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  WhereClause w;
  w.addIntCol("id", ">", 3);
  DbTab::CachingRowIterator it = t1->getRowsByWhereClause(w);
  ASSERT_EQ(2, it.length());
  ASSERT_FALSE(it.isEmpty());

  int id=3;
  while (!(it.isEnd()))
  {
    ++id;
    TabRow r = *it;
    ASSERT_EQ(id, r.getId());
    ++it;
  }

  // test empty result
  w.clear();
  w.addIntCol("id", ">", 333);
  it = t1->getRowsByWhereClause(w);
  ASSERT_EQ(0, it.length());
  ASSERT_TRUE(it.isEmpty());
  ASSERT_TRUE(it.isEnd());

  // test invalid query
  w.clear();
  w.addIntCol("kgjhdfkjg", 42);
  ASSERT_THROW(t1->getRowsByWhereClause(w), std::invalid_argument);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_AddColumn)
{
  auto db = getScenario01();
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  // try to create an existing column
  ASSERT_FALSE(t1->addColumn_int("i", false, CONFLICT_CLAUSE::__NOT_SET, false, 0, true));

  // create a new column without constraints
  ASSERT_TRUE(t1->addColumn_int("i2", false, CONFLICT_CLAUSE::__NOT_SET, false, 0, true));

  // create a new column with invalid constraints ("not null" but without default)
  ASSERT_FALSE(t1->addColumn_int("i3", true, CONFLICT_CLAUSE::__NOT_SET, false, 0, true));

  // create a new column with valid constraints
  ASSERT_TRUE(t1->addColumn_int("i3", true, CONFLICT_CLAUSE::__NOT_SET, true, 42, true));
  TabRow r = (*t1)[1];
  ASSERT_EQ(42, r.getInt("i3"));

  // create a new column with a foreign key
  ASSERT_TRUE(t1->addColumn_foreignKey("k1", "t2", CONSISTENCY_ACTION::CASCADE, CONSISTENCY_ACTION::__NOT_SET));
}
