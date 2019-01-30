#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "DatabaseTestScenario.h"
#include "ClausesAndQueries.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, ColumnValueClause_Empty)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;

  ASSERT_FALSE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 DEFAULT VALUES", stmt.getExpandedSQL());
  ASSERT_THROW(cvc.getUpdateStmt(db, "t1", 42), std::invalid_argument);

  // invalid tab names / empty values
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name

  ASSERT_THROW(cvc.getUpdateStmt(db, "t1", 42), std::invalid_argument);  // empty column values
  cvc.addCol("i", 42);  // otherwise the exceptions for empty/invalid tables names are not triggered
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_IntCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;

  cvc.addCol("i", 23);

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (i) VALUES (23)", stmt.getExpandedSQL());

  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET i=23 WHERE rowid=42", stmt.getExpandedSQL());

  // invalid table names
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_LongCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;

  cvc.addCol("i", LONG_MAX);

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (i) VALUES (9223372036854775807)", stmt.getExpandedSQL());

  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET i=9223372036854775807 WHERE rowid=42", stmt.getExpandedSQL());

  // invalid table names
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_DoubleCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;

  cvc.addCol("i", 23.666);

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (i) VALUES (23.666)", stmt.getExpandedSQL());

  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET i=23.666 WHERE rowid=42", stmt.getExpandedSQL());

  // invalid table names
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_StringCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;

  cvc.addCol("s", "xyz");

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (s) VALUES ('xyz')", stmt.getExpandedSQL());

  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET s='xyz' WHERE rowid=42", stmt.getExpandedSQL());

  // invalid table names
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}


//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_DateCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;
  boost::gregorian::date d{2016, 8, 7};

  cvc.addCol("i", d);

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (i) VALUES (20160807)", stmt.getExpandedSQL());

  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET i=20160807 WHERE rowid=42", stmt.getExpandedSQL());

  // invalid table names
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_TimestampCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;
  UTCTimestamp ts{2019,01,30,19,52,0};

  cvc.addCol("i", &ts);

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (i) VALUES (1548877920)", stmt.getExpandedSQL());

  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET i=1548877920 WHERE rowid=42", stmt.getExpandedSQL());

  // invalid table names
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_NullCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;
  cvc.addNullCol("i");

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (i) VALUES (NULL)", stmt.getExpandedSQL());
  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET i=NULL WHERE rowid=42", stmt.getExpandedSQL());

  // invalid tab names
  ASSERT_THROW(cvc.getInsertStmt(db, ""), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getInsertStmt(db, "NonExistingTable"), SqlStatementCreationError);  // wrong table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "", 42), std::invalid_argument);  // empty table name
  ASSERT_THROW(cvc.getUpdateStmt(db, "NonExistingTable", 42), SqlStatementCreationError);  // wrong table name
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_MultipleCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;
  cvc.addCol("i", 23);
  cvc.addCol("f", 23.666);
  cvc.addCol("s", "xyz");
  cvc.addNullCol("d");

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (i,f,s,d) VALUES (23,23.666,'xyz',NULL)", stmt.getExpandedSQL());
  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET i=23,f=23.666,s='xyz',d=NULL WHERE rowid=42", stmt.getExpandedSQL());

  // test the "clear" command
  cvc.clear();
  stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 DEFAULT VALUES", stmt.getExpandedSQL());
  ASSERT_THROW(cvc.getUpdateStmt(db, "t1", 42), std::invalid_argument);

}

