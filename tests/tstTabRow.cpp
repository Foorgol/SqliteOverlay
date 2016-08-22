#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"
#include "TabRow.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, TabRow_Ctor)
{
  auto db = getScenario01();

  // construct by row id
  TabRow r1(db.get(), "t1", 3);
  ASSERT_EQ(3, r1.getId());

  // construct by where clause
  WhereClause w;
  w.addIntCol("i", 84);
  TabRow r2(db.get(), "t1", w);
  ASSERT_EQ(3, r2.getId());

  // invalid row requests
  ASSERT_THROW(TabRow(db.get(), "t1", 999), std::invalid_argument);
  ASSERT_THROW(TabRow(db.get(), "", 1), std::invalid_argument);
  ASSERT_THROW(TabRow(db.get(), "ldflgdlg", 1), std::invalid_argument);
  ASSERT_THROW(TabRow(db.get(), "ldflgdlg", 1111), std::invalid_argument);
  w.clear();
  w.addIntCol("i", 999);
  ASSERT_THROW(TabRow(db.get(), "t1", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db.get(), "", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db.get(), "sdlkfslf", w), std::invalid_argument);
  w.clear();
  w.addIntCol("sdlfjksdf", 999);
  ASSERT_THROW(TabRow(db.get(), "t1", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db.get(), "", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db.get(), "sdlkfslf", w), std::invalid_argument);

  // test operator==
  ASSERT_TRUE(r1 == r2);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_getters)
{
  auto db = getScenario01();
  TabRow r(db.get(), "t1", 1);
  ASSERT_EQ(1, r.getId());

  // get string
  ASSERT_EQ("Hallo", r["s"]);
  auto s = r.getString2("s");
  ASSERT_TRUE(s != nullptr);
  ASSERT_EQ("Hallo", s->get());

  // get int
  ASSERT_EQ(42, r.getInt("i"));
  auto i = r.getInt2("i");
  ASSERT_TRUE(i != nullptr);
  ASSERT_EQ(42, i->get());

  // get double
  ASSERT_EQ(23.23, r.getDouble("f"));
  auto d = r.getDouble2("f");
  ASSERT_TRUE(d != nullptr);
  ASSERT_EQ(23.23, d->get());

  // test invalid column name
  ASSERT_THROW(r.getInt("skjfh"), std::invalid_argument);
  ASSERT_THROW(r.getDouble("skjfh"), std::invalid_argument);
  ASSERT_THROW(r["skjfh"], std::invalid_argument);
  ASSERT_EQ(nullptr, r.getInt2("skjfh"));
  ASSERT_EQ(nullptr, r.getString2("skjfh"));
  ASSERT_EQ(nullptr, r.getDouble2("skjfh"));

  // test date functions
  //
  // before the test, modify the int value in row 1 to get a suitable date value
  r.update("i", 20160807);
  boost::gregorian::date dExpected{2016, 8, 7};
  ASSERT_EQ(dExpected, r.getDate("i"));
  auto dt = r.getDate2("i");
  ASSERT_TRUE(dt != nullptr);
  ASSERT_EQ(dExpected, dt->get());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_Update)
{
  auto db = getScenario01();
  TabRow r(db.get(), "t1", 1);
  ASSERT_EQ(1, r.getId());

  // simple updates of one colum
  ASSERT_EQ(42, r.getInt("i"));
  ASSERT_TRUE(r.update("i", 88, nullptr));
  ASSERT_EQ(88, r.getInt("i"));
  ASSERT_EQ(23.23, r.getDouble("f"));
  ASSERT_TRUE(r.update("f", 12.34, nullptr));
  ASSERT_EQ(12.34, r.getDouble("f"));
  ASSERT_EQ("Hallo", r["s"]);
  ASSERT_TRUE(r.update("s", "xyz", nullptr));
  ASSERT_EQ("xyz", r["s"]);

  // update multiple columns at once
  ColumnValueClause cvc;
  cvc.addIntCol("i", 55);
  cvc.addNullCol("f");
  cvc.addStringCol("s", "xxx");
  ASSERT_TRUE(r.update(cvc, nullptr));
  ASSERT_EQ(55, r.getInt("i"));
  ASSERT_EQ("xxx", r["s"]);
  auto d = r.getDouble2("f");
  ASSERT_TRUE(d != nullptr);
  ASSERT_TRUE(d->isNull());

  // invalid column names
  ASSERT_FALSE(r.update("xycyxcyx", 88, nullptr));
  ASSERT_FALSE(r.update("yxcyc", 12.34, nullptr));
  ASSERT_FALSE(r.update("ycyxcm", "xyz", nullptr));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_OperatorEqual)
{
  auto db = getScenario01();
  TabRow r1(db.get(), "t1", 1);
  TabRow r2(db.get(), "t1", 1);
  ASSERT_EQ(1, r1.getId());
  ASSERT_EQ(1, r2.getId());

  DbTab* t2 = db->getTab("t2");
  ASSERT_EQ(1, t2->insertRow(nullptr));
  TabRow r3(db.get(), "t2", 1);

  ASSERT_TRUE(r1 == r2); // same tab, same ID
  ASSERT_TRUE(r3 == r3); // same row
  ASSERT_FALSE(r1 == r3); // different tabs, same ID
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_Erase)
{
  auto db = getScenario01();
  TabRow r(db.get(), "t1", 1);
  ASSERT_EQ(1, r.getId());
  auto t1 = db->getTab("t1");
  ASSERT_TRUE(t1 != nullptr);

  int oldLen = t1->length();
  ASSERT_TRUE(r.erase(nullptr));
  ASSERT_EQ(oldLen-1, t1->length());
  ASSERT_EQ(-1, r.getId());

  // make sure the old row doesn't exists anymore
  ASSERT_THROW(TabRow(db.get(), "t1", 1), std::invalid_argument);
}

//----------------------------------------------------------------
