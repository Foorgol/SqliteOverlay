#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "SqlStatement.h"

using namespace SqliteOverlay;


TEST_F(DatabaseTestScenario, StmtBind)
{
  prepScenario01();
  auto db = getRawDbHandle();

  // prep a dummy SQL query
  string sql{"SELECT * FROM t1 WHERE s=?"};

  // try an int bind
  SqlStatement stmt{db.get(), sql};
  stmt.bind(1, 42);
  ASSERT_EQ("SELECT * FROM t1 WHERE s=42", stmt.getExpandedSQL());

  // try a string
  stmt = SqlStatement{db.get(), sql};  // implicit test of "move assignment"
  stmt.bind(1, string{"abc"});
  ASSERT_EQ("SELECT * FROM t1 WHERE s='abc'", stmt.getExpandedSQL());

  // try a double
  stmt.reset(true);   // test of "reset()"
  stmt.bind(1, 42.42);
  ASSERT_EQ("SELECT * FROM t1 WHERE s=42.42", stmt.getExpandedSQL());

  // try a bool
  stmt.reset(true);
  stmt.bind(1, true);
  ASSERT_EQ("SELECT * FROM t1 WHERE s=1", stmt.getExpandedSQL());
  stmt = SqlStatement{db.get(), sql};
  stmt.bind(1, false);
  ASSERT_EQ("SELECT * FROM t1 WHERE s=0", stmt.getExpandedSQL());

  // try a char*
  stmt.reset(true);
  stmt.bind(1, "xyz");
  ASSERT_EQ("SELECT * FROM t1 WHERE s='xyz'", stmt.getExpandedSQL());

  // try two ways to achive "NULL"
  stmt.reset(true);
  ASSERT_EQ("SELECT * FROM t1 WHERE s=NULL", stmt.getExpandedSQL());
  stmt.reset(true);
  stmt.bindNull(1);
  ASSERT_EQ("SELECT * FROM t1 WHERE s=NULL", stmt.getExpandedSQL());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtStep)
{
  prepScenario01();
  auto db = getRawDbHandle();

  // prep and execute a SQL query that never returns any values
  SqlStatement stmt{db.get(), "INSERT INTO t1 (s) VALUES ('a')"};
  ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(stmt.isDone());
  ASSERT_FALSE(stmt.hasData());
  ASSERT_THROW(stmt.getInt(0), NoDataException);
  ASSERT_FALSE(stmt.step());
  ASSERT_FALSE(stmt.step());   // still `false` because we're done

  // prep and execute a SQL query that returns a single value
  stmt = SqlStatement{db.get(), "SELECT COUNT(*) FROM t1"};
  ASSERT_TRUE(stmt.step());   // yields the scalar result
  ASSERT_FALSE(stmt.isDone());
  ASSERT_TRUE(stmt.hasData());
  ASSERT_TRUE(stmt.getInt(0) > 1);
  ASSERT_FALSE(stmt.step());   // "no more data"
  ASSERT_TRUE(stmt.isDone());
  ASSERT_THROW(stmt.getInt(0), NoDataException);
  ASSERT_FALSE(stmt.step());   // still `false` because we're done

  // prep and execute a SQL query that could return
  // data but doesn't in this case
  stmt = SqlStatement{db.get(), "SELECT * FROM t1 WHERE i=123456"};
  ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(stmt.isDone());
  ASSERT_FALSE(stmt.hasData());
  ASSERT_THROW(stmt.getInt(0), NoDataException);
  ASSERT_FALSE(stmt.step());   // "no more data"

  // prep and execute a SQL query that returns multiple rows and columns
  stmt = SqlStatement{db.get(), "SELECT * FROM t1"};
  ASSERT_TRUE(stmt.step());
  ASSERT_FALSE(stmt.isDone());
  ASSERT_TRUE(stmt.hasData());
  ASSERT_NO_THROW(stmt.getInt(0));
  ASSERT_NO_THROW(stmt.getInt(1));
  ASSERT_TRUE(stmt.step());
  ASSERT_FALSE(stmt.isDone());
  ASSERT_TRUE(stmt.hasData());
  ASSERT_NO_THROW(stmt.getInt(0));
  ASSERT_NO_THROW(stmt.getInt(1));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtDataStep)
{
  prepScenario01();
  auto db = getRawDbHandle();

  // prep and execute a SQL query that never returns any values
  SqlStatement stmt{db.get(), "INSERT INTO t1 (s) VALUES ('a')"};
  ASSERT_FALSE(stmt.dataStep());   // successful execution but no data rows

  // prep and execute a SQL query that returns a single value
  stmt = SqlStatement{db.get(), "SELECT COUNT(*) FROM t1"};
  ASSERT_TRUE(stmt.dataStep());   // yields the scalar result
  ASSERT_FALSE(stmt.dataStep());   // "no more data"

  // prep and execute a SQL query that could return
  // data but doesn't in this case
  stmt = SqlStatement{db.get(), "SELECT * FROM t1 WHERE i=123456"};
  ASSERT_FALSE(stmt.dataStep());   // "no more data"

  // prep and execute a SQL query that returns multiple rows and columns
  stmt = SqlStatement{db.get(), "SELECT * FROM t1"};
  ASSERT_TRUE(stmt.dataStep());
  ASSERT_TRUE(stmt.dataStep());
  while (stmt.dataStep()) {};
  ASSERT_FALSE(stmt.hasData());
  ASSERT_TRUE(stmt.isDone());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtGetters)
{
  prepScenario01();
  auto db = getRawDbHandle();

  SqlStatement stmt{db.get(), "SELECT id, i, f, s FROM t1 WHERE id=1"};
  ASSERT_TRUE(stmt.dataStep());

  // test all getters
  ASSERT_EQ(1, stmt.getInt(0)); // ID column
  ASSERT_EQ(42, stmt.getInt(1)); // i column
  ASSERT_EQ(23.23, stmt.getDouble(2));
  ASSERT_EQ("Hallo", stmt.getString(3));
  ASSERT_TRUE(stmt.getBool(1));
  for (int i=0; i < 4; ++i) ASSERT_FALSE(stmt.isNull(i));

  // test invalid columns
  ASSERT_THROW(stmt.getBool(42), InvalidColumnException);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtColTypeAndName)
{
  prepScenario01();
  auto db = getRawDbHandle();
  SqlStatement stmt{db.get(), "SELECT id, i, f, s, d FROM t1 WHERE id=1"};
  ASSERT_TRUE(stmt.dataStep());

  // test all getters
  ASSERT_EQ(ColumnDataType::Integer, stmt.getColType(0));
  ASSERT_EQ(ColumnDataType::Integer, stmt.getColType(1));
  ASSERT_EQ(ColumnDataType::Float, stmt.getColType(2));
  ASSERT_EQ(ColumnDataType::Text, stmt.getColType(3));
  ASSERT_EQ(ColumnDataType::Text, stmt.getColType(4));

  // test invalid columns
  ASSERT_THROW(stmt.getColType(42), InvalidColumnException);

  // test NULL columns
  stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE id=2"};
  ASSERT_TRUE(stmt.dataStep());
  ASSERT_EQ(ColumnDataType::Null, stmt.getColType(0));
  ASSERT_TRUE(stmt.isNull(0));
}

