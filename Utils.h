/*
 * Copyright © 2017 Volker Knollmann
 *
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file or visit
 * http://www.wtfpl.net/ for more details.
 *
 * This program comes without any warranty. Use it at your own risk or
 * don't use it at all.
 */

#ifndef SQLITEOVERLAY_UTILS_H
#define SQLITEOVERLAY_UTILS_H

#include <string>
#include <vector>

using namespace std;

// forward
namespace Sloppy {
  namespace DateTime {
    class UTCTimestamp;
  }
}

//----------------------------------------------------------------------------

namespace SqliteOverlay
{
  class CSV_InvalidDataException {};
  class CSV_NullValueException {};
  class CSV_NoRowHeadersException {};
  class CSV_InvalidRowHeaderException {};

  //----------------------------------------------------------------------------

  string quoteAndEscapeStringForCSV(const string& inData);

  string unquoteAndUnescapeStringFromCSV(const string& inData);

  //----------------------------------------------------------------------------

  class CSV_Value
  {
  public:
    CSV_Value();  // constructs a NULL value
    CSV_Value(const string& inData);     // NOTE: empty string is NOT null, it is a valid string!
    CSV_Value(int i);
    CSV_Value(double d);
    CSV_Value(const Sloppy::DateTime::UTCTimestamp& utc);
    CSV_Value(long l);

    // getters
    bool isNull() const { return hasNullValue; }
    bool hasNumericValue() const;

    string asString() const;
    const string& asStringRef() const;
    int asInt() const;  // throws if content is invalid
    long asLong() const;  // throws if content is invalid
    double asDouble() const;  // throws if content is invalid
    Sloppy::DateTime::UTCTimestamp asUTCTimestamp() const;  // throws if content is invalid

    string toString() const;

  private:
    string val;
    bool hasNullValue;
    bool isString;
  };

  //----------------------------------------------------------------------------

  class CSV_Row
  {
  public:
    CSV_Row() {}  // constructs a row with zero fields
    CSV_Row(const string& commaSepQuotedAndEscapedValues);

    const CSV_Value& operator[](int colIdx) const;  // throws if idx is invalid

    size_t getColCount() const { return cols.size(); }
    bool empty() const { return cols.empty(); }

    // appending columns
    void append(const string& s);
    void append(int i);
    void append(double d);
    void append(const Sloppy::DateTime::UTCTimestamp& utc);
    void append();  // appends a NULL column
    void append(const CSV_Value& v);

    // convert to string
    string to_string() const;

  private:
    vector<CSV_Value> cols;
  };

  //----------------------------------------------------------------------------

  class CSV_Table
  {
  public:
    CSV_Table() : colCount{0} {}  // constructs empty tables without data and without column names
    CSV_Table(const string& tableData, bool firstLineContainsHeaders);

    const CSV_Row& operator[](int rowIdx) const;
    const CSV_Value& getVal(int rowIdx, int colIdx);
    const CSV_Value& getVal(int rowIdx, const string& colName);

    size_t getRowCount() const { return rows.size(); }
    bool empty() const { return rows.empty(); }

    void append(const CSV_Row& r);  // throws if r has the wrong column count

    string to_string() const;

  private:
    size_t colCount;
    vector<string> colNames;
    vector<CSV_Row> rows;
  };
}
#endif
