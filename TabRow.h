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

#ifndef SQLITE_OVERLAY_TABROW_H
#define	SQLITE_OVERLAY_TABROW_H

#include <Sloppy/String.h>
#include <Sloppy/ConfigFileParser/ConstraintChecker.h>

#include "SqliteDatabase.h"
#include "ClausesAndQueries.h"

namespace greg = boost::gregorian;

namespace SqliteOverlay
{

  /** \brief A class representing a single data row in a table (not in a view!)
   */
  class TabRow
  {
  public:
    /** \brief Ctor for a single row with a known ID
     *
     * Normally, the validity of the ID will be checked by executing a
     * SELECT query for that row. If, however, the ID is guarenteed to
     * be correct (e.g., because the source is another SELECT statement
     * that has just been executed), the check can be skiped and thus
     * the penalty for constructing a new TabRow object can be reduced.
     *
     * \throws std::invalid_argument if the table name
     * is empty or if the provided row ID was invalid
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    TabRow (
        const SqliteDatabase& db,   ///< the database that contains the table
        const string& _tabName,  ///< the table name
        int _rowId,   ///< the ID of the row, has to be in a column named "id"
        bool skipCheck = false   ///< if `true` the validity of the ID (read: the actual existence of the row) will *not* be checked
        );

    /**
     * \brief Constructor for the first row in the table that matches a custom WHERE clause.
     *
     * \throws std::invalid_argument if the table name or the WHERE clause is empty
     * or if the provided WHERE didn't produce any results
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    TabRow (
        const SqliteDatabase& db,   ///< the database that contains the table
        const string& _tabName,  ///< the table name
        const WhereClause& where   ///< the WHERE clause that identifies the row
        );

    /** \brief Empty default dtor
     */
    virtual ~TabRow () {}

    /** \brief Copy operator, makes us point to the same row as 'other'
     *
     * Test case: yes
     *
     */
    TabRow& operator=(const TabRow& other);

    /** \brief Copy ctor, creates a new object that points to the same row as 'other'
     *
     * Test case: yes
     *
     */
    TabRow(const TabRow& other);

    /** \brief Move operator, invalidates the source row
     *
     * Test case: yes
     *
     */
    TabRow& operator=(TabRow&& other);

    /** \brief Move ctor, invalidates the source row
     *
     * Test case: yes
     *
     */
    TabRow(TabRow&& other);

    /** \returns the row's rowid-value
     *
     * Test case: yes
     *
     */
    int id() const;

