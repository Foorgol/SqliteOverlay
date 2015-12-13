#include <memory>
#include <climits>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "Transaction.h"
#include "SampleDB.h"
#include "SqliteDatabase.h"
#include "DbTab.h"
#include "TabRow.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, Transaction)
{
  auto db = getScenario01();

  // get a second, concurrent connection
  auto db2 = SqliteDatabase::get<SampleDB>(getSqliteFileName(), false);

  //
  // Start an immediate transaction and change values
  //
  int err;
  auto tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);

  // change a value via connection 1
  DbTab* t1_1 = db->getTab("t1");
  TabRow r1_1 = t1_1->operator [](1);
  r1_1.update("i", 23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));

  // make sure that connection 2 still sees the old value
  DbTab* t1_2 = db2->getTab("t1");
  TabRow r1_2 = t1_2->operator [](1);
  ASSERT_EQ(42, r1_2.getInt("i"));

  // make sure that connectio 2 can't start a transaction
  // while the first one is still active
  auto tr2 = db2->startTransaction(TRANSACTION_TYPE::IMMEDIATE, &err);
  ASSERT_TRUE(tr2 == nullptr);
  ASSERT_EQ(SQLITE_BUSY, err);

  // commit the changes of connection 1
  ASSERT_TRUE(tr->commit(&err));
  ASSERT_EQ(SQLITE_DONE, err);

  // make sure that connection 2 now sees the new value
  ASSERT_EQ(23, r1_2.getInt("i"));

  //
  // Start an immediate transaction and rollback
  //
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);

  // change a value via connection 1
  r1_1.update("i", 666, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(666, r1_1.getInt("i"));

  // make sure that connection 2 still sees the old value
  ASSERT_EQ(23, r1_2.getInt("i"));

  // rollback the changes of connection 1
  ASSERT_TRUE(tr->rollback(&err));
  ASSERT_EQ(SQLITE_DONE, err);

  // make sure that both connections now sees the old value
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(23, r1_2.getInt("i"));
}

//----------------------------------------------------------------

