#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"
#include "TabRow.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, DbTab_Ctor)
{
  auto db = getScenario01();

  ASSERT_NO_THROW(DbTab(db, "t1", true));
  ASSERT_THROW(DbTab(db, "dsdfsdf", true), NoSuchTableException);
  ASSERT_NO_THROW(DbTab(db, "dsdfsdf", false));
  ASSERT_THROW(DbTab(db, "", true), std::invalid_argument);
  ASSERT_THROW(DbTab(db, " ", true), std::invalid_argument);
  ASSERT_THROW(DbTab(db, "", false), std::invalid_argument);
  ASSERT_THROW(DbTab(db, " ", false), std::invalid_argument);

  DbTab t1{db, "t1", true};
  ASSERT_TRUE(t1.length() > 0);
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_Insert)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  // insert default values
  int oldLen = t1.length();
  int newId = t1.insertRow();
  ASSERT_EQ(oldLen+1, newId);
  ASSERT_EQ(oldLen+1, t1.length());

  ++oldLen;

  // insert specific values
  ColumnValueClause cvc;
  cvc.addCol("i", 1234);
  cvc.addCol("f", 99.88);
  cvc.addNullCol("s");
  newId = t1.insertRow(cvc);
  ASSERT_EQ(oldLen+1, newId);
  ASSERT_EQ(oldLen+1, t1.length());
  string sql = "SELECT i FROM t1 WHERE rowid=" + to_string(newId);
  ASSERT_EQ(1234, db.execScalarQueryInt(sql));
  sql = "SELECT f FROM t1 WHERE rowid=" + to_string(newId);
  ASSERT_EQ(99.88, db.execScalarQueryDouble(sql));
  sql = "SELECT s FROM t1 WHERE rowid=" + to_string(newId);
  ASSERT_THROW(db.execScalarQueryString(sql), NullValueException);
  auto sOpt = db.execScalarQueryStringOrNull(sql);
  ASSERT_FALSE(sOpt.has_value());

  // insert nonsense values
  ++oldLen;
  cvc.clear();
  cvc.addCol("sdkjfsfd", 88);
  ASSERT_THROW(t1.insertRow(cvc), SqlStatementCreationError);
  ASSERT_EQ(oldLen, t1.length());
}
//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_SubscriptOperator)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  TabRow r = t1[2];
  ASSERT_EQ(2, r.id());
  ASSERT_THROW(t1[0], std::invalid_argument);
  ASSERT_THROW(t1[-1], std::invalid_argument);
  ASSERT_NO_THROW(t1[100]);  // actual existence of the row is intentionally not checked!

  WhereClause w;
  w.addCol("s", "Ho");
  r = t1[w];
  ASSERT_EQ(4, r.id());

  w.clear();
  w.addCol("lsdkf", "kljfd");
  ASSERT_THROW(t1[w], std::invalid_argument);

  w.clear();
  ASSERT_THROW(t1[w], std::invalid_argument);  // empty WHERE
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_Get2)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  // get2 with an ID
  for (int id : {-1, 0, 200})
  {
    auto r = t1.get2(id);
    ASSERT_FALSE(r.has_value());
  }

  auto r = t1.get2(3);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(3, r->id());

  // get2 with WHERE clauses
  WhereClause w;
  r = t1.get2(w);  // empty clause
  ASSERT_FALSE(r.has_value());
  w.addCol("sdkjf", "sdlfkj");
  r = t1.get2(w);  // invalid clause
  ASSERT_FALSE(r.has_value());
  w.clear();
  w.addCol("rowid", 200);
  r = t1.get2(w);  // clause without matches
  ASSERT_FALSE(r.has_value());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_RowByColumnValue)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  // single row by column value
  TabRow r = t1.getSingleRowByColumnValue("s", "Ho");
  ASSERT_EQ(4, r.id());
  r = t1.getSingleRowByColumnValue("f", 666.66);
  ASSERT_EQ(2, r.id());
  for (const string& s : {"i", "lkjfd", " ", ""})
  {
    ASSERT_THROW(t1.getSingleRowByColumnValue(s, 1000), NoDataException);
  }

  // single row by column value with optional result
  auto r2 = t1.getSingleRowByColumnValue2("s", "Ho");
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(4, r2->id());
  for (const string& s : {"i", "lkjfd", " ", ""})
  {
    auto r3 = t1.getSingleRowByColumnValue2(s, 1000);
    ASSERT_FALSE(r3.has_value());
  }

  // single row by column value NULL
  r = t1.getSingleRowByColumnValueNull("i");
  ASSERT_EQ(2, r.id());
  for (const string& s : {"s", "lkjfd", " ", ""})
  {
    ASSERT_THROW(t1.getSingleRowByColumnValueNull(s), NoDataException);
  }

  // single row by column value NULL with optional result
  r2 = t1.getSingleRowByColumnValueNull2("i");
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(2, r2->id());
  for (const string& s : {"s", "lkjfd", " ", ""})
  {
    auto r3 = t1.getSingleRowByColumnValueNull2(s);
    ASSERT_FALSE(r3.has_value());
  }
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_RowByWhere)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  string w{"rowid=3"};
  TabRow r = t1.getSingleRowByWhereClause(w);
  ASSERT_EQ(3, r.id());

  w = "rowid=3 AND rowid=4";
  ASSERT_THROW(t1.getSingleRowByWhereClause(w), NoDataException);

  for (const string& s : {"sfjklsdf", " ", ""})
  {
    ASSERT_THROW(t1.getSingleRowByWhereClause(s), SqlStatementCreationError);
  }
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_RowListByWhere)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  WhereClause w;
  ASSERT_THROW(t1.getRowsByWhereClause(w), std::invalid_argument);

  w.addCol("sdlfsdf", 42);
  auto rl = t1.getRowsByWhereClause(w);
  ASSERT_TRUE(rl.empty());

  w.clear();
  w.addCol("rowid", ">", 0);
  rl = t1.getRowsByWhereClause(w);
  ASSERT_EQ(5, rl.size());

  w.clear();
  w.addCol("i", 1000);
  rl = t1.getRowsByWhereClause(w);
  ASSERT_TRUE(rl.empty());

  // invalid where strings
  for (const string& ws : {"rowid < 0", "sskjdf=99", "i=100", "jkdfsdf", " "})
  {
    rl = t1.getRowsByWhereClause(ws);
    ASSERT_TRUE(rl.empty());
  }

  // empty where string
  ASSERT_THROW(t1.getRowsByWhereClause(""), std::invalid_argument);

  // valid string
  rl.clear();
  rl = t1.getRowsByWhereClause("rowid>0");
  ASSERT_EQ(5, rl.size());

  // valid string with empty result
  rl.clear();
  rl = t1.getRowsByWhereClause("rowid=3 AND rowid=2");
  ASSERT_EQ(0, rl.size());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_RowListByNull)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  ASSERT_THROW(t1.getRowsByColumnValueNull(""), std::invalid_argument);

  auto rl = t1.getRowsByColumnValueNull("i");
  ASSERT_EQ(1, rl.size());
  ASSERT_EQ(2, rl[0].id());

  rl = t1.getRowsByColumnValueNull("f");
  ASSERT_EQ(2, rl.size());
  ASSERT_EQ(3, rl[0].id());
  ASSERT_EQ(4, rl[1].id());

  for (const string& s : {"s", "lkjfd", " "})
  {
    rl = t1.getRowsByColumnValueNull(s);
    ASSERT_TRUE(rl.empty());
  }

}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_RowListByColVal)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  ASSERT_THROW(t1.getRowsByColumnValue("", 42), std::invalid_argument);

  auto rl = t1.getRowsByColumnValue("f", 666.66);
  ASSERT_EQ(1, rl.size());
  ASSERT_EQ(2, rl[0].id());

  rl = t1.getRowsByColumnValue("i", 84);
  ASSERT_EQ(3, rl.size());
  for (int i=0; i<3; ++i)
  {
    ASSERT_EQ(3+i, rl[i].id());
  }

  for (const string& s : {"s", "lkjfd", " "})
  {
    rl = t1.getRowsByColumnValue(s, "99");
    ASSERT_TRUE(rl.empty());
  }
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_AllRows)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  auto rl = t1.getAllRows();
  ASSERT_EQ(5, rl.size());

  DbTab t2{db,"t2", false};
  rl = t2.getAllRows();
  ASSERT_TRUE(rl.empty());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_DeleteByWhere)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  // define where clause
  WhereClause w;
  w.addCol("rowid", ">", 2);

  // exec delete
  int oldLen = t1.length();
  int delCnt = t1.deleteRowsByWhereClause(w);
  ASSERT_EQ(3, delCnt);
  ASSERT_EQ(2, t1.length());

  // try where clause that matches no rows
  w.clear();
  w.addCol("rowid", ">", 2222);
  delCnt = t1.deleteRowsByWhereClause(w);
  ASSERT_EQ(0, delCnt);
  ASSERT_EQ(2, t1.length());

  // delete by nonsense values
  w.clear();
  w.addCol("sdkjfsfd", 88);
  ASSERT_THROW(t1.deleteRowsByWhereClause(w), SqlStatementCreationError);
  ASSERT_EQ(2, t1.length());

  w.clear();
  ASSERT_THROW(t1.deleteRowsByWhereClause(w), std::invalid_argument);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_DeleteByColVal)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  ASSERT_THROW(t1.deleteRowsByColumnValue("", 42), std::invalid_argument);
  ASSERT_THROW(t1.deleteRowsByColumnValue("sfsdfsfd", 42), SqlStatementCreationError);

  int oldLen = t1.length();
  int delCnt = t1.deleteRowsByColumnValue("i", 84);
  ASSERT_EQ(3, delCnt);
  ASSERT_EQ(2, t1.length());

  oldLen = t1.length();
  delCnt = t1.deleteRowsByColumnValue("i", 8484);
  ASSERT_EQ(0, delCnt);
  ASSERT_EQ(2, t1.length());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_DeleteAll)
{
  auto db = getScenario01();

  DbTab t1{db,"t1", false};
  ASSERT_EQ(5, t1.length());
  t1.clear();
  ASSERT_EQ(0, t1.length());

  DbTab t2{db,"t2", false};
  ASSERT_EQ(0, t2.length());
  t2.clear();
  ASSERT_EQ(0, t2.length());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, DbTab_HasRow)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  ASSERT_TRUE(t1.hasRowId(1));
  ASSERT_FALSE(t1.hasRowId(100));
}

