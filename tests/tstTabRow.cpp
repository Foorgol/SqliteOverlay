#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include <Sloppy/Crypto/Sodium.h>
#include <Sloppy/Crypto/Crypto.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "TabRow.h"
#include "CommonTabularClass.h"
//#include "DbTab.h"

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

  // test JSON functions
  nlohmann::json jsonIn = nlohmann::json::parse(R"({"a": "abc", "b": 42})");
  r.update("s", jsonIn);
  nlohmann::json jsonOut1 = r.getJson("s");
  ASSERT_EQ("abc", jsonOut1["a"]);
  ASSERT_EQ(42, jsonOut1["b"]);
  auto jsonOut2 = r.getJson2("s");
  ASSERT_TRUE(jsonOut2.has_value());
  ASSERT_EQ("abc", jsonOut2->at("a"));
  ASSERT_EQ(42, jsonOut2->at("b"));
  r.update("s", "invalid JSON data");
  ASSERT_THROW(r.getJson("s"), nlohmann::json::parse_error);
  r.updateToNull("s");
  ASSERT_THROW(r.getJson("s"), NullValueException);
  auto oj = r.getJson2("s");
  ASSERT_FALSE(oj.has_value());
  r.update("s", "null");
  jsonOut1 = r.getJson("s");
  ASSERT_TRUE(jsonOut1.empty());
  ASSERT_EQ(nlohmann::json::value_t::null, jsonOut1.type());
  r.update("s", "{}");
  jsonOut1 = r.getJson("s");
  ASSERT_TRUE(jsonOut1.empty());
  ASSERT_EQ(nlohmann::json::value_t::object, jsonOut1.type());


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

//----------------------------------------------------------------

TEST(TabRowTests, ConstraintChecks)
{
  auto db = SqliteDatabase();

  db.execNonQuery("CREATE TABLE t(x)");
  db.execNonQuery("INSERT INTO t(x) VALUES(NULL)");
  SqlStatement stmt = db.prepStatement("INSERT INTO t(x) VALUES(?)");
  for (const string& v : {"42", "abc", "1234", "123abc", ""})
  {
    stmt.bind(1, v);
    stmt.step();
    stmt.reset(true);
  }

  TabRow r1{db, "t", 1, true};
  TabRow r2{db, "t", 2, true};
  TabRow r6{db, "t", 6, true};

  ASSERT_FALSE(r1.checkConstraint("x", Sloppy::ValueConstraint::Exist));
  ASSERT_TRUE(r2.checkConstraint("x", Sloppy::ValueConstraint::Exist));
  ASSERT_TRUE(r6.checkConstraint("x", Sloppy::ValueConstraint::Exist));

  ASSERT_FALSE(r1.checkConstraint("x", Sloppy::ValueConstraint::NotEmpty));
  ASSERT_TRUE(r2.checkConstraint("x", Sloppy::ValueConstraint::NotEmpty));
  ASSERT_FALSE(r6.checkConstraint("x", Sloppy::ValueConstraint::NotEmpty));

  ASSERT_FALSE(r1.checkConstraint("x", Sloppy::ValueConstraint::Numeric));
  ASSERT_TRUE(r2.checkConstraint("x", Sloppy::ValueConstraint::Numeric));
  ASSERT_FALSE(r6.checkConstraint("x", Sloppy::ValueConstraint::Numeric));

  ASSERT_FALSE(r1.checkConstraint("x", Sloppy::ValueConstraint::Alpha));
  ASSERT_FALSE(r2.checkConstraint("x", Sloppy::ValueConstraint::Alpha));
  ASSERT_FALSE(r6.checkConstraint("x", Sloppy::ValueConstraint::Alpha));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_Blob)
{
  static constexpr int BufSize = 1000000;

  auto db = getScenario01();

  // create 1M dummy data
  auto* sodium = Sloppy::Crypto::SodiumLib::getInstance();
  Sloppy::MemArray buf{BufSize};
  sodium->randombytes_buf(buf);

  TabRow r(db, "t1", 1);
  ASSERT_EQ(1, r.id());

  // assign the blob data to the "i" column
  r.update("i", buf.view());

  // retrieve the value
  auto bufBack = r.getBlob("i");
  ASSERT_FALSE(bufBack.empty());
  ASSERT_EQ(BufSize, bufBack.size());

  // compare both buffers; only succeeds if the buffers have equal size and content
  ASSERT_TRUE(sodium->memcmp(buf.view(), bufBack.view()));

  // test the "optional" variant
  auto opt = r.getBlob2("i");
  ASSERT_TRUE(opt.has_value());
  ASSERT_TRUE(sodium->memcmp(buf.view(), opt->view()));
  r.updateToNull("i");
  ASSERT_THROW(r.getBlob("i"), NullValueException);
  opt = r.getBlob2("i");
  ASSERT_FALSE(opt.has_value());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRow_CSV)
{
  auto db = getScenario01();

  TabRow r(db, "t1", 1);

  auto csv = r.toCSV(true);
  ASSERT_EQ(5, csv.size());
  ASSERT_EQ(1, csv.get(0).get<int64_t>());
  ASSERT_EQ(42, csv.get(1).get<int64_t>());
  ASSERT_EQ(23.23, csv.get(2).get<double>());
  ASSERT_EQ("Hallo", csv.get(3).get<string>());

  csv = r.toCSV(false);
  ASSERT_EQ(4, csv.size());
  ASSERT_EQ(42, csv.get(0).get<int64_t>());
  ASSERT_EQ(23.23, csv.get(1).get<double>());
  ASSERT_EQ("Hallo", csv.get(2).get<string>());

  csv = r.toCSV({"rowid", "s"});
  ASSERT_EQ(2, csv.size());
  ASSERT_EQ(1, csv.get(0).get<int64_t>());
  ASSERT_EQ("Hallo", csv.get(1).get<string>());

  csv = r.toCSV(vector<string>{});
  ASSERT_EQ(0, csv.size());
}
