#include <memory>
#include <climits>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "HelperFunc.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"
#include "TabRow.h"
#include "KeyValueTab.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, CopyTable)
{
  auto db = getScenario01();
  int err;

  // try invalid parameters
  ASSERT_FALSE(db->copyTable("", "", &err));
  ASSERT_FALSE(db->copyTable("t1", "", &err));
  ASSERT_FALSE(db->copyTable("t1", "t1", &err));
  ASSERT_FALSE(db->copyTable("sfsfsafd", "t2", &err));

  // copy whole table t1 to t1_copy
  ASSERT_TRUE(db->copyTable("t1", "t1_copy", &err));

  // make sure the new table exists and is filled
  ASSERT_TRUE(db->hasTable("t1_copy"));
  ASSERT_TRUE(db->getTab("t1")->length() == db->getTab("t1_copy")->length());
}

