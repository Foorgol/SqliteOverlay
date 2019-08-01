#include <gtest/gtest.h>

#include <Sloppy/Crypto/Sodium.h>
#include <Sloppy/Crypto/Crypto.h>

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

  // try JSON binds and gets
  stmt = SqlStatement{db.get(), "INSERT INTO t1(s) VALUES (?)"};
  nlohmann::json jsonIn = nlohmann::json::parse(R"({"a": "abc", "b": 42})");
  stmt.bind(1, jsonIn);
  stmt.step();
  stmt = SqlStatement{db.get(), "SELECT s FROM t1 WHERE rowid=6"};
  stmt.step();
  nlohmann::json jsonOut1 = stmt.getJson(0);
  ASSERT_EQ("abc", jsonOut1["a"]);
  ASSERT_EQ(42, jsonOut1["b"]);
  nlohmann::json jsonOut2;
  stmt.get(0, jsonOut2);
  ASSERT_EQ("abc", jsonOut2["a"]);
  ASSERT_EQ(42, jsonOut2["b"]);

  nlohmann::json jsonNull;
  ASSERT_EQ("null", jsonNull.dump());
  stmt = SqlStatement{db.get(), "INSERT INTO t1(s) VALUES (?)"};
  stmt.bind(1, jsonNull);
  stmt.step();
  stmt = SqlStatement{db.get(), "SELECT s FROM t1 WHERE rowid=7"};
  stmt.step();
  jsonOut1 = stmt.getJson(0);
  ASSERT_EQ("null", jsonOut1.dump());
  ASSERT_TRUE(jsonOut1.empty());

  nlohmann::json jsonEmpty = nlohmann::json::object();
  ASSERT_EQ("{}", jsonEmpty.dump());
  stmt = SqlStatement{db.get(), "INSERT INTO t1(s) VALUES (?)"};
  stmt.bind(1, jsonEmpty);
  stmt.step();
  stmt = SqlStatement{db.get(), "SELECT s FROM t1 WHERE rowid=8"};
  stmt.step();
  jsonOut1 = stmt.getJson(0);
  ASSERT_EQ("{}", jsonOut1.dump());
  ASSERT_TRUE(jsonOut1.empty());
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
  //ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(++stmt);   // try the prefix increment operator instead of `step()`
  ASSERT_TRUE(stmt.isDone());
  ASSERT_FALSE(stmt.hasData());
  ASSERT_THROW(stmt.getInt(0), NoDataException);
  //ASSERT_FALSE(stmt.step());
  ASSERT_FALSE(++stmt);   // "no more data"

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
  ASSERT_THROW(stmt.getInt(0), NullValueException);  // i is NULL in the second row
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

  SqlStatement stmt{db.get(), "SELECT rowid, i, f, s FROM t1 WHERE rowid=1"};
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

