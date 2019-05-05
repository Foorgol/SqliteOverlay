#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, SingleColumnIterator_Plain)
{
  SampleDB db = getScenario01();
  DbTab t1{db, "t1", false};

  vector<int> expected{
    42,
    -1,   // is a placeholder for "NULL"
    84,
    84,
    84
  };

  int rowid=1;
  for (auto it = t1.singleColumnIterator<int>("i"); it.hasData(); ++it)
  {
    ASSERT_EQ(rowid, it.rowid());
    int ex = expected[rowid-1];

    // test operator*
    if (ex > 0)
    {
      ASSERT_EQ(ex, *it);
    } else {
      ASSERT_THROW(*it, NullValueException);
    }

    // test get()
    int real{-42};
    if (ex > 0)
    {
      it.get(real);
      ASSERT_EQ(ex, real);
    } else {
      ASSERT_THROW(it.get(real), NullValueException);
    }

    // test get2
    optional<int> oi = it.get2();
    if (ex > 0)
    {
      ASSERT_EQ(ex, oi.value());
    } else {
      ASSERT_FALSE(oi.has_value());
    }

    ++rowid;
  }
  ASSERT_EQ(6, rowid);  // 5 rows --> 5 increments, starting from "1"
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, SingleColumnIterator_IntRange)
{
  SampleDB db = getScenario01();
  DbTab t1{db, "t1", false};

  vector<int> expected{
    42,
    -1,   // is a placeholder for "NULL"
    84,
    84,
    84
  };

  auto testRange = [&](int minId, int maxId, int cntExpected)
  {
    int rowid{(minId) < 1 ? 1 : minId};
    int cnt{0};

    for (auto it = t1.singleColumnIterator<int>("i", minId, maxId); it.hasData(); ++it)
    {
      ASSERT_EQ(rowid, it.rowid());
      int ex = expected[rowid-1];

      // test operator*
      if (ex > 0)
      {
        ASSERT_EQ(ex, *it);
      } else {
        ASSERT_THROW(*it, NullValueException);
      }

      ++rowid;
      ++cnt;
    }

    ASSERT_EQ(cntExpected, cnt);
  };

  testRange(3, 3, 1);
  testRange(2, -1, 4);
  testRange(3,2,0);
  testRange(-1, 1, 1);
  testRange(-1, 100, 5);
  testRange(-1, -1, 5);
}

  //----------------------------------------------------------------

TEST_F(DatabaseTestScenario, SingleColumnIterator_EmptyTable)
{
  SampleDB db = getScenario01();
  DbTab t2{db, "t2", false};
  ASSERT_EQ(0, t2.length());

  auto it = t2.singleColumnIterator<int>("i");
  ASSERT_FALSE(it.hasData());
  ASSERT_FALSE(++it);
  ASSERT_THROW(*it, NoDataException);

  int cnt{0};
  for (it = t2.singleColumnIterator<int>("i"); it.hasData(); ++it)
  {
    ++cnt;
  }
  ASSERT_EQ(0, cnt);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRowIterator_Plain)
{
  SampleDB db = getScenario01();
  DbTab t1{db, "t1", false};

  int rowid=1;
  for (auto it = t1.tabRowIterator(); it.hasData(); ++it)
  {
    ASSERT_EQ(rowid, it.rowid());

    // test operator*
    TabRow& rowRef = *it;
    ASSERT_EQ(rowid, rowRef.id());

    // test operator->
    ASSERT_EQ(rowid, it->id());

    ++rowid;
  }
  ASSERT_EQ(6, rowid);  // 5 rows --> 5 increments, starting from "1"
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRowIterator_IntRange)
{
  SampleDB db = getScenario01();
  DbTab t1{db, "t1", false};

  auto testRange = [&](int minId, int maxId, int cntExpected)
  {
    int rowid{(minId) < 1 ? 1 : minId};
    int cnt{0};

    for (auto it = t1.tabRowIterator(minId, maxId); it.hasData(); ++it)
    {
      ASSERT_EQ(rowid, it.rowid());

      // test operator*
      TabRow& rowRef = *it;
      ASSERT_EQ(rowid, rowRef.id());

      // test operator->
      ASSERT_EQ(rowid, it->id());

      ++rowid;
      ++cnt;
    }

    ASSERT_EQ(cntExpected, cnt);
  };

  testRange(3, 3, 1);
  testRange(2, -1, 4);
  testRange(3,2,0);
  testRange(-1, 1, 1);
  testRange(-1, 100, 5);
  testRange(-1, -1, 5);
}

  //----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TabRowIterator_EmptyTable)
{
  SampleDB db = getScenario01();
  DbTab t2{db, "t2", false};
  ASSERT_EQ(0, t2.length());

  auto it = t2.tabRowIterator();
  ASSERT_FALSE(it.hasData());
  ASSERT_FALSE(++it);
  ASSERT_THROW(*it, NoDataException);
  ASSERT_THROW(it->id(), NoDataException);

  int cnt{0};
  for (it = t2.tabRowIterator(); it.hasData(); ++it)
  {
    ++cnt;
  }
  ASSERT_EQ(0, cnt);
}

//----------------------------------------------------------------

