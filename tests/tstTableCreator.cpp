#include <gtest/gtest.h>

#include "TableCreator.h"

using namespace SqliteOverlay;
namespace bfs = boost::filesystem;

TEST(TableCreatorTests, AddColAndGetStatement)
{
  TableCreator tc;

  // no custom columns defined
  ASSERT_EQ("CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY)", tc.getSqlStatement("t"));

  // add an integer column without default value
  tc.addCol("c", ColumnDataType::Integer, ConflictClause::Abort, ConflictClause::Replace);
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, c INTEGER UNIQUE ON CONFLICT ABORT NOT NULL ON CONFLICT REPLACE)",
        tc.getSqlStatement("t")
        );
  tc.reset();

  // add a string column without default value
  tc.addCol("c", ColumnDataType::Text, ConflictClause::NotUsed, ConflictClause::Fail);
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, c TEXT NOT NULL ON CONFLICT FAIL)",
        tc.getSqlStatement("t")
        );
  tc.reset();

  // add a floating point column without default value
  tc.addCol("c", ColumnDataType::Float, ConflictClause::Ignore, ConflictClause::NotUsed);
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, c REAL UNIQUE ON CONFLICT IGNORE)",
        tc.getSqlStatement("t")
        );
  tc.reset();

  // add a blob column without default value
  tc.addCol("c", ColumnDataType::Blob, ConflictClause::NotUsed, ConflictClause::NotUsed);
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, c BLOB )",
        tc.getSqlStatement("t")
        );
  tc.reset();

  // add an integer column with a default value
  tc.addCol("c", ColumnDataType::Integer, ConflictClause::Abort, ConflictClause::Replace, -5);
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, c INTEGER UNIQUE ON CONFLICT ABORT NOT NULL ON CONFLICT REPLACE DEFAULT -5)",
        tc.getSqlStatement("t")
        );
  tc.reset();

  // add a string column with a default value
  tc.addCol("c", ColumnDataType::Text, ConflictClause::NotUsed, ConflictClause::Fail, "hello");
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, c TEXT NOT NULL ON CONFLICT FAIL DEFAULT 'hello')",
        tc.getSqlStatement("t")
        );
  tc.reset();

  // foreign key
  tc.addForeignKey("refCol", "refTab", ConsistencyAction::Cascade, ConsistencyAction::SetNull, ConflictClause::NotUsed, ConflictClause::Fail, "targetCol");
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, refCol INTEGER NOT NULL ON CONFLICT FAIL, FOREIGN KEY (refCol) REFERENCES refTab(targetCol) ON DELETE CASCADE ON UPDATE SET NULL)",
        tc.getSqlStatement("t")
        );
  tc.reset();
  tc.addForeignKey("refCol", "refTab", ConsistencyAction::Restrict, ConsistencyAction::NoAction, ConflictClause::Rollback, ConflictClause::Abort);
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, refCol INTEGER UNIQUE ON CONFLICT ROLLBACK NOT NULL ON CONFLICT ABORT, FOREIGN KEY (refCol) REFERENCES refTab(id) ON DELETE RESTRICT ON UPDATE NO ACTION)",
        tc.getSqlStatement("t")
        );
  tc.reset();
}

//----------------------------------------------------------------

TEST(TableCreatorTests, ExecTableCreation)
{
  TableCreator tc;
  SqliteDatabase memDb;

  tc.addCol("i", ColumnDataType::Integer, ConflictClause::NotUsed, ConflictClause::NotUsed);
  DbTab parentTab = tc.createTableAndResetCreator(memDb, "t1");
  ASSERT_TRUE(memDb.hasTable("t1"));

  ColumnValueClause cvc;
  cvc.addCol("i", 42);
  ASSERT_EQ(1, parentTab.insertRow(cvc));

  tc.addForeignKey("refCol", "t1", ConsistencyAction::Cascade, ConsistencyAction::Cascade, ConflictClause::NotUsed, ConflictClause::NotUsed);
  DbTab childTab = tc.createTableAndResetCreator(memDb, "t2");
  ASSERT_TRUE(memDb.hasTable("t2"));

  cvc.clear();
  cvc.addCol("refCol", 1);
  ASSERT_EQ(1, childTab.insertRow(cvc));
  cvc.clear();
  cvc.addCol("refCol", 10);
  ASSERT_THROW(childTab.insertRow(cvc), ConstraintFailedException);
}

//----------------------------------------------------------------

TEST(TableCreatorTests, UniqueColumnCombinations)
{
  TableCreator tc;
  SqliteDatabase memDb;

  tc.addCol("a", ColumnDataType::Integer, ConflictClause::NotUsed, ConflictClause::NotUsed);
  tc.addCol("b", ColumnDataType::Integer, ConflictClause::NotUsed, ConflictClause::NotUsed);
  tc.addCol("c", ColumnDataType::Integer, ConflictClause::NotUsed, ConflictClause::NotUsed);
  tc.addUniqueColumnCombination({"a", "b"}, ConflictClause::Fail);
  ASSERT_EQ(
        "CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, a INTEGER ,b INTEGER ,c INTEGER , UNIQUE(a,b) ON CONFLICT FAIL)",
        tc.getSqlStatement("t")
        );

  DbTab t1 = tc.createTableAndResetCreator(memDb, "t1");
  ASSERT_TRUE(memDb.hasTable("t1"));

  ColumnValueClause cvc;
  cvc.addCol("a", 10);
  cvc.addCol("b", 20);
  cvc.addCol("b", 30);
  ASSERT_EQ(1, t1.insertRow(cvc));
  cvc.clear();

  cvc.addCol("a", 10);
  cvc.addCol("b", 21);
  cvc.addCol("b", 30);
  ASSERT_EQ(2, t1.insertRow(cvc));
  cvc.clear();

  cvc.addCol("a", 11);
  cvc.addCol("b", 20);
  cvc.addCol("b", 30);
  ASSERT_EQ(3, t1.insertRow(cvc));
  cvc.clear();

  cvc.addCol("a", 10);
  cvc.addCol("b", 20);
  cvc.addCol("b", 30);
  ASSERT_THROW(t1.insertRow(cvc), ConstraintFailedException);
}
