#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
//#include "ClausesAndQueries.h"

using namespace SqliteOverlay;
namespace bfs = boost::filesystem;

TEST_F(DatabaseTestScenario, QueryInt)
{
  SampleDB db = getScenario01();

  // first version: SQL statement as string
  string sql = "SELECT COUNT(*) FROM t1 WHERE id > 0";
  ASSERT_EQ(5, db.execScalarQueryInt(sql));
  auto opt = db.execScalarQueryIntOrNull(sql);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(5, opt.value());

  // second version: SQL statement as statement
  SqlStatement stmt = db.prepStatement(sql);
  ASSERT_EQ(5, db.execScalarQueryInt(stmt));
  stmt = db.prepStatement(sql);
  opt = db.execScalarQueryIntOrNull(stmt);
  ASSERT_TRUE(opt.has_value());
  ASSERT_EQ(5, opt.value());

  // special case: query returning NULL column value
  sql = "SELECT i FROM t1 WHERE id=2";
  opt = db.execScalarQueryIntOrNull(sql);
  ASSERT_FALSE(opt.has_value());
  ASSERT_THROW(db.execScalarQueryInt(sql), NullValueException);

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE id=9999";
  ASSERT_THROW(db.execScalarQueryIntOrNull(sql), NoDataException);
  ASSERT_THROW(db.execScalarQueryInt(sql), NoDataException);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ContentQuery)
{
  SampleDB db = getScenario01();

  // SQL statement as string
  string sql = "SELECT * FROM t1 WHERE id > 0";
  SqlStatement stmt = db.execContentQuery(sql);
  int rowCount = 0;
  while (stmt.hasData())
  {
    ++rowCount;
    int val;
    val = stmt.getInt(0);  // get the row ID
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
    val = stmt.getInt(0);  // get the row ID
    ASSERT_EQ(rowCount, val);
  }
  ASSERT_EQ(5, rowCount);
}

//----------------------------------------------------------------
/*
TEST_F(DatabaseTestScenario, QueryDouble)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  double result = -1;
  string sql = "SELECT f FROM t1 WHERE id = 2";
  ASSERT_TRUE(db->execScalarQueryDouble(sql, &result, nullptr));
  ASSERT_EQ(666.66, result);
  auto sqr = db->execScalarQueryDouble(sql, nullptr);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ(666.66, sqr->get());

  // second version: SQL statement as statement
  result = 0;
  auto stmt = db->prepStatement(sql, nullptr);
  ASSERT_TRUE(db->execScalarQueryDouble(stmt, &result, nullptr));
  ASSERT_EQ(666.66, result);
  stmt = db->prepStatement(sql, nullptr);
  sqr = db->execScalarQueryDouble(stmt, nullptr);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ(666.66, sqr->get());

  // special case: query returning NULL column value
  sql = "SELECT f FROM t1 WHERE id=3";
  sqr = db->execScalarQueryDouble(sql, nullptr);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_TRUE(sqr->isNull());
  ASSERT_THROW(sqr->get(), std::invalid_argument);

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE id=9999";
  sqr = db->execScalarQueryDouble(sql, nullptr);
  ASSERT_TRUE(sqr == nullptr);
  ASSERT_FALSE(db->execScalarQueryDouble(sql, &result, nullptr));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryString)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  string result = "xxx";
  string sql = "SELECT s FROM t1 WHERE id = 5";
  ASSERT_TRUE(db->execScalarQueryString(sql, &result, nullptr));
  ASSERT_EQ("Ho", result);
  auto sqr = db->execScalarQueryString(sql, nullptr);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ("Ho", sqr->get());

  // second version: SQL statement as statement
  result = "xxx";
  auto stmt = db->prepStatement(sql, nullptr);
  ASSERT_TRUE(db->execScalarQueryString(stmt, &result, nullptr));
  ASSERT_EQ("Ho", result);
  stmt = db->prepStatement(sql, nullptr);
  sqr = db->execScalarQueryString(stmt, nullptr);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ("Ho", sqr->get());

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE id=9999";
  sqr = db->execScalarQueryString(sql, nullptr);
  ASSERT_TRUE(sqr == nullptr);
  ASSERT_FALSE(db->execScalarQueryString(sql, &result, nullptr));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryStringUTF8)
{
  auto db = getScenario01();

  string result = "xxx";
  string sql = "SELECT s FROM t1 WHERE id = 3";
  ASSERT_TRUE(db->execScalarQueryString(sql, &result, nullptr));
  ASSERT_EQ(u8"äöüÄÖÜ", result);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryAllTableAndViewNames)
{
  auto db = getScenario01();

  Sloppy::StringList names = db->allTableNames();
  ASSERT_EQ(2, names.size());
  ASSERT_FALSE(names.at(0) == names.at(1));
  ASSERT_TRUE((names.at(0) == "t1") || (names.at(0) == "t2"));
  ASSERT_TRUE((names.at(1) == "t1") || (names.at(1) == "t2"));
}
*/
//----------------------------------------------------------------

