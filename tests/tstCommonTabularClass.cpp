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

#include "DatabaseTestScenario.h"
#include "CommonTabularClass.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, CommonTabularClass_TestConstructor)
{
  auto db = getScenario01();
  
  // test NULL ptr to db
  ASSERT_THROW(CommonTabularClass t(NULL, "Lala", true), invalid_argument);
  
  // test invalid table / view name
  ASSERT_THROW(CommonTabularClass t(db.get(), "", true), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db.get(), "", false), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db.get(), "Lala", true), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db.get(), "Lala", false), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db.get(), "t1", true), invalid_argument);
  ASSERT_THROW(CommonTabularClass t(db.get(), "v1", false), invalid_argument);
  
  // test valid calls
  CommonTabularClass t(db.get(), "t1");
  CommonTabularClass v(db.get(), "v1", true);
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testAllColDefs)
{
  auto db = getScenario01();

  CommonTabularClass ctc(db.get(), "t1");
  ColInfoList cil = ctc.allColDefs();
    
  ASSERT_TRUE(cil.size() == 5);

  ColInfo ci = cil.at(0);
  ASSERT_TRUE(ci.getId() == 0);
  ASSERT_EQ("id", ci.getColName());
  ASSERT_EQ("INTEGER", ci.getColType());

  ci = cil.at(1);
  ASSERT_TRUE(ci.getId() == 1);
  ASSERT_EQ("i", ci.getColName());
  ASSERT_EQ("INT", ci.getColType());

  ci = cil.at(2);
  ASSERT_TRUE(ci.getId() == 2);
  ASSERT_EQ("f", ci.getColName());
  ASSERT_EQ("DOUBLE", ci.getColType());

  ci = cil.at(3);
  ASSERT_TRUE(ci.getId() == 3);
  ASSERT_EQ("s", ci.getColName());
  ASSERT_EQ("VARCHAR(40)", ci.getColType());

  ci = cil.at(4);
  ASSERT_TRUE(ci.getId() == 4);
  ASSERT_EQ("d", ci.getColName());
  ASSERT_EQ("DATETIME", ci.getColType());

}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testGetColType)
{
  auto db = getScenario01();
  CommonTabularClass t1(db.get(), "t1");
  
  // test invalid column names
  ASSERT_EQ(string(), t1.getColType("Lalala"));
  ASSERT_EQ(string(), t1.getColType(""));
  ASSERT_EQ(string(), t1.getColType(string()));
  
  // test normal column
  ASSERT_EQ("DOUBLE", t1.getColType("f"));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testcid2name)
{
  auto db = getScenario01();
  CommonTabularClass t1(db.get(), "t1");

  // test invalid column indices
  ASSERT_EQ(string(), t1.cid2name(-1));
  ASSERT_EQ(string(), t1.cid2name(200));
  
  // test normal column
  ASSERT_EQ("id", t1.cid2name(0));
  ASSERT_EQ("i", t1.cid2name(1));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testname2cid)
{
  auto db = getScenario01();
  CommonTabularClass t1(db.get(), "t1");

  // test invalid column names
  ASSERT_EQ(-1, t1.name2cid("Lalala"));
  ASSERT_EQ(-1, t1.name2cid(""));
  ASSERT_EQ(-1, t1.name2cid(string()));
  
  // test normal column
  ASSERT_EQ(0, t1.name2cid("id"));
  ASSERT_EQ(1, t1.name2cid("i"));
  ASSERT_EQ(2, t1.name2cid("f"));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testHasColumn)
{
  auto db = getScenario01();
  CommonTabularClass t1(db.get(), "t1");
  
  // test invalid column names
  ASSERT_FALSE(t1.hasColumn("Lalala"));
  ASSERT_FALSE(t1.hasColumn(""));
  ASSERT_FALSE(t1.hasColumn(string()));
  
  // test normal column
  ASSERT_TRUE(t1.hasColumn("id"));
  ASSERT_TRUE(t1.hasColumn("i"));
  ASSERT_TRUE(t1.hasColumn("f"));

  // test invalid column indices
  ASSERT_FALSE(t1.hasColumn(-1));
  ASSERT_FALSE(t1.hasColumn(200));
  
  // test normal column
  ASSERT_TRUE(t1.hasColumn(0));
  ASSERT_TRUE(t1.hasColumn(1));
  ASSERT_TRUE(t1.hasColumn(4));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testGetMatchCountForWhereClause)
{
  auto db = getScenario01();
  CommonTabularClass t1(db.get(), "t1");
  
  // test invalid or empty where clause
  ASSERT_EQ(-1, t1.getMatchCountForWhereClause(""));
  ASSERT_EQ(-1, t1.getMatchCountForWhereClause(string()));
  ASSERT_EQ(-1, t1.getMatchCountForWhereClause("lkdflsjflsdf"));
  
  // test valid queries without parameters
  ASSERT_EQ(3, t1.getMatchCountForWhereClause("i = 84"));
  
  // test valid query with parameters
  WhereClause w;
  w.addStringCol("s", "Ho");
  w.addIntCol("i", ">", 50);
  ASSERT_EQ(2, t1.getMatchCountForWhereClause(w));
  
  // test a query that matches zero rows
  w.clear();
  w.addIntCol("i", ">", 5000);
  ASSERT_EQ(0, t1.getMatchCountForWhereClause(w));

  // test an empty where-clause-instance
  w.clear();
  ASSERT_EQ(-1, t1.getMatchCountForWhereClause(w));

    // two parameters, ANDed, incl. NULL
  w.clear();
  w.addIntCol("i", 84);
  w.addNullCol("f");
  ASSERT_EQ(2, t1.getMatchCountForWhereClause(w));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testGetMatchCountForColumnValue)
{
  auto db = getScenario01();
  CommonTabularClass t1(db.get(), "t1");

  ASSERT_EQ(-1, t1.getMatchCountForColumnValue("InvalidColName", 42));
  
  // test valid query with parameters
  ASSERT_EQ(3, t1.getMatchCountForColumnValue("i", 84));
  ASSERT_EQ(1, t1.getMatchCountForColumnValue("f", 666.66));
  ASSERT_EQ(2, t1.getMatchCountForColumnValue("s", "Ho"));

  // test a query that matches zero rows
  ASSERT_EQ(0, t1.getMatchCountForColumnValue("i", 5000));
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, CommonTabularClass_testGetLength)
{
  auto db = getScenario01();
  CommonTabularClass t1(db.get(), "t1");
  CommonTabularClass t2(db.get(), "t2");
  CommonTabularClass v1(db.get(), "v1", true);

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



