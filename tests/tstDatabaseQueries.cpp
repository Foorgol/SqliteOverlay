#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, QueryInt)
{
  SampleDB db = getScenario01();

  // first version: SQL statement as string
  string sql = "SELECT COUNT(*) FROM t1 WHERE rowid > 0";
  ASSERT_EQ(5, db.execScalarQuery<int>(sql));
  auto opt = db.execScalarQuery2<int>(sql);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(5, opt.value());

  // second version: SQL statement as statement
  SqlStatement stmt = db.prepStatement(sql);
  ASSERT_EQ(5, db.execScalarQuery<int>(stmt));
  stmt = db.prepStatement(sql);
  opt = db.execScalarQuery2<int>(stmt);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(5, opt.value());

  // special case: query returning NULL column value
  sql = "SELECT i FROM t1 WHERE rowid=2";
  opt = db.execScalarQuery2<int>(sql);
  ASSERT_FALSE(opt.has_value());
  ASSERT_THROW(db.execScalarQuery<int>(sql), NullValueException);

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE rowid=9999";
  ASSERT_THROW(db.execScalarQuery2<int>(sql), NoDataException);
  ASSERT_THROW(db.execScalarQuery<int>(sql), NoDataException);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ContentQuery)
{
  SampleDB db = getScenario01();

  // SQL statement as string
  string sql = "SELECT rowid,* FROM t1 WHERE rowid > 0";
  SqlStatement stmt = db.execContentQuery(sql);
  int rowCount = 0;
  while (stmt.hasData())
  {
    ++rowCount;
    int val;
    val = stmt.get<int>(0);  // get the row ID
    ASSERT_EQ(rowCount, val);
    stmt.step();
  }
  ASSERT_EQ(5, rowCount);

  // the same, but using "dataStep()"
  stmt = db.prepStatement(sql);
  rowCount = 0;
  while (stmt.dataStep())
  {
    ++rowCount;
    int val;
    val = stmt.get<int>(0);  // get the row ID
    ASSERT_EQ(rowCount, val);
  }
  ASSERT_EQ(5, rowCount);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryDouble)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  string sql = "SELECT f FROM t1 WHERE rowid = 2";
  ASSERT_EQ(666.66, db.execScalarQuery<double>(sql));
  auto opt = db.execScalarQuery2<double>(sql);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(666.66, opt.value());

  // second version: SQL statement as statement
  SqlStatement stmt = db.prepStatement(sql);
  ASSERT_EQ(666.66, db.execScalarQuery<double>(stmt));
  stmt = db.prepStatement(sql);
  opt = db.execScalarQuery2<double>(stmt);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(666.66, opt.value());

  // special case: query returning NULL column value
  sql = "SELECT f FROM t1 WHERE rowid=3";
  opt = db.execScalarQuery2<double>(sql);
  ASSERT_FALSE(opt.has_value());
  ASSERT_THROW(db.execScalarQuery<double>(sql), NullValueException);

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE rowid=9999";
  ASSERT_THROW(db.execScalarQuery2<double>(sql), NoDataException);
  ASSERT_THROW(db.execScalarQuery<double>(sql), NoDataException);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryString)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  string result = "Ho";
  string sql = "SELECT s FROM t1 WHERE rowid = 5";
  ASSERT_EQ(result, db.execScalarQuery<std::string>(sql));
  auto opt = db.execScalarQuery2<std::string>(sql);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(result, opt.value());

  // second version: SQL statement as statement
  SqlStatement stmt = db.prepStatement(sql);
  ASSERT_EQ(result, db.execScalarQuery<std::string>(stmt));
  stmt = db.prepStatement(sql);
  opt = db.execScalarQuery2<std::string>(stmt);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(result, opt.value());

  // special case: query returning no results
  sql = "SELECT s FROM t1 WHERE rowid=9999";
  ASSERT_THROW(db.execScalarQuery2<std::string>(sql), NoDataException);
  ASSERT_THROW(db.execScalarQuery<std::string>(sql), NoDataException);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryStringUTF8)
{
  auto db = getScenario01();

  string sql = "SELECT s FROM t1 WHERE rowid = 3";
  ASSERT_EQ(u8"äöüÄÖÜ", db.execScalarQuery<std::string>(sql));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryAllTableAndViewNames)
{
  auto db = getScenario01();

  Sloppy::StringList names = db.allTableNames();
  ASSERT_EQ(2, names.size());
  ASSERT_FALSE(names.at(0) == names.at(1));
  ASSERT_TRUE((names.at(0) == "t1") || (names.at(0) == "t2"));
  ASSERT_TRUE((names.at(1) == "t1") || (names.at(1) == "t2"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryLong)
{
  SampleDB db = getScenario01();

  // write a long value to the database
  // note: `bind` with longs is tested elsewhere; thus we can
  // rely on it here
  auto stmt = db.prepStatement("UPDATE t1 SET i = ? WHERE rowid=1");
  stmt.bind(1, LONG_MAX);
  ASSERT_TRUE(stmt.step());

  // first version: SQL statement as string
  string sql = "SELECT i FROM t1 WHERE rowid=1";
  ASSERT_EQ(LONG_MAX, db.execScalarQuery<int64_t>(sql));
  auto opt = db.execScalarQuery2<int64_t>(sql);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(LONG_MAX, opt.value());

  // second version: SQL statement as statement
  stmt = db.prepStatement(sql);
  ASSERT_EQ(LONG_MAX, db.execScalarQuery<int64_t>(stmt));
  stmt = db.prepStatement(sql);
  opt = db.execScalarQuery2<int64_t>(stmt);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(LONG_MAX, opt.value());

  // special case: query returning NULL column value
  sql = "SELECT i FROM t1 WHERE rowid=2";
  opt = db.execScalarQuery2<int64_t>(sql);
  ASSERT_FALSE(opt.has_value());
  ASSERT_THROW(db.execScalarQuery<int64_t>(sql), NullValueException);

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE rowid=9999";
  ASSERT_THROW(db.execScalarQuery2<int64_t>(sql), NoDataException);
  ASSERT_THROW(db.execScalarQuery<int64_t>(sql), NoDataException);
}

