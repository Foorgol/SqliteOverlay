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

#include <regex>

#include <Sloppy/libSloppy.h>
#include <Sloppy/DateTime/DateAndTime.h>

#include <boost/algorithm/string.hpp>

#include "Utils.h"

namespace SqliteOverlay
{

  string quoteAndEscapeStringForCSV(const string& inData)
  {
    // replace all backslashes with a double backslash
    regex reBackslash{"\\\\"};  // matches a single backslash
    string result = regex_replace(inData, reBackslash, "\\\\");  // insert double backslash

    // replace " with \"
    regex reQuote{"\""}; // matches a single "
    result = regex_replace(result, reQuote, "\\\"");  // inserts \"

    // replace , with \,
    regex reComma{","}; // matches a single ','
    result = regex_replace(result, reComma, "\\,");  // inserts \,

    // replace newlines with a literal "\n"
    regex reNewline{"\n"}; // matches a single \n
    result = regex_replace(result, reNewline, "\\n");  // inserts a literal "\n"

    return "\"" + result + "\"";
  }

  //----------------------------------------------------------------------------

  string unquoteAndUnescapeStringFromCSV(const string& inData)
  {
    // the string has to start and end with '"'
    string d = boost::trim_copy(inData);
    if (d.size() < 2) throw CSV_InvalidDataException();
    if (d[0] != '"') throw CSV_InvalidDataException();
    if (d[d.size() - 1] != '"') throw CSV_InvalidDataException();

    // remove starting and trailing quotes
    if (d.length() < 2) return "";
    string result = d.substr(1, d.length() - 2);

    // make sure there no un-escaped commas, quotes and backslashes in the string
    for (char c : {'"', ',', '\\'})
    {
      size_t idx = result.find(c);
      while (idx != string::npos)
      {
        if ((idx == 0) && (c != '\\')) throw CSV_InvalidDataException();
        if ((c != '\\') && (idx > 0) && (result[idx - 1] != '\\')) throw CSV_InvalidDataException();

        idx = result.find(c, idx+1);
      }
    }

    // unescape newlines
    regex reNewline{"\\\\n"};
    result = regex_replace(result, reNewline, "\n");

    // unescape commas
    regex reComma{"\\\\,"};
    result = regex_replace(result, reComma, ",");

    // unescape quotes
    regex reQuote{"\\\\\""};
    result = regex_replace(result, reQuote, "\"");

    // unescape backslashes
    regex reBackslash{"\\\\\\\\"};
    result = regex_replace(result, reBackslash, "\\");

    return result;
  }

  //----------------------------------------------------------------------------

  CSV_Value::CSV_Value()
    :hasNullValue{true}, isString{false} {}

  //----------------------------------------------------------------------------

  CSV_Value::CSV_Value(const string& inData)
    :hasNullValue{false}, isString{true}
  {
    // if it starts with '"' it has to be a quoted string
    if (inData[0] == '"')
    {
      val = unquoteAndUnescapeStringFromCSV(inData);  // throws if we got invalid data
    } else {
      val = inData;   // unquoted string
    }
  }

  //----------------------------------------------------------------------------

  CSV_Value::CSV_Value(int i)
    :hasNullValue{false}, isString{false}
  {
    val = to_string(i);
  }

  //----------------------------------------------------------------------------

  CSV_Value::CSV_Value(double d)
    :hasNullValue{false}, isString{false}
  {
    val = to_string(d);
  }

  //----------------------------------------------------------------------------

  CSV_Value::CSV_Value(const Sloppy::DateTime::UTCTimestamp& utc)
    :hasNullValue{false}, isString{false}
  {
    val = to_string(utc.getRawTime());
  }

  //----------------------------------------------------------------------------

  CSV_Value::CSV_Value(long l)
    :hasNullValue{false}, isString{false}
  {
    val = to_string(l);
  }

  //----------------------------------------------------------------------------