//----------------------------------------------------------------
/*
TEST_F(DatabaseTestScenario, WhereClause_Empty)
{
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  ASSERT_EQ("", w.getSelectStmt("tab", false));
  ASSERT_EQ("", w.getSelectStmt("tab", true));
  ASSERT_EQ("", w.getDeleteStmt("tab"));

  // invalid tab names
  ASSERT_EQ("", w.getSelectStmt("", false));
  ASSERT_EQ("", w.getDeleteStmt(""));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_IntCol)
{
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addIntCol("col", 23);

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col=23", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col=23", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col=23", w.getDeleteStmt("tab"));

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addIntCol("col", ">", 23);

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col>23", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col>23", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col>23", w.getDeleteStmt("tab"));

  // invalid tab names
  ASSERT_EQ("", w.getSelectStmt("", false));
  ASSERT_EQ("", w.getDeleteStmt(""));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_DoubleCol)
{
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addDoubleCol("col", 23.666);

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col=23.666000", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col=23.666000", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col=23.666000", w.getDeleteStmt("tab"));

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addDoubleCol("col", "<", 23.666);

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col<23.666000", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col<23.666000", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col<23.666000", w.getDeleteStmt("tab"));

  // invalid tab names
  ASSERT_EQ("", w.getSelectStmt("", false));
  ASSERT_EQ("", w.getDeleteStmt(""));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_StringCol)
{
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addStringCol("col", "xyz");

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col=\"xyz\"", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col=\"xyz\"", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col=\"xyz\"", w.getDeleteStmt("tab"));

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addStringCol("col", "<", "xyz");

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col<\"xyz\"", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col<\"xyz\"", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col<\"xyz\"", w.getDeleteStmt("tab"));

  // invalid tab names
  ASSERT_EQ("", w.getSelectStmt("", false));
  ASSERT_EQ("", w.getDeleteStmt(""));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_DateCol)
{
  WhereClause w;
  boost::gregorian::date d{2016, 8, 7};

  ASSERT_TRUE(w.isEmpty());

  w.addDateCol("col", d);

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col=20160807", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col=20160807", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col=20160807", w.getDeleteStmt("tab"));

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addDateCol("col", ">", d);

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col>20160807", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col>20160807", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col>20160807", w.getDeleteStmt("tab"));

  // invalid tab names
  ASSERT_EQ("", w.getSelectStmt("", false));
  ASSERT_EQ("", w.getDeleteStmt(""));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_NullCol)
{
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addNullCol("col");

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col IS NULL", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col IS NULL", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col IS NULL", w.getDeleteStmt("tab"));

  w.clear();
  ASSERT_TRUE(w.isEmpty());

  // invalid tab names
  ASSERT_EQ("", w.getSelectStmt("", false));
  ASSERT_EQ("", w.getDeleteStmt(""));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_MultipleCol)
{
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addIntCol("col1", 23);
  w.addDoubleCol("col2", 23.666);
  w.addStringCol("col3", "xyz");
  w.addNullCol("col4");

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col1=23 AND col2=23.666000 AND col3=\"xyz\" AND col4 IS NULL", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col1=23 AND col2=23.666000 AND col3=\"xyz\" AND col4 IS NULL", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col1=23 AND col2=23.666000 AND col3=\"xyz\" AND col4 IS NULL", w.getDeleteStmt("tab"));

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addIntCol("col1", ">", 23);
  w.addDoubleCol("col2", "<", 23.666);
  w.addStringCol("col3", "<>", "xyz");
  w.addNullCol("col4");

  ASSERT_FALSE(w.isEmpty());
  ASSERT_EQ("SELECT id FROM tab WHERE col1>23 AND col2<23.666000 AND col3<>\"xyz\" AND col4 IS NULL", w.getSelectStmt("tab", false));
  ASSERT_EQ("SELECT COUNT(*) FROM tab WHERE col1>23 AND col2<23.666000 AND col3<>\"xyz\" AND col4 IS NULL", w.getSelectStmt("tab", true));
  ASSERT_EQ("DELETE FROM tab WHERE col1>23 AND col2<23.666000 AND col3<>\"xyz\" AND col4 IS NULL", w.getDeleteStmt("tab"));

  // invalid tab names
  ASSERT_EQ("", w.getSelectStmt("", false));
  ASSERT_EQ("", w.getDeleteStmt(""));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ScalarQueryResult_Null)
{
  ScalarQueryResult<int> sqr;

  ASSERT_TRUE(sqr.isNull());
  ASSERT_THROW(sqr.get(), std::invalid_argument);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ScalarQueryResult_Value)
{
  ScalarQueryResult<int> sqr(42);

  ASSERT_FALSE(sqr.isNull());
  ASSERT_EQ(42, sqr.get());
}
*/
