#include <memory>
#include <climits>

#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "Transaction.h"
#include "SampleDB.h"
#include "SqliteDatabase.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, BasicTransaction)
{
  auto db = getScenario01();

  // get a second, concurrent connection
  auto db2 = db.duplicateConnection(false);

  //
  // Start an immediate transaction and change values
  //
  auto tr = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);
  ASSERT_TRUE(tr.isActive());

  // change a value via connection 1
  auto stmt = db.prepStatement("UPDATE t1 SET i=23 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // make sure that connection 2 still sees the old value
  ASSERT_EQ(42, db2.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // make sure that connectio 2 can't start a transaction
  // while the first one is still active
  ASSERT_THROW(db2.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback), BusyException);

  // commit the changes of connection 1
  ASSERT_NO_THROW(tr.commit());
  ASSERT_FALSE(tr.isActive());

  // make sure that connection 2 now sees the new value
  ASSERT_EQ(23, db2.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // everthing should no be without transaction
  ASSERT_TRUE(db.isAutoCommit());

  //
  // Start an immediate transaction and rollback
  //
  tr = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);
  ASSERT_TRUE(tr.isActive());

  // change a value via connection 1
  stmt = db.prepStatement("UPDATE t1 SET i=666 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(666, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // make sure that connection 2 still sees the old value
  ASSERT_EQ(23, db2.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // rollback the changes of connection 1
  ASSERT_NO_THROW(tr.rollback());
  ASSERT_FALSE(tr.isActive());

  // make sure that both connections now see the old value
  ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));
  ASSERT_EQ(23, db2.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TransactionDtor)
{
  auto db = getScenario01();

  //
  // Start an immediate transaction in a separate scope
  // and use "Rollback" as the dtor action
  //
  {
    auto tr = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);
    auto stmt = db.prepStatement("UPDATE t1 SET i=23 WHERE rowid=1");
    ASSERT_TRUE(stmt.step());
    ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

    // transaction dtor is called when leaving the scope
  }

  // we should see the original value now
  ASSERT_EQ(42, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  //
  // Start an immediate transaction in a separate scope
  // and use "Commit" as the dtor action
  //
  {
    auto tr = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Commit);
    auto stmt = db.prepStatement("UPDATE t1 SET i=23 WHERE rowid=1");
    ASSERT_TRUE(stmt.step());
    ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

    // transaction dtor is called when leaving the scope
  }

  // we should see the new value now
  ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, NestedTransaction1)
{
  auto db = getScenario01();

  // no transaction yet
  ASSERT_TRUE(db.isAutoCommit());

  // first transaction
  auto tr1 = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);
  ASSERT_TRUE(tr1.isActive());
  ASSERT_FALSE(tr1.isNested());

  // apply a change within the first transaction
  auto stmt = db.prepStatement("UPDATE t1 SET i=23 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // start a second transaction
  auto tr2 = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);
  ASSERT_TRUE(tr2.isActive());
  ASSERT_TRUE(tr2.isNested());

  // apply a change within the second transaction
  stmt = db.prepStatement("UPDATE t1 SET i=666 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(666, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // undo the second transaction
  tr2.rollback();
  ASSERT_FALSE(tr2.isActive());
  ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // unto the first transaction
  tr1.rollback();
  ASSERT_FALSE(tr1.isActive());
  ASSERT_EQ(42, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, NestedTransaction2)
{
  auto db = getScenario01();

  // no transaction yet
  ASSERT_TRUE(db.isAutoCommit());

  // first transaction
  auto tr1 = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);
  ASSERT_TRUE(tr1.isActive());
  ASSERT_FALSE(tr1.isNested());

  // apply a change within the first transaction
  auto stmt = db.prepStatement("UPDATE t1 SET i=23 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(23, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // start a second transaction
  auto tr2 = db.startTransaction(TransactionType::Immediate, TransactionDtorAction::Rollback);
  ASSERT_TRUE(tr2.isActive());
  ASSERT_TRUE(tr2.isNested());

  // apply a change within the second transaction
  stmt = db.prepStatement("UPDATE t1 SET i=666 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(666, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // commit the outer transaction; this implicitly commits
  // the changes applied by the inner transaction.
  //
  // think of all changes as a sequence on a timeline...
  tr1.commit();
  ASSERT_FALSE(tr1.isActive());
  ASSERT_EQ(666, db.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));

  // committing the inner transaction is now invalid
  // and the inner transaction is set to "finished"
  ASSERT_THROW(tr2.rollback(), GenericSqliteException);
  ASSERT_FALSE(tr2.isActive());
}
/*
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
*/