//----------------------------------------------------------------
TEST_F(DatabaseTestScenario, DbTab_AddColumn)
{
  auto db = getScenario01();
  DbTab t1{db,"t1", false};

  // helper func for retrieving the CREATE TABLE string
  // for table t1 and checking for certain text snippets
  auto hasColDef = [&db](const string& snippet){
    string s = db.execScalarQueryString("SELECT sql FROM sqlite_master WHERE name='t1'");
    cout << s << endl;
    auto idx = s.find(snippet);
    return (idx != string::npos);
  };

  // try to create an already exising column
  int oldColCount = t1.allColDefs().size();
  bool isOkay = t1.addColumn("i", ColumnDataType::Text);
  ASSERT_FALSE(isOkay);
  ASSERT_EQ(oldColCount, t1.allColDefs().size());

  // actually create a new column
  isOkay = t1.addColumn("i1", ColumnDataType::Integer, ConflictClause::NotUsed, "42");
  ASSERT_TRUE(isOkay);
  ASSERT_EQ(oldColCount+1, t1.allColDefs().size());
  ASSERT_TRUE(hasColDef("i1 INTEGER DEFAULT '42'"));

  // new column with NOT NULL constraint
  isOkay = t1.addColumn("i2", ColumnDataType::Text, ConflictClause::Abort, 999);
  ASSERT_TRUE(isOkay);
  ASSERT_EQ(oldColCount+2, t1.allColDefs().size());
  ASSERT_TRUE(hasColDef("i2 TEXT NOT NULL ON CONFLICT ABORT DEFAULT 999"));

  // empty column name
  ASSERT_THROW(t1.addColumn("", ColumnDataType::Text, ConflictClause::Abort, "999"), std::invalid_argument);

  // NULL data type
  isOkay = t1.addColumn("i3", ColumnDataType::Null, ConflictClause::Fail, 23.23);
  ASSERT_TRUE(isOkay);
  ASSERT_EQ(oldColCount+3, t1.allColDefs().size());
  ASSERT_TRUE(hasColDef("i3  NOT NULL ON CONFLICT FAIL DEFAULT 23.23"));

  //
  // Foreign key
  //
  DbTab t2{db, "t2", false};
  ColumnValueClause cvc;
  cvc.addCol("i", 3);
  cvc.addCol("f", 42.42);
  cvc.addCol("s", "abcd");
  ASSERT_EQ(1, t2.insertRow(cvc));

  isOkay = t1.addColumn_foreignKey("iRef", "t2", "", ConsistencyAction::Cascade, ConsistencyAction::SetNull);
  ASSERT_TRUE(isOkay);
  ASSERT_EQ(oldColCount+4, t1.allColDefs().size());
  ASSERT_TRUE(hasColDef("iRef INTEGER  REFERENCES t2(rowid) ON DELETE CASCADE ON UPDATE SET NULL"));

  isOkay = t1.addColumn_foreignKey("sRef", "t2", "s", ConsistencyAction::Restrict, ConsistencyAction::SetNull);
  ASSERT_TRUE(isOkay);
  ASSERT_EQ(oldColCount+5, t1.allColDefs().size());
  ASSERT_TRUE(hasColDef("sRef TEXT  REFERENCES t2(s) ON DELETE RESTRICT ON UPDATE SET NULL"));
}
