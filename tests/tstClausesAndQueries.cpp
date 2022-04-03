#include <gtest/gtest.h>

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

TEST_F(DatabaseTestScenario, ColumnValueClause_JsonCol)
{
  SampleDB db = getScenario01();
  ColumnValueClause cvc;
  nlohmann::json jsonIn = nlohmann::json::parse(R"({"a": "abc", "b": 42})");

  cvc.addCol("s", jsonIn);

  ASSERT_TRUE(cvc.hasColumns());

  auto stmt = cvc.getInsertStmt(db, "t1");
  ASSERT_EQ("INSERT INTO t1 (s) VALUES ('{\"a\":\"abc\",\"b\":42}')", stmt.getExpandedSQL());

  stmt = cvc.getUpdateStmt(db, "t1", 42);
  ASSERT_EQ("UPDATE t1 SET s='{\"a\":\"abc\",\"b\":42}' WHERE rowid=42", stmt.getExpandedSQL());

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
  const date::year_month_day d{date::year{2016} /8 / 7};

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
  using namespace std::chrono_literals;

  SampleDB db = getScenario01();
  ColumnValueClause cvc;
  Sloppy::DateTime::WallClockTimepoint_secs ts{date::year{2019} / 01 / 30, 19h, 52min, 0s};

  cvc.addCol("i", ts);

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

TEST_F(DatabaseTestScenario, WhereClause_Empty)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  // SELECT without column values
  ASSERT_THROW(w.getSelectStmt(db, "", false), std::invalid_argument);
  ASSERT_THROW(w.getSelectStmt(db, "t1", false), std::invalid_argument);
  auto stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1", stmt.getExpandedSQL());
  ASSERT_THROW(w.getSelectStmt(db, "wrongname", true), SqlStatementCreationError);

  // DELETE without column values
  ASSERT_THROW(w.getDeleteStmt(db, ""), std::invalid_argument);
  ASSERT_THROW(w.getDeleteStmt(db, "t1"), std::invalid_argument);
  ASSERT_THROW(w.getDeleteStmt(db, "wrongname"), std::invalid_argument);

  // DELETE with invalid table name
  w.addCol("i", 99);
  ASSERT_THROW(w.getDeleteStmt(db, ""), std::invalid_argument);
  ASSERT_THROW(w.getDeleteStmt(db, "wrongname"), SqlStatementCreationError);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_IntCol)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addCol("i", 23);

  ASSERT_FALSE(w.isEmpty());

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i=23", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i=23", stmt.getExpandedSQL());

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addCol("i", ">", 23);
  ASSERT_FALSE(w.isEmpty());

  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i>23", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i>23", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i>23", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_LongCol)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addCol("i", LONG_MIN);

  ASSERT_FALSE(w.isEmpty());

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=-9223372036854775808", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i=-9223372036854775808", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i=-9223372036854775808", stmt.getExpandedSQL());

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addCol("i", ">", LONG_MIN);
  ASSERT_FALSE(w.isEmpty());

  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i>-9223372036854775808", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i>-9223372036854775808", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i>-9223372036854775808", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_DoubleCol)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addCol("f", 23.666);

  ASSERT_FALSE(w.isEmpty());

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE f=23.666", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE f=23.666", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE f=23.666", stmt.getExpandedSQL());

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addCol("f", ">", 23.666);
  ASSERT_FALSE(w.isEmpty());

  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE f>23.666", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE f>23.666", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE f>23.666", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_StringCol)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addCol("s", "xyz");

  ASSERT_FALSE(w.isEmpty());

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE s='xyz'", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE s='xyz'", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE s='xyz'", stmt.getExpandedSQL());

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addCol("s", ">", std::string_view{"xyz"});
  ASSERT_FALSE(w.isEmpty());

  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE s>'xyz'", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE s>'xyz'", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE s>'xyz'", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_DateCol)
{
  SampleDB db = getScenario01();
  WhereClause w;
  date::year_month_day d{date::year{2016} / 8 / 7};

  ASSERT_TRUE(w.isEmpty());

  w.addCol("d", d);
  ASSERT_FALSE(w.isEmpty());

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE d=20160807", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE d=20160807", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE d=20160807", stmt.getExpandedSQL());

  w.clear();
  ASSERT_TRUE(w.isEmpty());


  w.addCol("d", ">", d);
  ASSERT_FALSE(w.isEmpty());

  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE d>20160807", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE d>20160807", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE d>20160807", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_NullCol)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addNullCol("i");
  ASSERT_FALSE(w.isEmpty());

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i IS NULL", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i IS NULL", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i IS NULL", stmt.getExpandedSQL());

  w.clear();
  ASSERT_TRUE(w.isEmpty());

  w.addNotNullCol("i");
  ASSERT_FALSE(w.isEmpty());

  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i IS NOT NULL", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i IS NOT NULL", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i IS NOT NULL", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_MultipleCol)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addCol("i", 23);
  w.addCol("f", 23.666);
  w.addCol("s", "xyz");
  w.addNullCol("d");
  ASSERT_FALSE(w.isEmpty());

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23 AND f=23.666 AND s='xyz' AND d IS NULL", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i=23 AND f=23.666 AND s='xyz' AND d IS NULL", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i=23 AND f=23.666 AND s='xyz' AND d IS NULL", stmt.getExpandedSQL());

  w.clear();
  ASSERT_TRUE(w.isEmpty());

  w.addCol("i", ">", 23);
  w.addCol("f", "<", 23.666);
  w.addCol("s", "<>", std::string_view{"xyz"});
  w.addNotNullCol("d");
  ASSERT_FALSE(w.isEmpty());

  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i>23 AND f<23.666 AND s<>'xyz' AND d IS NOT NULL", stmt.getExpandedSQL());
  stmt = w.getSelectStmt(db, "t1", true);
  ASSERT_EQ("SELECT COUNT(*) FROM t1 WHERE i>23 AND f<23.666 AND s<>'xyz' AND d IS NOT NULL", stmt.getExpandedSQL());
  stmt = w.getDeleteStmt(db, "t1");
  ASSERT_EQ("DELETE FROM t1 WHERE i>23 AND f<23.666 AND s<>'xyz' AND d IS NOT NULL", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, WhereClause_OrderAndLimit)
{
  SampleDB db = getScenario01();
  WhereClause w;
  ASSERT_TRUE(w.isEmpty());

  w.addCol("i", 23);
  w.addCol("f", ">", 23.666);
  ASSERT_FALSE(w.isEmpty());

  w.setLimit(10);

  auto stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23 AND f>23.666 LIMIT 10", stmt.getExpandedSQL());

  w.clear();
  w.addCol("i", 23);
  w.addCol("f", ">", 23.666);
  w.setOrderColumn_Asc("d");
  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23 AND f>23.666 ORDER BY d ASC", stmt.getExpandedSQL());
  w.setOrderColumn_Asc("i,f");
  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23 AND f>23.666 ORDER BY d ASC, i,f ASC", stmt.getExpandedSQL());

  w.clear();
  w.addCol("i", 23);
  w.addCol("f", ">", 23.666);
  w.setOrderColumn_Desc("d");
  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23 AND f>23.666 ORDER BY d DESC", stmt.getExpandedSQL());
  w.setOrderColumn_Desc("i,f");
  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23 AND f>23.666 ORDER BY d DESC, i,f DESC", stmt.getExpandedSQL());

  w.setLimit(42);
  stmt = w.getSelectStmt(db, "t1", false);
  ASSERT_EQ("SELECT rowid FROM t1 WHERE i=23 AND f>23.666 ORDER BY d DESC, i,f DESC LIMIT 42", stmt.getExpandedSQL());
}
