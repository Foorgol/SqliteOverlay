/*
 * Copyright © 2014 Volker Knollmann
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file or visit
 * http://www.wtfpl.net/ for more details.
 * 
 * This program comes without any warranty. Use it at your own risk or
 * don't use it at all.
 */

#include <stdexcept>

#include <boost/date_time/local_time/local_time.hpp>

#include "DatabaseTestScenario.h"
#include "CommonTabularClass.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, CommonTabularClass_TestConstructor)
{
  auto db = getScenario01();
  
  // test invalid table / view name
  ASSERT_THROW(CommonTabularClass t(db, "", true, true), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db, "", false, true), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db, " ", true, true), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db, " ", false, true), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db, "Lala", true, true), NoSuchTableException);
  ASSERT_THROW(CommonTabularClass t(db, "Lala", false, true), NoSuchTableException);
  ASSERT_THROW(CommonTabularClass t(db, "t1", true, true), NoSuchTableException);
  ASSERT_THROW(CommonTabularClass t(db, "v1", false, true), NoSuchTableException);
  
  // test valid calls
  CommonTabularClass t(db, "t1", false, true);
  CommonTabularClass v(db, "v1", true, true);

  // the next two succeed because the name check is disabled
  CommonTabularClass ctc1(db, "Lala", true, false);
  CommonTabularClass ctc2(db, "Lala", false, false);
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testAllColDefs)
{
  auto db = getScenario01();

  CommonTabularClass ctc(db, "t1", false, false);
  ColInfoList cil = ctc.allColDefs();
    
  ASSERT_EQ(4, cil.size());

  auto ci = cil.at(0);
  ASSERT_TRUE(ci.id() == 0);
  ASSERT_EQ("i", ci.name());
  ASSERT_EQ(ColumnAffinity::Integer, ci.affinity());
  ASSERT_EQ("INT", ci.declType());

  ci = cil.at(1);
  ASSERT_TRUE(ci.id() == 1);
  ASSERT_EQ("f", ci.name());
  ASSERT_EQ(ColumnAffinity::Real, ci.affinity());
  ASSERT_EQ("DOUBLE", ci.declType());

  ci = cil.at(2);
  ASSERT_TRUE(ci.id() == 2);
  ASSERT_EQ("s", ci.name());
  ASSERT_EQ(ColumnAffinity::Text, ci.affinity());
  ASSERT_EQ("VARCHAR(40)", ci.declType());

  ci = cil.at(3);
  ASSERT_TRUE(ci.id() == 3);
  ASSERT_EQ("d", ci.name());
  ASSERT_EQ(ColumnAffinity::Numeric, ci.affinity());
  ASSERT_EQ("DATETIME", ci.declType());
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_AffinityAndDeclType)
{
  auto db = getScenario01();
  CommonTabularClass ctc(db, "t1", false, false);

  ASSERT_EQ(ColumnAffinity::Integer, ctc.colAffinity("i"));
  ASSERT_EQ("INT", ctc.colDeclType("i"));

  ASSERT_THROW(ctc.colDeclType("invalidName"), std::invalid_argument);
  ASSERT_THROW(ctc.colAffinity("invalidName"), std::invalid_argument);
  ASSERT_THROW(ctc.colDeclType(""), std::invalid_argument);
  ASSERT_THROW(ctc.colAffinity(""), std::invalid_argument);
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testcid2name)
{
  auto db = getScenario01();
  CommonTabularClass t1(db, "t1", false, false);

  // test invalid column indices
  ASSERT_THROW(t1.cid2name(-1), std::invalid_argument);
  ASSERT_THROW(t1.cid2name(200), std::invalid_argument);

  // test normal columns
  ASSERT_EQ("i", t1.cid2name(0));
  ASSERT_EQ("f", t1.cid2name(1));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testname2cid)
{
  auto db = getScenario01();
  CommonTabularClass t1(db, "t1", false, false);

  // test invalid column names
  ASSERT_THROW(t1.name2cid("Lalala"), std::invalid_argument);
  ASSERT_THROW(t1.name2cid(""), std::invalid_argument);
  ASSERT_THROW(t1.name2cid(string()), std::invalid_argument);
  
  // test normal columns
  ASSERT_EQ(0, t1.name2cid("i"));
  ASSERT_EQ(1, t1.name2cid("f"));
  ASSERT_EQ(2, t1.name2cid("s"));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testHasColumn)
{
  auto db = getScenario01();
  CommonTabularClass t1(db, "t1", false, false);
  
  // test invalid column names
  ASSERT_FALSE(t1.hasColumn("Lalala"));
  ASSERT_FALSE(t1.hasColumn(""));
  ASSERT_FALSE(t1.hasColumn(string()));
  
  // test normal column
  ASSERT_TRUE(t1.hasColumn("i"));
  ASSERT_TRUE(t1.hasColumn("f"));

  // test invalid column indices
  ASSERT_FALSE(t1.hasColumn(-1));
  ASSERT_FALSE(t1.hasColumn(200));
  
  // test normal columns
  ASSERT_TRUE(t1.hasColumn(0));
  ASSERT_TRUE(t1.hasColumn(1));
  ASSERT_TRUE(t1.hasColumn(3));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testGetMatchCountForWhereClause)
{
  auto db = getScenario01();
  CommonTabularClass t1(db, "t1", false, false);

  // invalid or empty where clause object
  WhereClause w;
  ASSERT_THROW(t1.getMatchCountForWhereClause(w), std::invalid_argument);
  w.addCol("InvalidName", 42);
  ASSERT_THROW(t1.getMatchCountForWhereClause(w), SqlStatementCreationError);

  // invalid or empty strings
  ASSERT_THROW(t1.getMatchCountForWhereClause(""), std::invalid_argument);
  ASSERT_THROW(t1.getMatchCountForWhereClause("skdjf"), SqlStatementCreationError);
  ASSERT_THROW(t1.getMatchCountForWhereClause("x=199"), SqlStatementCreationError);

  // test valid queries without parameters
  ASSERT_EQ(3, t1.getMatchCountForWhereClause("i = 84"));
  
  // test valid query with parameters
  w.clear();
  w.addCol("s", "Ho");
  w.addCol("i", ">", 50);
  ASSERT_EQ(2, t1.getMatchCountForWhereClause(w));
  
  // test a query that matches zero rows
  w.clear();
  w.addCol("i", ">", 5000);
  ASSERT_EQ(0, t1.getMatchCountForWhereClause(w));

  // two parameters, ANDed, incl. NULL
  w.clear();
  w.addCol("i", 84);
  w.addNullCol("f");
  ASSERT_EQ(2, t1.getMatchCountForWhereClause(w));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testGetMatchCountForColumnValue)
{
  auto db = getScenario01();
  CommonTabularClass t1(db, "t1", false, false);

  ASSERT_THROW(t1.getMatchCountForColumnValue("", 42), std::invalid_argument);
  ASSERT_THROW(t1.getMatchCountForColumnValue("InvalidColName", 42), std::invalid_argument);
  
  // test valid query with parameters
  ASSERT_EQ(3, t1.getMatchCountForColumnValue("i", 84));
  ASSERT_EQ(1, t1.getMatchCountForColumnValue("f", 666.66));
  ASSERT_EQ(2, t1.getMatchCountForColumnValue("s", "Ho"));

  // test a query that matches zero rows
  ASSERT_EQ(0, t1.getMatchCountForColumnValue("i", 5000));

  // test for NULL query
  ASSERT_EQ(2, t1.getMatchCountForColumnValueNull("f"));
  ASSERT_EQ(0, t1.getMatchCountForColumnValueNull("rowid"));
  ASSERT_THROW(t1.getMatchCountForColumnValueNull(""), std::invalid_argument);
  ASSERT_THROW(t1.getMatchCountForColumnValueNull("InvalidColName"), std::invalid_argument);
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testGetLength)
{
  auto db = getScenario01();
  CommonTabularClass t1(db, "t1", false, false);
  CommonTabularClass t2(db, "t2", false, false);
  CommonTabularClass v1(db, "v1", true, false);

  // test normal table
  ASSERT_EQ(5, t1.length());
  
  // test empty table
  ASSERT_EQ(0, t2.length());

  // test view
  ASSERT_EQ(3, v1.length());
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------



