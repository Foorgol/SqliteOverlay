#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
//#include "ClausesAndQueries.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, HasViewHasTable)
{
  SampleDB db = getScenario01();

  ASSERT_TRUE(db.hasView("v1"));
  ASSERT_FALSE(db.hasView("sdfklsfd"));
  ASSERT_FALSE(db.hasView("t1"));

  ASSERT_TRUE(db.hasTable("t1"));
  ASSERT_FALSE(db.hasTable("sdfklsfd"));
  ASSERT_FALSE(db.hasTable("v1"));

}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, LastInsertIdAndRowsAffected)
{
  SampleDB db = getScenario01();

  // get the current max ID
  int maxId = db.execScalarQuery<int>("SELECT MAX(rowid) FROM t1");
  ASSERT_TRUE(maxId > 0);

  // insert a new row
  db.execNonQuery("INSERT INTO t1(i) VALUES(123)");
  ASSERT_EQ(maxId+1, db.getLastInsertId());
  ASSERT_EQ(1, db.getRowsAffected());

  // delete all rows and check getRowsAffected
  db.execNonQuery("DELETE FROM t1 WHERE rowid>0");
  ASSERT_EQ(maxId + 1, db.getRowsAffected());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, CheckDirty)
{
  auto db1 = getScenario01();
  auto db2 = db1.duplicateConnection<SampleDB>(false);

  // we start with clean databases
  ASSERT_FALSE(db1.isDirty());
  ASSERT_FALSE(db1.hasLocalChanges());
  ASSERT_FALSE(db1.hasExternalChanges());
  ASSERT_FALSE(db2.isDirty());
  ASSERT_FALSE(db2.hasLocalChanges());
  ASSERT_FALSE(db2.hasExternalChanges());

  // modify the DB on connection 1
  db1.execNonQuery("INSERT INTO t1(i) VALUES(123)");

  // check all local and global dirty flags
  ASSERT_TRUE(db1.isDirty());
  ASSERT_TRUE(db1.hasLocalChanges());
  ASSERT_FALSE(db1.hasExternalChanges());
  ASSERT_TRUE(db2.isDirty());
  ASSERT_FALSE(db2.hasLocalChanges());
  ASSERT_TRUE(db2.hasExternalChanges());

  // reset the local change flags
  db1.resetLocalChangeCounter();
  ASSERT_FALSE(db1.isDirty());
  ASSERT_FALSE(db1.hasLocalChanges());
  ASSERT_FALSE(db1.hasExternalChanges());
  ASSERT_TRUE(db2.isDirty());
  ASSERT_FALSE(db2.hasLocalChanges());
  ASSERT_TRUE(db2.hasExternalChanges());

  // modify the DB on connection 2
  db2.execNonQuery("INSERT INTO t1(i) VALUES(456)");
  ASSERT_TRUE(db1.isDirty());
  ASSERT_FALSE(db1.hasLocalChanges());
  ASSERT_TRUE(db1.hasExternalChanges());  // second insertion via db2
  ASSERT_TRUE(db2.isDirty());
  ASSERT_TRUE(db2.hasLocalChanges());
  ASSERT_TRUE(db2.hasExternalChanges());  // db2 still sees the first insertion via db1, since the global counters on db2 have not been reset

  // clear db1's external change flag
  db1.resetExternalChangeCounter();
  ASSERT_FALSE(db1.isDirty());
  ASSERT_FALSE(db1.hasLocalChanges());
  ASSERT_FALSE(db1.hasExternalChanges());
  ASSERT_TRUE(db2.isDirty());
  ASSERT_TRUE(db2.hasLocalChanges());
  ASSERT_TRUE(db2.hasExternalChanges());  // db2 still sees the first insertion ia db1, since the global counters on db2 have not been reset

  // clear db2's overall dirty flag
  db2.resetDirtyFlag();
  ASSERT_FALSE(db1.isDirty());
  ASSERT_FALSE(db1.hasLocalChanges());
  ASSERT_FALSE(db1.hasExternalChanges());
  ASSERT_FALSE(db2.isDirty());
  ASSERT_FALSE(db2.hasLocalChanges());
  ASSERT_FALSE(db2.hasExternalChanges());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, CheckDatabaseComparison)
{
  auto db1 = getScenario01();
  auto db2 = db1.duplicateConnection(false);
  SqliteDatabase memDb{};

  ASSERT_TRUE(db1 == db2);  // different handles but same file
  ASSERT_TRUE(db1 == db1);
  ASSERT_TRUE(memDb == memDb);

  ASSERT_FALSE(memDb == db1);


  ASSERT_FALSE(db1 != db2);  // different handles but same file
  ASSERT_FALSE(db1 != db1);
  ASSERT_FALSE(memDb != memDb);

  ASSERT_TRUE(memDb != db1);
}