    /** \brief Updates one or more columns of the row to new values
     *
     * If the ColumnValueClause doesn't contain any data, we simply
     * do nothing and return immediately.
     *
     * \throws BusyException if the database wasn't available for
     * updating the columns
     *
     * \throws std::invalid_argument if the ColumnValueClause contained invalid column names
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    void update(
        const ColumnValueClause& cvc   ///< ColumnValueClause with a list of column-value-pairs for the update
        ) const;

    /** \brief Updates a given column of the row to a new value; works for
     * the standard types int, double, float, bool, string
     *
     * \throws BusyException if the database wasn't available for
     * updating the column
     *
     * \throws std::invalid_argument if the provided column doesn't exist
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    template<typename T>
    void update(
        const string& colName,   ///< the name of the column to modify
        const T& newVal   ///< the new value for that column
        ) const
    {
      if (colName.empty())
      {
        throw std::invalid_argument("Tabrow::update(): empty column name");
      }

      Sloppy::estring sql{cachedUpdateStatementForRow};
      sql.arg(colName);

      SqlStatement stmt;
      try
      {
        stmt = db.get().prepStatement(sql);
      }
      catch (SqlStatementCreationError)
      {
        throw std::invalid_argument("Tabrow::update(): invalid column name");
      }
      catch (...)
      {
        throw;
      }

      stmt.bind(1, newVal);
      db.get().execNonQuery(stmt);
    }

    /** \brief Updates a given column of the row to NULL
     *
     * \throws BusyException if the database wasn't available for
     * updating the column
     *
     * \throws std::invalid_argument if the provided column doesn't exist
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    void updateToNull(
        const string& colName   ///< the name of the column to modify
        ) const;

    /** \returns a string with the contents of a given column
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * Test case: yes
     *
     */
    string operator[](
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column in various types
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL and the result type isn't `optional<...>`
     *
     * Test case: yes, as part of the getXXX functions
     *
     */
    template<typename T>
    T get(
        const string& colName   ///< the name of the column to query
        ) const
    {
      if (colName.empty())
      {
        throw std::invalid_argument("Column access: received empty column name");
      }

      SqlStatement stmt;
      string sql = "SELECT " + colName + cachedWhereStatementForRow;
      try
      {
        stmt = db.get().prepStatement(sql);
      }
      catch (SqlStatementCreationError)
      {
        throw std::invalid_argument("Column access: received invalid column name");
      }
      catch (...)
      {
        throw;
      }

      stmt.step();

      T outVal;
      stmt.get(0, outVal);

      return outVal;
    }

    /** \returns the contents of a given column as an integer
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * Test case: yes
     *
     */
    int getInt(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as an integer
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * Test case: yes
     *
     */
    long getLong(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as an integer
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * Test case: yes
     *
     */
    double getDouble(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a timestamp in local time
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * Test case: not yet
     *
     */
    LocalTimestamp getLocalTime(
        const string& colName,   ///< the name of the column to query
        boost::local_time::time_zone_ptr tzp   ///< pointer to the timezone info for constructing the LocalTimestamp object
        ) const;

    /** \returns the contents of a given column as a timestamp in UTC
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * Test case: yes
     *
     */
    UTCTimestamp getUTCTime(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a JSON object
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * \throws nlohmann::json::parse_error if the column didn't contain valid JSON data
     *
     * Test case: yes
     *
     */
    nlohmann::json getJson(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a date
     *
     * \note The date has to be stored as an integer value, e.g.,
     * 2018-10-28 should be stored as the integer value "20181028".
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * \throws NullValueException if the column contained NULL
     *
     * Test case: yes
     *
     */
    greg::date getDate(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as an integer or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * Test case: yes
     *
     */
    optional<int> getInt2(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a long integer or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * Test case: yes
     *
     */
    optional<long> getLong2(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a double or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * Test case: yes
     *
     */
    optional<double> getDouble2(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a string or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * Test case: yes
     *
     */
    optional<string> getString2(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a LocalTimestamp
     * or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * Test case: not yet
     *
     */
    optional<LocalTimestamp> getLocalTime2(
        const string& colName,   ///< the name of the column to query
        boost::local_time::time_zone_ptr tzp   ///< pointer to the local time zone description
        ) const;

    /** \returns the contents of a given column as a UTCTimestamp
     * or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     */
    optional<UTCTimestamp> getUTCTime2(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a JSON object
     * or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     */
    optional<nlohmann::json> getJson2(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \returns the contents of a given column as a date
     * or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     *
     * Test case: yes
     *
     */
    optional<greg::date> getDate2(
        const string& colName   ///< the name of the column to query
        ) const;

    /** \brief Overload operator for "is equal", compares table name,
     * row ID and path to the database file
     *
     *
     * Test case: yes
     *
     */
    inline bool operator== (const TabRow& other) const
    {
      return ((other.tabName == tabName) && (other.rowId == rowId) && (other.db.get() == db.get()));  // enforce the same db file!
    }

    /** \brief Overload operator for "is not equal", compares table name,
     * and row ID.
     *
     *
     * Test case: yes
     *
     */
    inline bool operator!= (const TabRow& other) const
    {
      return (!(this->operator == (other)));
    }

    /** \returns the reference to the database we're using
     *
     * Test case: not yet
     *
     */
    const SqliteDatabase& getDb() const;

    /** \brief Erases the row from the table.
     *
     * \warning This TabRow instance may not be used anymore
     * after calling `erase()`!
     *
     * \throws BusyException if the database wasn't available for
     * deleting the column
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    void erase() const;

    /** \returns two column values with a single SELECT statement / a single database query
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NullValueException if the column contained NULL and the requested type is not `optional<...>`
     *
     * Test case: yes
     *
     */
    template<typename T1, typename T2>
    void multiGetByRef(
        const string& colName1,   ///< name of the first column to get
        T1& out1,   ///< reference for storing the value of the first column
        const string& colName2,   ///< name of the second column to get
        T2& out2   ///< reference for storing the value of the second column
        ) const
    {
      SqlStatement stmt = getSelectStatementWithColumnChecking(colName1, colName2);
      stmt.step();
      stmt.multiGet(0, out1, 1, out2);
    }
    
    /** \returns three column values with a single SELECT statement / a single database query
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NullValueException if the column contained NULL and the requested type is not `optional<...>`
     *
     * Test case: yes
     *
     */
    template<typename T1, typename T2, typename T3>
    void multiGetByRef(
        const string& colName1,
        T1& out1,
        const string& colName2,
        T2& out2,
        const string& colName3,
        T3& out3
        ) const
    {
      SqlStatement stmt = getSelectStatementWithColumnChecking(colName1, colName2, colName3);
      stmt.step();
      stmt.multiGet(0, out1, 1, out2, 2, out3);
    }

    /** \returns four column values with a single SELECT statement / a single database query
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NullValueException if the column contained NULL and the requested type is not `optional<...>`
     *
     * Test case: yes
     *
     */
    template<typename T1, typename T2, typename T3, typename T4>
    void multiGetByRef(
        const string& colName1,
        T1& out1,
        const string& colName2,
        T2& out2,
        const string& colName3,
        T3& out3,
        const string& colName4,
        T4& out4
        ) const
    {
      SqlStatement stmt = getSelectStatementWithColumnChecking(colName1, colName2, colName3, colName4);
      stmt.step();
      stmt.multiGet(0, out1, 1, out2, 2, out3, 3, out4);
    }

    /** \returns two column values with a single SELECT statement / a single database query
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NullValueException if the column contained NULL and the requested type is not `optional<...>`
     *
     * Test case: yes
     *
     */
    template<typename T1, typename T2>
    tuple<T1,T2> multiGetAsTuple(
        const string& colName1,
        const string& colName2
        ) const
    {
      SqlStatement stmt = getSelectStatementWithColumnChecking(colName1, colName2);
      stmt.step();
      return stmt.tupleGet<T1, T2>(0, 1);
    }

    /** \returns three column values with a single SELECT statement / a single database query
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NullValueException if the column contained NULL and the requested type is not `optional<...>`
     *
     * Test case: yes
     *
     */
    template<typename T1, typename T2, typename T3>
    tuple<T1,T2, T3> multiGetAsTuple(
        const string& colName1,
        const string& colName2,
        const string& colName3
        ) const
    {
      SqlStatement stmt = getSelectStatementWithColumnChecking(colName1, colName2, colName3);
      stmt.step();
      return stmt.tupleGet<T1, T2, T3>(0, 1, 2);
    }

    /** \returns four column values with a single SELECT statement / a single database query
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NullValueException if the column contained NULL and the requested type is not `optional<...>`
     *
     * Test case: yes
     *
     */
    template<typename T1, typename T2, typename T3, typename T4>
    tuple<T1,T2, T3, T4> multiGetAsTuple(
        const string& colName1,
        const string& colName2,
        const string& colName3,
        const string& colName4
        ) const
    {
      SqlStatement stmt = getSelectStatementWithColumnChecking(colName1, colName2, colName3, colName4);
      stmt.step();
      return stmt.tupleGet<T1, T2, T3, T4>(0, 1, 2, 3);
    }

    /** \brief Checks whether a cell's content satisfies a given constraint, e.g. "alphanumeric".
     *
     * \note Consider calling this function from within a transaction in order to avoid that the
     * constraint is violated by another database connection after the check.
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \returns `true` if the constraint is satisfied, 'false' otherwise.
     */
    bool checkConstraint(
        const string& colName,   ///< the name of the column to check
        Sloppy::ValueConstraint c,   ///< the constraint to check the column content against
        string* errMsg = nullptr   ///< optional pointer to a string in which an english error message in case of a constraint violation is stored
        ) const;

  protected:
    void genCommaSepString(string& target, const string& element) const
    {
      if (element.empty()) return;
      if (!target.empty()) target += ",";
      target += element;
    }

    template<typename T, typename... Targs>
    void genCommaSepString(string& target, const T& element, Targs... FuncArgs) const
    {
      genCommaSepString(target, element);
      genCommaSepString(target, FuncArgs...);
    }

    template<typename... Targs>
    SqlStatement getSelectStatementWithColumnChecking(Targs... FuncArgs) const
    {
      string colNames;
      genCommaSepString(colNames, FuncArgs...);
      string sql = "SELECT " + colNames + cachedWhereStatementForRow;

      SqlStatement stmt;
      try
      {
        stmt = db.get().prepStatement(sql);
      }
      catch (SqlStatementCreationError)
      {
        throw std::invalid_argument("Column access: received invalid column name");
      }
      catch (...)
      {
        throw;
      }

      return stmt;
    }

    /**
     * the handle to the (parent) database
     */
    reference_wrapper<const SqliteDatabase> db;
    
    /**
     * the name of the associated table
     */
    string tabName;
    
    /**
     * the unique index of the data row
     */
    int rowId;

    string cachedWhereStatementForRow;
    Sloppy::estring cachedUpdateStatementForRow;
  };

}

#endif	/* TABROW_H */