  bool CSV_Value::hasNumericValue() const
  {
    if (hasNullValue) return false;

    // this return "true" for any valid int or double
    return Sloppy::isDouble(val);
  }

  //----------------------------------------------------------------------------

  string CSV_Value::asString() const
  {
    if (hasNullValue) throw CSV_NullValueException();

    return val;
  }

  //----------------------------------------------------------------------------

  const string&CSV_Value::asStringRef() const
  {
    if (hasNullValue) throw CSV_NullValueException();

    return val;
  }

  //----------------------------------------------------------------------------

  int CSV_Value::asInt() const
  {
    if (hasNullValue) throw CSV_NullValueException();

    return stoi(val);
  }

  //----------------------------------------------------------------------------

  long CSV_Value::asLong() const
  {
    if (hasNullValue) throw CSV_NullValueException();

    return stol(val);
  }

  //----------------------------------------------------------------------------

  double CSV_Value::asDouble() const
  {
    if (hasNullValue) throw CSV_NullValueException();

    return stod(val);
  }

  //----------------------------------------------------------------------------

  Sloppy::DateTime::UTCTimestamp CSV_Value::asUTCTimestamp() const
  {
    if (hasNullValue) throw CSV_NullValueException();

    time_t utc = asLong();
    return Sloppy::DateTime::UTCTimestamp(utc);
  }

  //----------------------------------------------------------------------------

