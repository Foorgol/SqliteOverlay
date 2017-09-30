#include <vector>
#include <tuple>

#include <gtest/gtest.h>

#include <Sloppy/DateTime/DateAndTime.h>

#include "Utils.h"

using namespace SqliteOverlay;


TEST(CSV, EscapeString)
{
  // define a list of in- and out-strings
  // the output string does not contain the
  // enclosing quotes
  vector<pair<string,string>> sList = {
    {"", ""},
    {" ", " "},
    {"abc", "abc"},
    {",", "\\,"},
    {"a\nb", "a\\nb"},
    {"\n", "\\n"},
    {"\\", "\\\\"},
    {"\"", "\\\""},
    {"", ""},
  };

  // check quoting / escaping of all sample strings
  for (const pair<string,string>& p : sList)
  {
    string out = SqliteOverlay::quoteAndEscapeStringForCSV(p.first);
    string expected = "\"" + p.second + "\"";
    ASSERT_EQ(expected, out);

    // also check the return path: unquote / unescape
    string orig = SqliteOverlay::unquoteAndUnescapeStringFromCSV(out);
    ASSERT_EQ(p.first, orig);
  }
}

//----------------------------------------------------------------------------

TEST(CSV, CSV_Value_Null)
{
  // null value
  CSV_Value v;
  ASSERT_TRUE(v.isNull());
  ASSERT_FALSE(v.hasNumericValue());
  ASSERT_THROW(v.asString(), CSV_NullValueException);
  ASSERT_THROW(v.asStringRef(), CSV_NullValueException);
  ASSERT_THROW(v.asInt(), CSV_NullValueException);
  ASSERT_THROW(v.asLong(), CSV_NullValueException);
  ASSERT_THROW(v.asDouble(), CSV_NullValueException);
  ASSERT_THROW(v.asUTCTimestamp(), CSV_NullValueException);
}

//----------------------------------------------------------------------------

TEST(CSV, CSV_Value_string)
{
  // define a list of unescaped and escaped strings
  // the escaped string does not contain the
  // enclosing quotes
  vector<pair<string,string>> sList = {
    {" ", " "},
    {"abc", "abc"},
    {",", "\\,"},
    {"a\nb", "a\\nb"},
    {"\n", "\\n"},
    {"\\", "\\\\"},
    {"\"", "\\\""},
  };

  for (const pair<string,string>& p : sList)
  {
    CSV_Value v{"\"" + p.second + "\""};

    ASSERT_FALSE(v.isNull());
    ASSERT_FALSE(v.hasNumericValue());
    ASSERT_EQ(p.first, v.asString());
    ASSERT_EQ(p.first, v.asStringRef());
    ASSERT_THROW(v.asInt(), std::invalid_argument);
    ASSERT_THROW(v.asLong(), std::invalid_argument);
    ASSERT_THROW(v.asDouble(), std::invalid_argument);
    ASSERT_THROW(v.asUTCTimestamp(), std::invalid_argument);
  }

  // some numeric values, disguised as strings
  // that will be wrapped in quotes for the tests below
  sList = {
    {" ", "1"},
    {"1.1", "1.1"},
    {"+42", "+42"},
    {"-6.66", "-6.66"},
  };

  for (const pair<string,string>& p : sList)
  {
    CSV_Value v{"\"" + p.second + "\""};

    ASSERT_FALSE(v.isNull());
    ASSERT_TRUE(v.hasNumericValue());
    ASSERT_NO_THROW(v.asString());
    ASSERT_NO_THROW(v.asStringRef());
    ASSERT_NO_THROW(v.asInt());
    ASSERT_NO_THROW(v.asLong());
    ASSERT_NO_THROW(v.asDouble());
  }

  // direct, unquoted / unescaped strings
  CSV_Value v{"abc,\\"};
  ASSERT_FALSE(v.isNull());
  ASSERT_FALSE(v.hasNumericValue());
  ASSERT_EQ("abc,\\", v.asString());
  ASSERT_EQ("abc,\\", v.asStringRef());
  ASSERT_THROW(v.asInt(), std::invalid_argument);
  ASSERT_THROW(v.asLong(), std::invalid_argument);
  ASSERT_THROW(v.asDouble(), std::invalid_argument);
  ASSERT_THROW(v.asUTCTimestamp(), std::invalid_argument);

  v = CSV_Value{""};  // empty string is not null, it's an empty string
  ASSERT_FALSE(v.isNull());
  ASSERT_FALSE(v.hasNumericValue());
  ASSERT_EQ("", v.asString());
  ASSERT_EQ("\"\"", v.toString());
}

//----------------------------------------------------------------------------

TEST(CSV, CSV_Value_numeric)
{
  CSV_Value v{1};
  ASSERT_TRUE(v.hasNumericValue());
  ASSERT_EQ(1, v.asInt());
  ASSERT_EQ("1", v.asString());

  v = CSV_Value{-3.456};
  ASSERT_TRUE(v.hasNumericValue());
  ASSERT_EQ(-3.456, v.asDouble());
}

