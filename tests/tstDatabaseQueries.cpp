#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "HelperFunc.h"
#include "ClausesAndQueries.h"

using namespace SqliteOverlay;
namespace bfs = boost::filesystem;

TEST_F(DatabaseTestScenario, QueryInt)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  int result = -1;
  string sql = "SELECT COUNT(*) FROM t1 WHERE id > 0";
  ASSERT_TRUE(db->execScalarQueryInt(sql, &result));
  ASSERT_EQ(5, result);
  auto sqr = db->execScalarQueryInt(sql);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ(5, sqr->get());

  // second version: SQL statement as statement
  result = 0;
  auto stmt = db->prepStatement(sql);
  ASSERT_TRUE(db->execScalarQueryInt(stmt, &result));
  ASSERT_EQ(5, result);
  stmt = db->prepStatement(sql);
  sqr = db->execScalarQueryInt(stmt);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ(5, sqr->get());

  // special case: query returning NULL column value
  sql = "SELECT i FROM t1 WHERE id=2";
  sqr = db->execScalarQueryInt(sql);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_TRUE(sqr->isNull());
  ASSERT_THROW(sqr->get(), std::invalid_argument);

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE id=9999";
  sqr = db->execScalarQueryInt(sql);
  ASSERT_TRUE(sqr == nullptr);
  ASSERT_FALSE(db->execScalarQueryInt(sql, &result));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ContentQuery)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  string sql = "SELECT * FROM t1 WHERE id > 0";
  auto stmt = db->execContentQuery(sql);
  ASSERT_TRUE(stmt != nullptr);
  int rowCount = 0;
  while (stmt->hasData())
  {
    ++rowCount;
    int val;
    ASSERT_TRUE(stmt->getInt(0, &val));
    ASSERT_EQ(rowCount, val);
    stmt->step();
  }
  ASSERT_EQ(5, rowCount);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryDouble)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  double result = -1;
  string sql = "SELECT f FROM t1 WHERE id = 2";
  ASSERT_TRUE(db->execScalarQueryDouble(sql, &result));
  ASSERT_EQ(666.66, result);
  auto sqr = db->execScalarQueryDouble(sql);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ(666.66, sqr->get());

  // second version: SQL statement as statement
  result = 0;
  auto stmt = db->prepStatement(sql);
  ASSERT_TRUE(db->execScalarQueryDouble(stmt, &result));
  ASSERT_EQ(666.66, result);
  stmt = db->prepStatement(sql);
  sqr = db->execScalarQueryDouble(stmt);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ(666.66, sqr->get());

  // special case: query returning NULL column value
  sql = "SELECT f FROM t1 WHERE id=3";
  sqr = db->execScalarQueryDouble(sql);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_TRUE(sqr->isNull());
  ASSERT_THROW(sqr->get(), std::invalid_argument);

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE id=9999";
  sqr = db->execScalarQueryDouble(sql);
  ASSERT_TRUE(sqr == nullptr);
  ASSERT_FALSE(db->execScalarQueryDouble(sql, &result));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryString)
{
  auto db = getScenario01();

  // first version: SQL statement as string
  string result = "xxx";
  string sql = "SELECT s FROM t1 WHERE id = 5";
  ASSERT_TRUE(db->execScalarQueryString(sql, &result));
  ASSERT_EQ("Ho", result);
  auto sqr = db->execScalarQueryString(sql);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ("Ho", sqr->get());

  // second version: SQL statement as statement
  result = "xxx";
  auto stmt = db->prepStatement(sql);
  ASSERT_TRUE(db->execScalarQueryString(stmt, &result));
  ASSERT_EQ("Ho", result);
  stmt = db->prepStatement(sql);
  sqr = db->execScalarQueryString(stmt);
  ASSERT_TRUE(sqr != nullptr);
  ASSERT_FALSE(sqr->isNull());
  ASSERT_EQ("Ho", sqr->get());

  // special case: query returning no results
  sql = "SELECT i FROM t1 WHERE id=9999";
  sqr = db->execScalarQueryString(sql);
  ASSERT_TRUE(sqr == nullptr);
  ASSERT_FALSE(db->execScalarQueryString(sql, &result));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryStringUTF8)
{
  auto db = getScenario01();

  string result = "xxx";
  string sql = "SELECT s FROM t1 WHERE id = 3";
  ASSERT_TRUE(db->execScalarQueryString(sql, &result));
  ASSERT_EQ(u8"äöüÄÖÜ", result);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, QueryAllTableAndViewNames)
{
  auto db = getScenario01();

  StringList names = db->allTableNames();
  ASSERT_EQ(2, names.size());
  ASSERT_FALSE(names.at(0) == names.at(1));
  ASSERT_TRUE((names.at(0) == "t1") || (names.at(0) == "t2"));
  ASSERT_TRUE((names.at(1) == "t1") || (names.at(1) == "t2"));
}

//----------------------------------------------------------------