  string CSV_Value::toString() const
  {
    if (hasNullValue) return "";
    if (isString) return quoteAndEscapeStringForCSV(val);

    return val;
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  CSV_Row::CSV_Row(const string& commaSepQuotedAndEscapedValues)
  {
    if (commaSepQuotedAndEscapedValues.empty()) return;

    // find all comma positions
    vector<size_t> commaPos;
    size_t minIdx = 0;
    while (true)
    {
      size_t idx = commaSepQuotedAndEscapedValues.find(',', minIdx);

      // no more commas found
      if (idx == string::npos) break;

      // search starting point for next iteration
      minIdx = idx + 1;

      // is this an escaped comma?
      if ((idx > 0) && (commaSepQuotedAndEscapedValues[idx-1] == '\\'))
      {
        continue;   // skip escaped commas
      }

      // valid, field separating comma
      commaPos.push_back(idx);
    }

    // initialize column values from comma-delimited data
    size_t idxStart = 0;
    for (size_t idx : commaPos)
    {
      // row starts with a NULL value or
      // intermediate NULL value "xxxx,,xxxx"
      if (idx == idxStart)
      {
        cols.push_back(CSV_Value());
      } else {
        // regular content field
        string val = Sloppy::getStringSlice(commaSepQuotedAndEscapedValues, idxStart, idx - 1);
        cols.push_back(CSV_Value(val));
      }

      // start position for next slice is one behind the comma
      idxStart = idx + 1;

      // if the row ended with a comma, we have to push
      // another NULL value
      if (idxStart == commaSepQuotedAndEscapedValues.size())
      {
        cols.push_back(CSV_Value());
      }
    }

    // add any remaining characters after the last comma
    if (idxStart <= (commaSepQuotedAndEscapedValues.size() - 1))
    {
      string val = commaSepQuotedAndEscapedValues.substr(idxStart);
      cols.push_back(CSV_Value(val));
    }
  }

  //----------------------------------------------------------------------------

  const CSV_Value& CSV_Row::operator[](int colIdx) const
  {
    return cols.at(colIdx);
  }

  //----------------------------------------------------------------------------

  void CSV_Row::append(const string& s)
  {
    cols.push_back(CSV_Value{s});
  }

  //----------------------------------------------------------------------------

  void CSV_Row::append(int i)
  {
    cols.push_back(CSV_Value{i});
  }

  //----------------------------------------------------------------------------

  void CSV_Row::append(double d)
  {
    cols.push_back(CSV_Value{d});
  }

  //----------------------------------------------------------------------------

  void CSV_Row::append(const Sloppy::DateTime::UTCTimestamp& utc)
  {
    cols.push_back(CSV_Value{utc});
  }

  //----------------------------------------------------------------------------

  void CSV_Row::append()
  {
    cols.push_back(CSV_Value{});
  }

  //----------------------------------------------------------------------------

  void CSV_Row::append(const CSV_Value& v)
  {
    cols.push_back(v);
  }

  //----------------------------------------------------------------------------

  string CSV_Row::to_string() const
  {
    string result;

    for (const CSV_Value& v : cols)
    {
      result += v.toString();
      result += ",";
    }

    return result.substr(0, result.size() - 1);
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  CSV_Table::CSV_Table(const string& tableData, bool firstLineContainsHeaders)
    :colCount{0}
  {
    if (tableData.empty()) return;

    // split at all newline positions
    Sloppy::StringList lines;
    Sloppy::stringSplitter(lines, tableData, "\n", true);

    // erase empty lines
    auto it = lines.begin();
    while (it != lines.end())
    {
      if ((*it).empty())
      {
        it = lines.erase(it);
      } else {
        ++it;
      }
    }

    if (lines.empty()) return;

    // store the column names, if any
    if (firstLineContainsHeaders)
    {
      string hdrLine = lines[0];
      Sloppy::stringSplitter(colNames, hdrLine, ",", true);
      lines.erase(lines.begin());

      // make sure there are no empty column names
      for (const string& n : colNames)
      {
        if (n.empty()) throw CSV_InvalidRowHeaderException();
      }

      // also, the header line should not end with a comma
      if (hdrLine[hdrLine.size() - 1] == ',') throw CSV_InvalidRowHeaderException();

      // store the number of columns as defined
      // by the header line
      colCount = colNames.size();
    }

    // iterate over the remaining lines
    // and construct CSV_Rows from the content
    it = lines.begin();
    while (it != lines.end())
    {
      CSV_Row r{*it};

      // if we have no headers, the first row
      // defines the number of columns for the
      // whole table
      if (colCount == 0) colCount = r.getColCount();

      // throw if the column count of this row does not
      // match the required column count
      if (r.getColCount() != colCount)
      {
        throw CSV_InvalidDataException();
      }

      // store the row
      rows.push_back(r);

      ++it;
    }
  }

  //----------------------------------------------------------------------------

  const CSV_Row& CSV_Table::operator[](int rowIdx) const
  {
    return rows.at(rowIdx);
  }

  //----------------------------------------------------------------------------

  const CSV_Value& CSV_Table::getVal(int rowIdx, int colIdx)
  {
    return rows.at(rowIdx).operator [](colIdx);
  }

  //----------------------------------------------------------------------------

  const CSV_Value& CSV_Table::getVal(int rowIdx, const string& colName)
  {
    if (colNames.empty())
    {
      throw CSV_NoRowHeadersException{};
    }

    auto it = std::find(colNames.begin(), colNames.end(), colName);
    if (it == colNames.end())
    {
      throw CSV_InvalidRowHeaderException();
    }

    size_t colIdx = it - colNames.begin();

    return getVal(rowIdx, colIdx);
  }

  //----------------------------------------------------------------------------

  void CSV_Table::append(const CSV_Row& r)
  {
    // appending empty rows is always okay
    if (r.empty())
    {
      rows.push_back(r);
      return;
    }

    // for non-empty rows, check whether the column count matches
    if ((colCount > 0) && (r.getColCount() != colCount)) throw CSV_InvalidDataException();

    if (colCount == 0) // appending to an empty table
    {
      colCount = r.getColCount();
    }

    rows.push_back(r);
  }

  //----------------------------------------------------------------------------

  string CSV_Table::to_string() const
  {
    string result;

    if (!(colNames.empty()))
    {
      result = Sloppy::commaSepStringFromStringList(colNames) + "\n";
    }

    for (const CSV_Row& r : rows)
    {
      result += r.to_string() + "\n";
    }

    return result;
  }

}