//----------------------------------------------------------------------------

TEST(CSV, CSV_Value_numericString)
{
  CSV_Value v{"1"};
  ASSERT_TRUE(v.hasNumericValue());
  ASSERT_EQ(1, v.asInt());

  v = CSV_Value{"-1.234"};
  ASSERT_TRUE(v.hasNumericValue());
  ASSERT_EQ(-1, v.asInt());
  ASSERT_EQ(-1.234, v.asDouble());

  v = CSV_Value{"+666"};
  ASSERT_TRUE(v.hasNumericValue());
  ASSERT_EQ(666, v.asInt());
  ASSERT_EQ(666.0, v.asDouble());
}

//----------------------------------------------------------------------------

TEST(CSV, CSV_Row_funcs)
{
  CSV_Row r;
  ASSERT_TRUE(r.empty());
  ASSERT_EQ(0, r.getColCount());

  r = CSV_Row{""};
  ASSERT_TRUE(r.empty());
  ASSERT_EQ(0, r.getColCount());

  r = CSV_Row{" "};
  ASSERT_FALSE(r.empty());
  ASSERT_EQ(1, r.getColCount());

  r = CSV_Row{","};
  ASSERT_FALSE(r.empty());
  ASSERT_EQ(2, r.getColCount());
  ASSERT_TRUE(r[0].isNull());
  ASSERT_TRUE(r[1].isNull());

  r = CSV_Row{"a"};
  ASSERT_FALSE(r.empty());
  ASSERT_EQ(1, r.getColCount());
  ASSERT_FALSE(r[0].isNull());

  r = CSV_Row{"a, b  ,\"  c \\,  \", d,"};
  ASSERT_FALSE(r.empty());
  ASSERT_EQ(5, r.getColCount());
  ASSERT_FALSE(r[0].isNull());
  ASSERT_FALSE(r[1].isNull());
  ASSERT_FALSE(r[2].isNull());
  ASSERT_FALSE(r[3].isNull());
  ASSERT_TRUE(r[4].isNull());
  ASSERT_EQ(" b  ", r[1].asString());  // no trimming, even for un-quoted strings
  ASSERT_EQ("  c ,  ", r[2].asString());  // check quotes and escaping commas
}

//----------------------------------------------------------------------------

TEST(CSV, CSV_Table_noHeader)
{
  CSV_Table t;
  ASSERT_TRUE(t.empty());
  ASSERT_EQ(0, t.getRowCount());

  // valid empty table
  t = CSV_Table{"\n", false};
  ASSERT_TRUE(t.empty());
  ASSERT_EQ(0, t.getRowCount());

  // valid table without headers
  t = CSV_Table{"1,2,3\n4,5,6", false};
  ASSERT_EQ(2, t.getRowCount());
  ASSERT_EQ(5, t.getVal(1, 1).asInt());

  // invalid table with wrong column count
  ASSERT_THROW(
        CSV_Table("1,2,3\n4,5", false),
        CSV_InvalidDataException
        );
  ASSERT_THROW(
        CSV_Table("1,2,3\n4,5,6,7,8", false),
        CSV_InvalidDataException
        );
}

//----------------------------------------------------------------------------

TEST(CSV, CSV_Table_withHeaders)
{
  // valid table with headers but no data
  CSV_Table t{"a,b,c", true};
  ASSERT_EQ(0, t.getRowCount());
  ASSERT_TRUE(t.empty());

  t = CSV_Table{"a,b,c\n", true};
  ASSERT_EQ(0, t.getRowCount());
  ASSERT_TRUE(t.empty());

  // valid table with headers and data
  t = CSV_Table{"a,b,c\n1,2,3", true};
  ASSERT_EQ(1, t.getRowCount());
  ASSERT_FALSE(t.empty());
  ASSERT_EQ("2", t[0][1].asString());

  t = CSV_Table{"a,b,c\n1,2,3\n", true};
  ASSERT_EQ(1, t.getRowCount());
  ASSERT_FALSE(t.empty());
  ASSERT_EQ(2, t[0][1].asInt());

  // invalid column count
  ASSERT_THROW(
        CSV_Table("a,b,c\n1,2,3\n4,5", true),
        CSV_InvalidDataException
        );

  // invalid column header
  ASSERT_THROW(
        CSV_Table("a,b,\n1,2,3", true),
        CSV_InvalidRowHeaderException
        );
  ASSERT_THROW(
        CSV_Table("a,,c\n1,2,3", true),
        CSV_InvalidRowHeaderException
        );
}

//----------------------------------------------------------------------------

TEST(CSV, ConstructionFromScratch)
{
  CSV_Table t{{"a,b,c,d"}, true};

  CSV_Row r;
  r.append(" ");
  r.append();
  r.append(42);
  r.append(6.66);
  t.append(r);

  ASSERT_EQ("a, b, c, d\n\" \",,42,6.660000\n", t.to_string());
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

