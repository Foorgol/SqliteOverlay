#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "ClausesAndQueries.h"

using namespace SqliteOverlay;
/*
TEST_F(DatabaseTestScenario, ColumnValueClause_Empty)
{
  ColumnValueClause cvc;

  ASSERT_FALSE(cvc.hasColumns());

  ASSERT_EQ("INSERT INTO tab DEFAULT VALUES", cvc.getInsertStmt("tab"));
  ASSERT_EQ("", cvc.getUpdateStmt("tab", 42));

  // invalid tab names
  ASSERT_EQ("", cvc.getInsertStmt(""));
  ASSERT_EQ("", cvc.getUpdateStmt("", 42));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_IntCol)
{
  ColumnValueClause cvc;
  cvc.addIntCol("col", 23);

  ASSERT_TRUE(cvc.hasColumns());

  ASSERT_EQ("INSERT INTO tab (col) VALUES (23)", cvc.getInsertStmt("tab"));
  ASSERT_EQ("UPDATE tab SET col=23 WHERE id=42", cvc.getUpdateStmt("tab", 42));

  // invalid tab names
  ASSERT_EQ("", cvc.getInsertStmt(""));
  ASSERT_EQ("", cvc.getUpdateStmt("", 42));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_DoubleCol)
{
  ColumnValueClause cvc;
  cvc.addDoubleCol("col", 23.666);

  ASSERT_TRUE(cvc.hasColumns());

  ASSERT_EQ("INSERT INTO tab (col) VALUES (23.666000)", cvc.getInsertStmt("tab"));
  ASSERT_EQ("UPDATE tab SET col=23.666000 WHERE id=42", cvc.getUpdateStmt("tab", 42));

  // invalid tab names
  ASSERT_EQ("", cvc.getInsertStmt(""));
  ASSERT_EQ("", cvc.getUpdateStmt("", 42));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_StringCol)
{
  ColumnValueClause cvc;
  cvc.addStringCol("col", "xyz");

  ASSERT_TRUE(cvc.hasColumns());

  ASSERT_EQ("INSERT INTO tab (col) VALUES (\"xyz\")", cvc.getInsertStmt("tab"));
  ASSERT_EQ("UPDATE tab SET col=\"xyz\" WHERE id=42", cvc.getUpdateStmt("tab", 42));

  // invalid tab names
  ASSERT_EQ("", cvc.getInsertStmt(""));
  ASSERT_EQ("", cvc.getUpdateStmt("", 42));
}


//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_DateCol)
{
  ColumnValueClause cvc;
  boost::gregorian::date d{2016, 8, 7};

  cvc.addDateCol("col", d);

  ASSERT_TRUE(cvc.hasColumns());

  ASSERT_EQ("INSERT INTO tab (col) VALUES (20160807)", cvc.getInsertStmt("tab"));
  ASSERT_EQ("UPDATE tab SET col=20160807 WHERE id=42", cvc.getUpdateStmt("tab", 42));

  // invalid tab names
  ASSERT_EQ("", cvc.getInsertStmt(""));
  ASSERT_EQ("", cvc.getUpdateStmt("", 42));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_NullCol)
{
  ColumnValueClause cvc;
  cvc.addNullCol("col");

  ASSERT_TRUE(cvc.hasColumns());

  ASSERT_EQ("INSERT INTO tab (col) VALUES (NULL)", cvc.getInsertStmt("tab"));
  ASSERT_EQ("UPDATE tab SET col=NULL WHERE id=42", cvc.getUpdateStmt("tab", 42));

  // invalid tab names
  ASSERT_EQ("", cvc.getInsertStmt(""));
  ASSERT_EQ("", cvc.getUpdateStmt("", 42));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ColumnValueClause_MultipleCol)
{
  ColumnValueClause cvc;
  cvc.addIntCol("col1", 23);
  cvc.addDoubleCol("col2", 23.666);
  cvc.addStringCol("col3", "xyz");
  cvc.addNullCol("col4");

  ASSERT_TRUE(cvc.hasColumns());

  ASSERT_EQ("INSERT INTO tab (col1, col2, col3, col4) VALUES (23, 23.666000, \"xyz\", NULL)", cvc.getInsertStmt("tab"));
  ASSERT_EQ("UPDATE tab SET col1=23, col2=23.666000, col3=\"xyz\", col4=NULL WHERE id=42", cvc.getUpdateStmt("tab", 42));

  // invalid tab names
  ASSERT_EQ("", cvc.getInsertStmt(""));
  ASSERT_EQ("", cvc.getUpdateStmt("", 42));
}

//----------------------------------------------------------------

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
