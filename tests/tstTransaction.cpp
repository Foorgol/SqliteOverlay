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
#include "TableCreator.h"

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
  auto tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
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

  // make sure that connection 2 can't start a transaction
  // while the first one is still active
  auto tr2 = db2->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
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
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
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

TEST_F(DatabaseTestScenario, TransactionWithKeyConflict)
{
  auto db = getScenario01();

  // create a child table that references to a parent, in this case t1
  int err;
  TableCreator tc{db.get()};
  tc.addForeignKey("t1Ref", "t1", CONSISTENCY_ACTION::RESTRICT, CONSISTENCY_ACTION::RESTRICT);
  auto child = tc.createTableAndResetCreator("child", &err);
  assert(child != nullptr);
  assert(err = SQLITE_DONE);

  // try to insert a child row with invalid reference
  ColumnValueClause cvc;
  cvc.addIntCol("t1Ref", 42);
  ASSERT_TRUE(child->insertRow(cvc, &err) < 0);
  ASSERT_EQ(SQLITE_CONSTRAINT, err);

  // insert a child row with a valid reference
  cvc.clear();
  cvc.addIntCol("t1Ref", 1);
  ASSERT_TRUE(child->insertRow(cvc, &err) > 0);
  ASSERT_EQ(SQLITE_DONE, err);

  // insert a valid and an invalid reference
  // as part of a larger transaction
  //
  // make sure the row with the valid reference
  // survives
  auto tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  int oldRowCount = child->length();
  cvc.clear();
  cvc.addIntCol("t1Ref", 1);
  ASSERT_TRUE(child->insertRow(cvc, &err) > 0);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(oldRowCount + 1, child->length());
  cvc.clear();
  cvc.addIntCol("t1Ref", 88);
  ASSERT_TRUE(child->insertRow(cvc, &err) < 0);
  ASSERT_EQ(SQLITE_CONSTRAINT, err);
  ASSERT_EQ(oldRowCount + 1, child->length());
  ASSERT_TRUE(tr->commit(&err));
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(oldRowCount + 1, child->length());

  //
  // try to delete a row from the parent table that has
  // pending child references
  //
  auto t1 = db->getTab("t1");
  oldRowCount = t1->length();
  ASSERT_TRUE(t1 != nullptr);
  ASSERT_EQ(-1, t1->deleteRowsByColumnValue("id", 1, &err));
  ASSERT_EQ(SQLITE_CONSTRAINT, err);
  ASSERT_EQ(oldRowCount, t1->length());

  //
  // try to delete a row from the parent table that has
  // pending child references. Make the deletion a part
  // of a larger transaction.
  //
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(1, t1->deleteRowsByColumnValue("id", 2, &err));
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(oldRowCount - 1, t1->length());
  ASSERT_EQ(-1, t1->deleteRowsByColumnValue("id", 1, &err));
  ASSERT_EQ(SQLITE_CONSTRAINT, err);
  ASSERT_EQ(oldRowCount - 1, t1->length());
  tr->commit(&err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(oldRowCount - 1, t1->length());

  //
  // same as before, but this time rollback the changes
  //
  --oldRowCount;
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(1, t1->deleteRowsByColumnValue("id", 3, &err));
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(oldRowCount - 1, t1->length());
  ASSERT_EQ(0, t1->getMatchCountForColumnValue("id", 3));
  ASSERT_EQ(-1, t1->deleteRowsByColumnValue("id", 1, &err));
  ASSERT_EQ(SQLITE_CONSTRAINT, err);
  ASSERT_EQ(oldRowCount - 1, t1->length());
  tr->rollback(&err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(oldRowCount, t1->length());
  ASSERT_EQ(1, t1->getMatchCountForColumnValue("id", 3));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, NestedTransaction)
{
  auto db = getScenario01();

  // get a second, concurrent connection
  auto db2 = SqliteDatabase::get<SampleDB>(getSqliteFileName(), false);

  //
  // Start an immediate transaction and change values
  //
  int err;
  auto tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_FALSE(tr->isNested());
  DbTab* t1_1 = db->getTab("t1");
  TabRow r1_1 = t1_1->operator [](1);
  r1_1.update("i", 23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));

  //
  // Start another transaction on the same DB-connection
  //
  auto trNested = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(trNested != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_TRUE(trNested->isNested());

  // update a value as part of the inner transaction
  r1_1.update("i", 0, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(0, r1_1.getInt("i"));

  // make sure that connection 2 still sees the initial value
  DbTab* t1_2 = db2->getTab("t1");
  TabRow r1_2 = t1_2->operator [](1);
  ASSERT_EQ(42, r1_2.getInt("i"));

  // commit the inner transaction
  ASSERT_TRUE(trNested->commit(&err));
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(0, r1_1.getInt("i"));

  // make sure that connection 2 still sees the initial value
  ASSERT_EQ(42, r1_2.getInt("i"));

  // ROLLBACK the outer transaction with
  // undoes the already committed changes of the
  // inner transaction
  ASSERT_TRUE(tr->rollback(&err));
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(42, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  //
  // Same as before, but this time we ROLLBACK the inner
  // transaction and commit the outer transaction
  //
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  r1_1.update("i", 23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  trNested = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(trNested != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  r1_1.update("i", 0, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(0, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  ASSERT_TRUE(trNested->rollback(&err));  // inner ROLLBACK
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  ASSERT_TRUE(tr->commit(&err));  // outer commit
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(23, r1_2.getInt("i"));

  //
  // Same as before, but this time we only
  // commit the outer transaction
  //
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  r1_1.update("i", 42, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(42, r1_1.getInt("i"));
  ASSERT_EQ(23, r1_2.getInt("i"));

  trNested = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(trNested != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  r1_1.update("i", 0, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(0, r1_1.getInt("i"));
  ASSERT_EQ(23, r1_2.getInt("i"));

  ASSERT_TRUE(tr->commit(&err));  // outer commit
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(0, r1_1.getInt("i"));
  ASSERT_EQ(0, r1_2.getInt("i"));

  ASSERT_FALSE(trNested->commit(&err));   // an inner commit must fail after the outer commit
  ASSERT_EQ(SQLITE_ERROR, err);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DtorAction)
{
  auto db = getScenario01();

  // get a second, concurrent connection
  auto db2 = SqliteDatabase::get<SampleDB>(getSqliteFileName(), false);

  int err;
  DbTab* t1_1 = db->getTab("t1");
  TabRow r1_1 = t1_1->operator [](1);
  DbTab* t1_2 = db2->getTab("t1");
  TabRow r1_2 = t1_2->operator [](1);

  //
  // Start an immediate transaction and change values
  // in a separate block so that the transaction's dtor
  // is called when leaving the scope
  //
  {
    auto tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
    ASSERT_TRUE(tr != nullptr);
    ASSERT_EQ(SQLITE_DONE, err);

    // change a value via connection 1
    r1_1.update("i", 23, &err);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_EQ(23, r1_1.getInt("i"));

    // make sure that connection 2 still sees the old value
    ASSERT_EQ(42, r1_2.getInt("i"));
  }

  // since the dtor-action was "rollback", also connection 1
  // should see the old value again
  ASSERT_EQ(42, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  //
  // same as above, but this time with
  // "commit" as the dtor-action
  //
  {
    auto tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::COMMIT, &err);
    ASSERT_TRUE(tr != nullptr);
    ASSERT_EQ(SQLITE_DONE, err);

    // change a value via connection 1
    r1_1.update("i", 23, &err);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_EQ(23, r1_1.getInt("i"));

    // make sure that connection 2 still sees the old value
    ASSERT_EQ(42, r1_2.getInt("i"));
  }

  // since the dtor-action was "commit", both connections
  // should see the updated value
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(23, r1_2.getInt("i"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, NestedTransactionWithDtor)
{
  auto db = getScenario01();

  // get a second, concurrent connection
  auto db2 = SqliteDatabase::get<SampleDB>(getSqliteFileName(), false);

  DbTab* t1_1 = db->getTab("t1");
  TabRow r1_1 = t1_1->operator [](1);
  DbTab* t1_2 = db2->getTab("t1");
  TabRow r1_2 = t1_2->operator [](1);

  //
  // Start an immediate transaction and change values
  //
  int err;
  auto tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_FALSE(tr->isNested());
  r1_1.update("i", 23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));

  //
  // Start another transaction on the same DB-connection
  // with dtor-action "rollback" and in a separate scope
  //
  {
    auto trNested = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
    ASSERT_TRUE(trNested != nullptr);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_TRUE(trNested->isNested());

    // update a value as part of the inner transaction
    r1_1.update("i", 0, &err);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_EQ(0, r1_1.getInt("i"));

    // make sure that connection 2 still sees the initial value
    ASSERT_EQ(42, r1_2.getInt("i"));

    // the modification will be rolled back when leaving this scope
  }

  // make sure that connection 2 still sees the initial value
  // and connection 1 the value of the outer transaction
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  // ROLLBACK the outer transaction by calling
  // its destructor
  tr.reset();
  ASSERT_EQ(42, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  //
  // same as above, but this time with "commit" as dtor-action
  // in the inner transaction
  //
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_FALSE(tr->isNested());
  r1_1.update("i", 23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));

  {
    auto trNested = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::COMMIT, &err);
    ASSERT_TRUE(trNested != nullptr);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_TRUE(trNested->isNested());

    r1_1.update("i", 0, &err);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_EQ(0, r1_1.getInt("i"));

    // make sure that connection 2 still sees the initial value
    ASSERT_EQ(42, r1_2.getInt("i"));

    // dtor-call follows
  }

  // make sure that connection 2 still sees the initial value
  // and connection 1 the value of the inner transaction
  ASSERT_EQ(0, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  // ROLLBACK the outer transaction by calling
  // its destructor
  tr.reset();
  ASSERT_EQ(42, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));


  //
  // same as above, but this time with "rollback" as dtor-action
  // in the inner transaction and "commit" in the outer
  //
  tr = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::COMMIT, &err);
  ASSERT_TRUE(tr != nullptr);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_FALSE(tr->isNested());
  r1_1.update("i", 23, &err);
  ASSERT_EQ(SQLITE_DONE, err);
  ASSERT_EQ(23, r1_1.getInt("i"));

  {
    auto trNested = db->startTransaction(TRANSACTION_TYPE::IMMEDIATE, TRANSACTION_DESTRUCTOR_ACTION::ROLLBACK, &err);
    ASSERT_TRUE(trNested != nullptr);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_TRUE(trNested->isNested());

    r1_1.update("i", 0, &err);
    ASSERT_EQ(SQLITE_DONE, err);
    ASSERT_EQ(0, r1_1.getInt("i"));

    // make sure that connection 2 still sees the initial value
    ASSERT_EQ(42, r1_2.getInt("i"));

    // dtor-call follows
  }

  // make sure that connection 2 still sees the initial value
  // and connection 1 the value of the outer transaction
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(42, r1_2.getInt("i"));

  // commit the outer transaction by calling
  // its destructor
  tr.reset();
  ASSERT_EQ(23, r1_1.getInt("i"));
  ASSERT_EQ(23, r1_2.getInt("i"));
}
