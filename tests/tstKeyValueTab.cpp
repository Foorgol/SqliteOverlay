#include <memory>
#include <climits>
#include <cmath>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"
#include "TabRow.h"
#include "KeyValueTab.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, KeyValueTab_Creation)
{
  auto db = getScenario01();

  ASSERT_THROW(db.createNewKeyValueTab(""), std::invalid_argument);
  ASSERT_THROW(db.createNewKeyValueTab(" "), std::invalid_argument);

  auto kvt = db.createNewKeyValueTab(" kvt\t ");
  ASSERT_TRUE(db.hasTable("kvt"));
  DbTab t{db, "kvt", false};
  ASSERT_TRUE(t.hasColumn("K"));
  ASSERT_TRUE(t.hasColumn("V"));

  ASSERT_THROW(db.createNewKeyValueTab("kvt"), std::invalid_argument);

  // this is a test that by default no statements
  // are cached in the KeyValueTab instance
  ASSERT_NO_THROW(db.close());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, KeyValueTab_Ctor)
{
  auto db = getScenario01();
  db.createNewKeyValueTab("kvt");

  // try to get a non-existing table
  ASSERT_THROW(KeyValueTab(db, "sfjklsdf"), NoSuchTableException);

  // get an existing table
  auto kvt = KeyValueTab(db, "kvt");
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, KeyValueTab_SettersAndGetters)
{
  auto db = getScenario01();
  auto kvt = db.createNewKeyValueTab("kvt");
  DbTab rawTab{db, "kvt", false};

  // set value for new key
  ASSERT_EQ(0, rawTab.getMatchCountForColumnValue("K", "i"));
  kvt.set("i", 42);
  ASSERT_EQ(1, rawTab.getMatchCountForColumnValue("K", "i"));
  ASSERT_EQ(42, kvt.getInt("i"));

  // update value for existing key
  kvt.set("i", 666);
  ASSERT_EQ(1, rawTab.getMatchCountForColumnValue("K", "i"));
  ASSERT_EQ(666, kvt.getInt("i"));

  // get the value of a non-existing key
  ASSERT_THROW(kvt.getInt("sdkljf"), NoDataException);
  int v{0};
  //kvt.get("sdkljf", v);
  ASSERT_THROW(kvt.get("sdkljf", v), NoDataException);
  optional<int> oi{123};
  ASSERT_TRUE(oi.has_value());
  ASSERT_NO_THROW(kvt.get("sdkljf", oi));
  ASSERT_FALSE(oi.has_value());

  // get an existing key with an `optional<T>` reference
  ASSERT_NO_THROW(kvt.get("i", oi));
  ASSERT_TRUE(oi.has_value());
  ASSERT_EQ(666, oi.value());

  //
  // get strings using the operator[]
  //
  string s = kvt["i"];
  ASSERT_EQ("666", s);
  ASSERT_THROW(kvt["kfdj"], NoDataException);

  //
  // set and get JSON
  //
  nlohmann::json jsonIn = nlohmann::json::parse(R"({"a": "abc", "b": 42})");
  kvt.set("json", jsonIn);
  WhereClause wc;
  wc.addCol("K", "json");
  TabRow r{db, "kvt", wc};
  ASSERT_EQ("{\"a\":\"abc\",\"b\":42}", r["V"]);
  ASSERT_EQ("{\"a\":\"abc\",\"b\":42}", kvt.getJson("json").dump());
  auto oj = kvt.getJson2("json");
  ASSERT_TRUE(oj.has_value());
  ASSERT_EQ("{\"a\":\"abc\",\"b\":42}", oj.value().dump());

  //
  // getters type 2, not throwing
  //
  auto oi2 = kvt.getInt2("sfsdf");
  ASSERT_FALSE(oi2.has_value());
  oi2 = kvt.getInt2("i");
  ASSERT_TRUE(oi2.has_value());

  kvt.set("l", LONG_MAX);
  auto ol = kvt.getInt64_2("l");
  ASSERT_EQ(LONG_MAX, ol.value());

  // invalid key content returns default values
  kvt.set("l", "abc");
  ASSERT_EQ("abc", kvt["l"]);
  ASSERT_EQ(0, kvt.getLong("l"));

  /*
  //
  // Check query for key existence
  //
  ASSERT_TRUE(kvt->hasKey("s"));
  ASSERT_TRUE(kvt->hasKey("d"));
  ASSERT_TRUE(kvt->hasKey("i"));
  ASSERT_FALSE(kvt->hasKey("sdljkfsdf"));
  */
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, KeyValueTab_KeyQuery)
{
  auto db = getScenario01();
  auto kvt = db.createNewKeyValueTab("kvt");

  kvt.set("i", 42);
  ASSERT_TRUE(kvt.hasKey("i"));
  ASSERT_FALSE(kvt.hasKey("sdljkfsdf"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, KeyValueTab_Constraints)
{
  SqliteDatabase db;
  auto kvt = db.createNewKeyValueTab("kvt");
  ASSERT_EQ(0, kvt.size());

  const vector<string> refVals{"", " ", "abc", "123", "a12b", "#+.", "12.3", "-5", "on", "0", "Europe/Berlin", "2019-04-15"};
  for (int i=0; i < refVals.size(); ++i)
  {
    const string k = "k" + to_string(i);
    kvt.set(k, refVals[i]);
  }
  ASSERT_EQ(refVals.size(), kvt.size());

  // prepare a set of constraints plus expected results for each entry
  vector<pair<Sloppy::ValueConstraint, vector<bool>>> constrChecks{
    {Sloppy::ValueConstraint::Exist, {1,1,1,1,1,1,1,1,1,1,1,1,   0   }},  // in this special case we check for one more key than actually exist
    {Sloppy::ValueConstraint::NotEmpty, {0,1,1,1,1,1,1,1,1,1,1,1}},
    {Sloppy::ValueConstraint::Alnum, {0,0,1,1,1,0,0,0,1,1,0,0}},   // the dot in "12.3" makes in NOT alphanumeric!
    {Sloppy::ValueConstraint::Alpha, {0,0,1,0,0,0,0,0,1,0,0,0}},
    {Sloppy::ValueConstraint::Digit, {0,0,0,1,0,0,0,0,0,1,0,0}},
    {Sloppy::ValueConstraint::Numeric, {0,0,0,1,0,0,1,1,0,1,0,0}},
    {Sloppy::ValueConstraint::Integer, {0,0,0,1,0,0,0,1,0,1,0,0}},
    {Sloppy::ValueConstraint::Bool, {0,0,0,0,0,0,0,0,1,1,0,0}},
    {Sloppy::ValueConstraint::StandardTimezone, {0,0,0,0,0,0,0,0,0,0,1,0}},
    {Sloppy::ValueConstraint::IsoDate, {0,0,0,0,0,0,0,0,0,0,0,1}},
  };

  // loop over all constraints and all key for each constraint
  for (const pair<Sloppy::ValueConstraint, vector<bool>>& p : constrChecks)
  {
    // make sure we didn't forget one or the other check
    ASSERT_TRUE(p.second.size() >= refVals.size());

    for (int i=0; i < p.second.size(); ++i)
    {
      const string k = "k" + to_string(i);
      ASSERT_EQ(p.second[i], kvt.checkConstraint(k, p.first));
    }
  }
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, KeyValueTab_Remove)
{
  auto db = getScenario01();
  auto kvt = db.createNewKeyValueTab("kvt");

  ASSERT_EQ(0, kvt.size());

  kvt.set("i", 42);
  ASSERT_EQ(1, kvt.size());
  auto v = kvt.getInt2("i");
  ASSERT_TRUE(v.has_value());
  ASSERT_EQ(42, *v);

  kvt.remove("i");
  ASSERT_EQ(0, kvt.size());
  v = kvt.getInt2("i");
  ASSERT_FALSE(v.has_value());

  ASSERT_NO_THROW(kvt.remove("sdkjfhsdjkf"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, KeyValueTab_AllKeys)
{
  auto db = getScenario01();
  auto kvt = db.createNewKeyValueTab("kvt");

  auto ak = kvt.allKeys();
  ASSERT_TRUE(ak.empty());

  kvt.set("k1", "sdkfj");
  ak = kvt.allKeys();
  ASSERT_EQ(1, ak.size());
  ASSERT_EQ("k1", ak[0]);

  kvt.set("k2", 12.34);
  ak = kvt.allKeys();
  ASSERT_EQ(2, ak.size());
  ASSERT_TRUE((ak[0] == "k1") || (ak[1] == "k1"));
  ASSERT_TRUE((ak[0] == "k2") || (ak[1] == "k2"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, KeyValueTab_Caching)
{
  auto db = getScenario01();
  auto kvt = db.createNewKeyValueTab("kvt");

  // enable caching
  kvt.enableStatementCache(true);

  // trigger a few queries so that we
  // get pending, un-finalized statements
  kvt.set("k1", 42);
  ASSERT_EQ(42, kvt.getInt("k1"));

  // a close() operation on the database should now fail
  ASSERT_THROW(db.close(), BusyException);

  // release all statements and succeed in closing the database
  kvt.releaseDatabase();
  ASSERT_NO_THROW(db.close());
}