TEST_F(DatabaseTestScenario, HugeJsonObjs)
{
  prepScenario01();
  auto db = getRawDbHandle();

  // create a huge JSON object
  static constexpr int keyLen = 20;
  static constexpr int valLen = 100;
  static constexpr int minByteCount = 1e7;
  static constexpr int nEntries = 1 + (minByteCount / (keyLen + valLen + 6));  // 6 = 4 quotes, 1 colon, 1 comma per entry
  nlohmann::json jsonIn = nlohmann::json::object();
  for (int i=0; i < nEntries; ++i)
  {
    string key = Sloppy::Crypto::getRandomAlphanumString(keyLen);
    string val = Sloppy::Crypto::getRandomAlphanumString(valLen);
    jsonIn[key] = val;
  }

  const string serialized = jsonIn.dump();
  ASSERT_TRUE(serialized.size() >= minByteCount);

  // store and retrieve that large JSON object
  SqlStatement stmt{db.get(), "INSERT INTO t1(s) VALUES (?)"};
  stmt.bind(1, jsonIn);
  stmt.step();
  stmt = SqlStatement{db.get(), "SELECT s FROM t1 WHERE rowid=6"};
  stmt.step();
  nlohmann::json jsonOut = stmt.getJson(0);

  // compare the stored and the retrieved object
  ASSERT_EQ(jsonIn.size(), jsonOut.size());
  for (auto it = jsonIn.begin(); it != jsonIn.end(); ++it)
  {
    const string& key = it.key();
    const string& refVal = it.value();
    const string& realVal = jsonOut.at(key);

    ASSERT_EQ(refVal, realVal);
  }
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtColTypeAndName)
{
  prepScenario01();
  auto db = getRawDbHandle();
  SqlStatement stmt{db.get(), "SELECT rowid, i, f, s, d FROM t1 WHERE rowid=1"};
  ASSERT_TRUE(stmt.dataStep());

  // test all getters
  ASSERT_EQ(ColumnDataType::Integer, stmt.getColDataType(0));
  ASSERT_EQ(ColumnDataType::Integer, stmt.getColDataType(1));
  ASSERT_EQ(ColumnDataType::Float, stmt.getColDataType(2));
  ASSERT_EQ(ColumnDataType::Text, stmt.getColDataType(3));
  ASSERT_EQ(ColumnDataType::Text, stmt.getColDataType(4));

  // test invalid columns
  ASSERT_THROW(stmt.getColDataType(42), InvalidColumnException);

  // test NULL columns
  stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=2"};
  ASSERT_TRUE(stmt.dataStep());
  ASSERT_EQ(ColumnDataType::Null, stmt.getColDataType(0));
  ASSERT_TRUE(stmt.isNull(0));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtLimits)
{
  prepScenario01();
  auto db = getRawDbHandle();

  // bind, update and retrieve INT_MIN and INT_MAX
  for (int i : {INT_MIN, INT_MAX})
  {
    SqlStatement stmt{db.get(), "UPDATE t1 SET i = ? WHERE rowid=1"};
    stmt.bind(1, i);
    string sql{"UPDATE t1 SET i = "};
    sql += to_string(i);
    sql += " WHERE rowid=1";
    ASSERT_EQ(sql, stmt.getExpandedSQL());

    ASSERT_TRUE(stmt.step());
    ASSERT_FALSE(stmt.hasData());
    ASSERT_TRUE(stmt.isDone());

    stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=1"};
    ASSERT_TRUE(stmt.step());
    ASSERT_EQ(i, stmt.getInt(0));
    ASSERT_EQ(i, stmt.getLong(0));  // should work for int as well
  }

  // bind, update and retrieve LONG_MIN and LONG_MAX
  for (long i : {LONG_MIN, LONG_MAX})
  {
    SqlStatement stmt{db.get(), "UPDATE t1 SET i = ? WHERE rowid=1"};
    stmt.bind(1, i);
    string sql{"UPDATE t1 SET i = "};
    sql += to_string(i);
    sql += " WHERE rowid=1";
    ASSERT_EQ(sql, stmt.getExpandedSQL());

    ASSERT_TRUE(stmt.step());
    ASSERT_FALSE(stmt.hasData());
    ASSERT_TRUE(stmt.isDone());

    stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=1"};
    ASSERT_TRUE(stmt.step());
    ASSERT_EQ(i, stmt.getLong(0));
    ASSERT_NE(i, stmt.getInt(0));  // should NOT work for int
  }
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtTime)
{
  prepScenario01();
  auto db = getRawDbHandle();

  // the following timestamps are all identical
  auto tzp = Sloppy::DateTime::getPopulatedTzDatabase().time_zone_from_region("Europe/Berlin");
  UTCTimestamp utcRef{2019, 1, 27, 9, 42, 42};
  LocalTimestamp localRef{2019, 1, 27, 10, 42, 42, tzp};
  time_t rawRef{1548582162};

  // check consistency
  ASSERT_EQ(rawRef, utcRef.getRawTime());
  ASSERT_EQ(rawRef, localRef.getRawTime());

  // storage of local time
  SqlStatement stmt{db.get(), "UPDATE t1 SET i = ? WHERE rowid=1"};
  stmt.bind(1, localRef);
  ASSERT_TRUE(stmt.step());
  stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=1"};
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(rawRef, stmt.getLong(0));

  // reset
  stmt = SqlStatement{db.get(), "UPDATE t1 SET i = 0 WHERE rowid=1"};
  ASSERT_TRUE(stmt.step());
  stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=1"};
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(0, stmt.getLong(0));

  // storage of UTC time
  stmt = SqlStatement{db.get(), "UPDATE t1 SET i = ? WHERE rowid=1"};
  stmt.bind(1, utcRef);
  ASSERT_TRUE(stmt.step());
  stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=1"};
  ASSERT_TRUE(stmt.step());
  ASSERT_EQ(rawRef, stmt.getLong(0));

  // retrieval of local time
  stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=1"};
  ASSERT_TRUE(stmt.step());
  LocalTimestamp lt = stmt.getLocalTime(0, tzp);
  ASSERT_EQ(lt, localRef);
  ASSERT_EQ(rawRef, lt.getRawTime());

  // retrieval of UTC time
  stmt = SqlStatement{db.get(), "SELECT i FROM t1 WHERE rowid=1"};
  ASSERT_TRUE(stmt.step());
  UTCTimestamp u = stmt.getUTCTime(0);
  ASSERT_EQ(u, utcRef);
  ASSERT_EQ(rawRef, u.getRawTime());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, StmtBlob)
{
  // start with a blank in-memory database
  auto db = SqliteDatabase();

  // create a table with a blob column
  SqlStatement stmt = db.prepStatement("CREATE TABLE t1(b BLOB)");
  ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(db.hasTable("t1"));

  // create 1M dummy data
  auto* sodium = Sloppy::Crypto::SodiumLib::getInstance();
  Sloppy::MemArray buf{1000000};
  sodium->randombytes_buf(buf);

  // store the dummy data in the database
  stmt = db.prepStatement("INSERT INTO t1(b) VALUES(?)");
  stmt.bind(1, buf.view());
  ASSERT_TRUE(stmt.step());

  // retrieve the dummy data from the database
  stmt = db.prepStatement("SELECT b FROM t1 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(stmt.hasData());
  Sloppy::MemArray reBuf = stmt.getBlob(0);

  // compare both buffers; only succeeds if the buffers have equal size and content
  ASSERT_TRUE(sodium->memcmp(buf.view(), reBuf.view()));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TemplateGetter)
{
  auto db = getScenario01();

  auto stmt = db.prepStatement("SELECT rowid, i, f, s FROM t1 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(stmt.hasData());

  int i{0};
  stmt.get(0, i);
  ASSERT_EQ(1, i);

  long l{0};
  stmt.get(1, l);
  ASSERT_EQ(42, l); // i column

  double d{0};
  stmt.get(2, d);
  ASSERT_EQ(23.23, d);

  string s;
  stmt.get(3, s);
  ASSERT_EQ("Hallo", s);

  //
  // test the more advanced multi-getters
  //
  i = 0;
  l = 0;
  stmt.multiGet(0, i, 1, l);
  ASSERT_EQ(1, i);
  ASSERT_EQ(42, l);

  d = 0;
  s.clear();
  tie (d,s) = stmt.tupleGet<double, string>(2, 3);
  ASSERT_EQ(23.23, d);
  ASSERT_EQ("Hallo", s);

  i = 0;
  l = 0;
  d = 0;
  s.clear();
  stmt.multiGet(0, i, 1, l, 2, d, 3, s);
  ASSERT_EQ(1, i);
  ASSERT_EQ(42, l);
  ASSERT_EQ(23.23, d);
  ASSERT_EQ("Hallo", s);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, TemplateGetterOptional)
{
  SqliteDatabase db{}; // empty in-memory database
  db.execNonQuery("CREATE TABLE t1(a, b, c)");
  db.execNonQuery("INSERT INTO t1(a, b, c) VALUES(NULL, NULL, NULL)");
  db.execNonQuery("INSERT INTO t1(a, b, c) VALUES(42, \"xxx\", 3.3)");

  optional<int> i;
  optional<double> d;
  optional<long> l;
  optional<string> s;

  auto stmt = db.prepStatement("SELECT rowid, a, b, c FROM t1 WHERE rowid=2");
  ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(stmt.hasData());

  stmt.get(0, i);
  ASSERT_EQ(2, i.value());

  stmt.get(1, l);
  ASSERT_EQ(42, l.value()); // i column

  stmt.get(2, s);
  ASSERT_EQ("xxx", s.value());

  stmt.get(3, d);
  ASSERT_EQ(3.3, d.value());

  stmt = db.prepStatement("SELECT rowid, a, b, c FROM t1 WHERE rowid=1");
  ASSERT_TRUE(stmt.step());
  ASSERT_TRUE(stmt.hasData());

  stmt.get(0, i);
  ASSERT_EQ(1, i.value());

  stmt.get(1, l);
  ASSERT_FALSE(l.has_value());

  stmt.get(2, s);
  ASSERT_FALSE(s.has_value());

  stmt.get(3, d);
  ASSERT_FALSE(d.has_value());

  //
  // test the more advanced multi-getters
  //
  int plainInt{0};
  stmt.multiGet(0, plainInt, 1, i);
  ASSERT_EQ(1, plainInt);
  ASSERT_FALSE(i.has_value());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ExportCSV1)
{
  auto db = getScenario01();

  // easy case:
  // export the whole table
  auto stmt = db.prepStatement("SELECT rowid, i, f, s FROM t1");
  auto csv = stmt.toCSV(true);
  ASSERT_TRUE(csv.hasHeaders());
  ASSERT_EQ(4, csv.nCols());
  ASSERT_EQ("rowid", csv.getHeader(0));
  ASSERT_EQ("i", csv.getHeader(1));
  ASSERT_EQ("f", csv.getHeader(2));
  ASSERT_EQ("s", csv.getHeader(3));
  ASSERT_EQ(5, csv.size());
  auto& r0 = csv.get(1);
  ASSERT_EQ(2, r0[0].get<long>());  // rowid
  ASSERT_FALSE(r0[1].has_value());  // NULL in i
  ASSERT_EQ(666.66, r0[2].get<double>());  // double in f
  ASSERT_EQ("Hi", r0[3].get<string>());

  // make sure the statement is finalized
  ASSERT_TRUE(stmt.isDone());

  // throw if applied to finalized statements
  ASSERT_THROW(stmt.toCSV(true), NoDataException);

  // export only partial tables / query data because
  // step() has already been called
  stmt = db.prepStatement("SELECT rowid, i, f, s FROM t1");
  stmt.step();  // now on row 1
  stmt.step();  // now on row 2
  csv = stmt.toCSV(false);
  ASSERT_FALSE(csv.hasHeaders());
  ASSERT_EQ(4, csv.size());
  auto& r1 = csv.get(0);
  ASSERT_EQ(2, r1[0].get<long>());  // rowid
  ASSERT_FALSE(r1[1].has_value());  // NULL in i
  ASSERT_EQ(666.66, r1[2].get<double>());  // double in f
  ASSERT_EQ("Hi", r1[3].get<string>());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ExportCSV2)
{
  auto db = getScenario01();

  // use a dummy statement that doesn't produce any data
  auto stmt = db.prepStatement("SELECT rowid, i FROM t1 WHERE rowid > 1000");
  auto csv = stmt.toCSV(true);
  ASSERT_EQ(0, csv.size());
  ASSERT_EQ(0, csv.nCols());
  ASSERT_FALSE(csv.hasHeaders());   // no data rows ==> no headers, even if requested!
  ASSERT_TRUE(stmt.isDone());
}
