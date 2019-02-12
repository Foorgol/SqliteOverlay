#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "TabRow.h"
#include "CommonTabularClass.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, TabRow_Ctor)
{
  auto db = getScenario01();

  // construct by row id
  TabRow r1(db, "t1", 3);
  ASSERT_EQ(3, r1.id());

  // construct by where clause
  WhereClause w;
  w.addCol("i", 84);
  TabRow r2(db, "t1", w);
  ASSERT_EQ(3, r2.id());

  // invalid row requests
  ASSERT_THROW(TabRow(db, "t1", 999), std::invalid_argument);
  ASSERT_THROW(TabRow(db, "", 1), std::invalid_argument);
  ASSERT_THROW(TabRow(db, "ldflgdlg", 1), std::invalid_argument);
  ASSERT_THROW(TabRow(db, "ldflgdlg", 1111), std::invalid_argument);
  w.clear();
  w.addCol("i", 999);
  ASSERT_THROW(TabRow(db, "t1", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db, "", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db, "sdlkfslf", w), std::invalid_argument);
  w.clear();
  w.addCol("sdlfjksdf", 999);
  ASSERT_THROW(TabRow(db, "t1", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db, "", w), std::invalid_argument);
  ASSERT_THROW(TabRow(db, "sdlkfslf", w), std::invalid_argument);
  w.clear();
  ASSERT_THROW(TabRow(db, "t1", w), std::invalid_argument);

  // test operator==
  ASSERT_TRUE(r1 == r2);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_getters)
{
  auto db = getScenario01();
  TabRow r(db, "t1", 1);
  ASSERT_EQ(1, r.id());

  // get string
  ASSERT_EQ("Hallo", r["s"]);
  auto optString = r.getString2("s");
  ASSERT_TRUE(optString.has_value());
  ASSERT_EQ("Hallo", optString.value());

  // get int
  ASSERT_EQ(42, r.getInt("i"));
  auto i = r.getInt2("i");
  ASSERT_TRUE(i.has_value());
  ASSERT_EQ(42, i.value());

  // get double
  ASSERT_EQ(23.23, r.getDouble("f"));
  auto d = r.getDouble2("f");
  ASSERT_TRUE(d.has_value());
  ASSERT_EQ(23.23, d.value());

  // test invalid column name
  ASSERT_THROW(r.getInt("skjfh"), std::invalid_argument);
  ASSERT_THROW(r.getDouble("skjfh"), std::invalid_argument);
  ASSERT_THROW(r["skjfh"], std::invalid_argument);
  ASSERT_THROW(r.getInt2("skjfh"), std::invalid_argument);
  ASSERT_THROW(r.getDouble2("skjfh"), std::invalid_argument);
  ASSERT_THROW(r.getString2("skjfh"), std::invalid_argument);

  // test date functions
  //
  // before the test, modify the int value in row 1 to get a suitable date value
  r.update("i", 20160807);
  boost::gregorian::date dExpected{2016, 8, 7};
  ASSERT_EQ(dExpected, r.getDate("i"));
  auto dt = r.getDate2("i");
  ASSERT_TRUE(dt.has_value());
  ASSERT_EQ(dExpected, dt.value());

  // test NULL columns; if it works for one type, it
  // works for all types... thanks to templating
  r = TabRow(db, "t1", 2);
  ASSERT_EQ(2, r.id());
  i = r.getInt2("i");
  ASSERT_FALSE(i.has_value());
  ASSERT_THROW(r.getInt("i"), NullValueException);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_Update)
{
  auto db = getScenario01();
  TabRow r(db, "t1", 1);
  ASSERT_EQ(1, r.id());

  // simple updates of one colum
  ASSERT_EQ(42, r.getInt("i"));
  ASSERT_NO_THROW(r.update("i", 88));
  ASSERT_EQ(88, r.getInt("i"));
  ASSERT_EQ(23.23, r.getDouble("f"));
  ASSERT_NO_THROW(r.update("f", 12.34));
  ASSERT_EQ(12.34, r.getDouble("f"));
  ASSERT_EQ("Hallo", r["s"]);
  ASSERT_NO_THROW(r.update("s", "xyz"));
  ASSERT_EQ("xyz", r["s"]);

  // update multiple columns at once
  ColumnValueClause cvc;
  cvc.addCol("i", 55);
  cvc.addNullCol("f");
  cvc.addCol("s", "xxx");
  ASSERT_NO_THROW(r.update(cvc));
  ASSERT_EQ(55, r.getInt("i"));
  ASSERT_EQ("xxx", r["s"]);
  auto d = r.getDouble2("f");
  ASSERT_FALSE(d.has_value());
  ASSERT_THROW(r.getDouble("f"), NullValueException);

  // invalid column names
  ASSERT_THROW(r.update("xycyxcyx", 88), std::invalid_argument);
  ASSERT_THROW(r.update("", 22.22), std::invalid_argument);

  // update to NULL
  ASSERT_EQ(55, r.getInt("i"));
  r.updateToNull("i");
  ASSERT_FALSE(r.getInt2("i").has_value());
  ASSERT_THROW(r.getInt("i"), NullValueException);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_OperatorEqual)
{
  auto db1 = getScenario01();
  auto db2 = db1.duplicateConnection(true);

  TabRow db1_t1_r1(db1, "t1", 1);
  TabRow db2_t1_r1(db2, "t1", 1);

  TabRow db1_t1_r1a(db1, "t1", 1);

  db1.execNonQuery("INSERT INTO t2 DEFAULT VALUES");
  TabRow db1_t2_r1(db1, "t2", 1);

  ASSERT_TRUE(db1_t1_r1 == db1_t1_r1); // same connection, same tab, same row (indentity test)
  ASSERT_TRUE(db1_t1_r1 == db2_t1_r1); // different connections, same db, same tab, same row

  ASSERT_TRUE(db1_t1_r1 == db1_t1_r1a); // same connection, same tab, same row in different objects

  ASSERT_FALSE(db1_t2_r1 == db1_t1_r1); // same connection, different tabs, same ID
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_Erase)
{
  auto db = getScenario01();
  TabRow r(db, "t1", 1);
  ASSERT_EQ(1, r.id());

  CommonTabularClass ctc{db, "t1", false, false};
  int oldLen = ctc.length();
  ASSERT_NO_THROW(r.erase());
  ASSERT_EQ(oldLen-1, ctc.length());
  //ASSERT_EQ(-1, r.id());

  // make sure the old row doesn't exist anymore
  ASSERT_THROW(TabRow(db, "t1", 1), std::invalid_argument);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_CopyMove)
{
  auto db = getScenario01();
  TabRow r1(db, "t1", 1);

  // move operator
  TabRow r2 = std::move(r1);
  ASSERT_EQ(1, r2.id());
  ASSERT_EQ(-1, r1.id());
  ASSERT_EQ(42, r2.getInt("i"));
  ASSERT_FALSE(r1 == r2);

  // move ctor
  TabRow r3{std::move(r2)};
  ASSERT_EQ(1, r3.id());
  ASSERT_EQ(-1, r2.id());
  ASSERT_EQ(42, r3.getInt("i"));
  ASSERT_FALSE(r2 == r3);

  // copy operator
  r2 = r3;
  ASSERT_EQ(1, r2.id());
  ASSERT_EQ(42, r2.getInt("i"));
  ASSERT_EQ(1, r3.id());
  ASSERT_EQ(42, r3.getInt("i"));
  ASSERT_TRUE(r2 == r3);

  // copy ctor
  TabRow r4{r3};
  ASSERT_EQ(1, r4.id());
  ASSERT_EQ(42, r4.getInt("i"));
  ASSERT_EQ(1, r3.id());
  ASSERT_EQ(42, r3.getInt("i"));
  ASSERT_TRUE(r4 == r3);
  ASSERT_TRUE(r4 == r2);
  ASSERT_TRUE(r2 == r3);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_MultiGet)
{
  auto db = getScenario01();
  TabRow r(db, "t1", 1);

  int i{0};
  double f{0};
  string s;

  r.multiGetByRef("i", i, "f", f);
  ASSERT_EQ(42, i);
  ASSERT_EQ(23.23, f);

  i = 0;
  f = 0;
  r.multiGetByRef("i", i, "f", f, "s", s);
  ASSERT_EQ(42, i);
  ASSERT_EQ(23.23, f);
  ASSERT_EQ("Hallo", s);

  i = 0;
  f = 0;
  s.clear();
  int id;
  r.multiGetByRef("rowid", id, "i", i, "f", f, "s", s);
  ASSERT_EQ(1, id);
  ASSERT_EQ(42, i);
  ASSERT_EQ(23.23, f);
  ASSERT_EQ("Hallo", s);

  i = 0;
  f = 0;
  tie(i, f) = r.multiGetAsTuple<int, double>("i", "f");
  ASSERT_EQ(42, i);
  ASSERT_EQ(23.23, f);

  i = 0;
  f = 0;
  s.clear();
  tie(i, f, s) = r.multiGetAsTuple<int, double, string>("i", "f", "s");
  ASSERT_EQ(42, i);
  ASSERT_EQ(23.23, f);
  ASSERT_EQ("Hallo", s);

  i = 0;
  f = 0;
  s.clear();
  id = 0;
  tie(id, i, f, s) = r.multiGetAsTuple<int, int, double, string>("rowid", "i", "f", "s");
  ASSERT_EQ(1, id);
  ASSERT_EQ(42, i);
  ASSERT_EQ(23.23, f);
  ASSERT_EQ("Hallo", s);

  // test invalid column names
  ASSERT_THROW(r.multiGetByRef("sdfkjsdjkf", i, "f", f), std::invalid_argument);

  // test optional<...> types
  optional<int> oi = 0;
  f = 0;
  ASSERT_NO_THROW(r.multiGetByRef("i", oi, "f", f));
  ASSERT_TRUE(oi.has_value());
  ASSERT_EQ(42, oi.value());
  ASSERT_EQ(23.23, f);

  // test NULL values
  r = TabRow{db, "t1", 2};  // in row #2, the column "i" is NULL
  ASSERT_THROW(r.multiGetByRef("i", i, "f", f), NullValueException);

  oi = 0;
  f = 0;
  ASSERT_NO_THROW(r.multiGetByRef("i", oi, "f", f));
  ASSERT_FALSE(oi.has_value());
  ASSERT_EQ(666.66, f);
}
