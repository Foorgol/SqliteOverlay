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

  // try to get a non-existing table
  ASSERT_THROW(KeyValueTab(db, "sfjklsdf"), NoSuchTableException);

  /*auto kvt = KeyValueTab::getTab(db.get(), "kvt", false);
  ASSERT_TRUE(kvt == nullptr);

  // try to create a new table
  kvt = KeyValueTab::getTab(db.get(), "kvt", true);
  ASSERT_TRUE(kvt != nullptr);
  ASSERT_TRUE(db->hasTable("kvt"));
  auto rawTab = db->getTab("kvt");
  ASSERT_TRUE(rawTab->hasColumn("K"));
  ASSERT_TRUE(rawTab->hasColumn("V"));

  // try to get an existing table
  kvt = nullptr;
  kvt = KeyValueTab::getTab(db.get(), "kvt", false);
  ASSERT_TRUE(kvt != nullptr);*/
}

//----------------------------------------------------------------
/*
TEST_F(DatabaseTestScenario, KeyValueTab_SettersAndGetters)
{
  auto db = getScenario01();

  // create a new table
  auto kvt = KeyValueTab::getTab(db.get(), "kvt", true);
  ASSERT_TRUE(kvt != nullptr);
  auto rawTab = db->getTab("kvt");

  //
  // set and get integers
  //

  // set value for new key
  int err;
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "i") == 0);
  kvt->set("i", 42, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "i") == 1);
  ASSERT_TRUE(kvt->getInt("i") == 42);

  // update value for existing key
  kvt->set("i", 23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "i") == 1);
  ASSERT_TRUE(kvt->getInt("i") == 23);

  // get value of non-existing key
  ASSERT_TRUE(kvt->getInt("sdfljsf") == INT_MIN);

  //
  // set and get doubles
  //

  // set value for new key
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "d") == 0);
  kvt->set("d", 42.42, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "d") == 1);
  ASSERT_TRUE(kvt->getDouble("d") == 42.42);

  // update value for existing key
  kvt->set("d", 23.23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "d") == 1);
  ASSERT_TRUE(kvt->getDouble("d") == 23.23);

  // get value of non-existing key
  ASSERT_TRUE(std::isnan(kvt->getDouble("sdfljsf")));

  //
  // set and get strings
  //

  // set value for new key
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "s") == 0);
  kvt->set("s", "abc", &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "s") == 1);
  ASSERT_EQ("abc", (*kvt)["s"]);

  // update value for existing key
  kvt->set("s", "xyz", &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(rawTab->getMatchCountForColumnValue("K", "s") == 1);
  ASSERT_TRUE((*kvt)["s"] == "xyz");

  // get value of non-existing key
  ASSERT_TRUE((*kvt)["sdfsdfs"].empty());

  //
  // Check query for key existence
  //
  ASSERT_TRUE(kvt->hasKey("s"));
  ASSERT_TRUE(kvt->hasKey("d"));
  ASSERT_TRUE(kvt->hasKey("i"));
  ASSERT_FALSE(kvt->hasKey("sdljkfsdf"));
}
*/
//----------------------------------------------------------------

