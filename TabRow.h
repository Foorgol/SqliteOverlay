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

#pragma once

#include <stdint.h>                                     // for int64_t
#include <Sloppy/json.hpp>                              // for json
#include <chrono>                                       // for duration
#include <functional>                                   // for reference_wra...
#include <optional>                                     // for optional
#include <stdexcept>                                    // for invalid_argument
#include <string>                                       // for string, alloc...
#include <tuple>                                        // for tuple
#include <vector>                                       // for vector

#include <Sloppy/CSV.h>                                 // for CSV_Row
#include <Sloppy/ConfigFileParser/ConstraintChecker.h>  // for ValueConstraint
#include <Sloppy/DateTime/DateAndTime.h>                // for WallClockTime...
#include <Sloppy/DateTime/date.h>                       // for year_month_day
#include <Sloppy/Memory.h>                              // for MemArray
#include <Sloppy/String.h>                              // for estring

#include "SqlStatement.h"                               // for SqlStatement
#include "SqliteDatabase.h"                             // for SqliteDatabase
#include "SqliteExceptions.h"                           // for SqlStatementC...

namespace SqliteOverlay
{

  class ColumnValueClause;
  class WhereClause;

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
        const std::string& _tabName,  ///< the table name
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
        const std::string& _tabName,  ///< the table name
        const WhereClause& where   ///< the WHERE clause that identifies the row
        );

    /** \brief Empty default dtor
     */
    ~TabRow () {}

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
        const std::string& colName,   ///< the name of the column to modify
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
        const std::string& colName   ///< the name of the column to modify
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
    std::string operator[](
        const std::string& colName   ///< the name of the column to query
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
        const std::string& colName   ///< the name of the column to query
        ) const
    {
      if (colName.empty())
      {
        throw std::invalid_argument("Column access: received empty column name");
      }

      SqlStatement stmt;
      std::string sql = "SELECT " + colName + cachedWhereStatementForRow;
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

      return stmt.get<T>(0);
    }

    template<typename T>
    std::optional<T> get2(
        const std::string& colName   ///< the name of the column to query
        ) const
    {
      if (colName.empty())
      {
        throw std::invalid_argument("Column access: received empty column name");
      }

      SqlStatement stmt;
      std::string sql = "SELECT " + colName + cachedWhereStatementForRow;
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

      return stmt.get2<T>(0);
    }


    /** \returns the contents of a given column as a duration of custom granularity
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
    template<class RepType, class PeriodRatio>
    Sloppy::DateTime::WallClockTimepoint_secs getDuration(
        const std::string& colName   ///< the name of the column to query
        ) const
    {
      if constexpr (std::is_same_v<RepType, int>) {
        return std::chrono::duration<RepType, PeriodRatio>(get<int>(colName));
      }
      if constexpr (std::is_same_v<RepType, int64_t>) {
        return std::chrono::duration<RepType, PeriodRatio>(get<int64_t>(colName));
      }
      if constexpr (std::is_same_v<RepType, double>) {
        return std::chrono::duration<RepType, PeriodRatio>(get<double>(colName));
      }

      static_assert (alwaysFalse<RepType>, "Unsupported RepType for TabRow::getDuration()");
    }

    /** \returns the contents of a given column as a timestamp
     *
     *  The column has to contain the timestamp in seconds since the
     *  UNIX epoch (which is more or less UTC).
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
    Sloppy::DateTime::WallClockTimepoint_secs get(
        const std::string& colName,   ///< the name of the column to query
        date::time_zone* tzp   ///< pointer to the timezone info that is passed to the ctor of the WallClockTimepoint
        ) const
    {
      const time_t rawTime = get<int64_t>(colName);
      return Sloppy::DateTime::WallClockTimepoint_secs{rawTime, tzp};
    }

    /** \returns the contents of a given column as a timestamp
     * or empty if the column was empty
     *
     *  The column has to contain the timestamp in seconds since the
     *  UNIX epoch (which is more or less UTC).
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
    std::optional<Sloppy::DateTime::WallClockTimepoint_secs> get2(
        const std::string& colName,   ///< the name of the column to query
        date::time_zone* tzp   ///< pointer to the local time zone description
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
    std::tuple<T1,T2> multiGetAsTuple(
        const std::string& colName1,
        const std::string& colName2
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
    std::tuple<T1,T2, T3> multiGetAsTuple(
        const std::string& colName1,
        const std::string& colName2,
        const std::string& colName3
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
    std::tuple<T1,T2, T3, T4> multiGetAsTuple(
        const std::string& colName1,
        const std::string& colName2,
        const std::string& colName3,
        const std::string& colName4
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
        const std::string& colName,   ///< the name of the column to check
        Sloppy::ValueConstraint c,   ///< the constraint to check the column content against
        std::string* errMsg = nullptr   ///< optional pointer to a string in which an english error message in case of a constraint violation is stored
        ) const;

    /** \brief Exports all columns of the row as a CSV_Row
     *
     * \throws InvalidColumnException if any of the columns contains BLOB data because
     * that can't be properly exported to CSV.
     *
     * \returns a CSV_Row instance containing a copy of the contents of the row
     */
    Sloppy::CSV_Row toCSV(
        bool includeRowId   ///< if `true`, the first column of the CSV row will be `rowid`
        ) const;

    /** \brief Exports selected columns of the row as a CSV_Row
     *
     * \throws InvalidColumnException if any of the columns contains BLOB data because
     * that can't be properly exported to CSV.
     *
     * \throws SqlStatementCreationError if the provided column list contained
     * an invalid column name.
     *
     * \returns a CSV_Row instance containing a copy of the contents of the row; empty,
     * if the provided column list is empty.
     */
    Sloppy::CSV_Row toCSV(
        const std::vector<std::string>& colNames   ///< the list of columns that shall be exported
        ) const;

    /** \returns a reference to the underlying database instance
     */
    const SqliteDatabase& dbRef() const
    {
      return db.get();
    }

  protected:
    void genCommaSepString(std::string& target, const std::string& element) const
    {
      if (element.empty()) return;
      if (!target.empty()) target += ",";
      target += element;
    }

    template<typename T, typename... Targs>
    void genCommaSepString(std::string& target, const T& element, Targs... FuncArgs) const
    {
      genCommaSepString(target, element);
      genCommaSepString(target, FuncArgs...);
    }

    template<typename... Targs>
    SqlStatement getSelectStatementWithColumnChecking(Targs... FuncArgs) const
    {
      std::string colNames;
      genCommaSepString(colNames, FuncArgs...);
      std::string sql = "SELECT " + colNames + cachedWhereStatementForRow;

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
    std::reference_wrapper<const SqliteDatabase> db;
    
    /**
     * the name of the associated table
     */
    std::string tabName;
    
    /**
     * the unique index of the data row
     */
    int rowId;

    std::string cachedWhereStatementForRow;
    Sloppy::estring cachedUpdateStatementForRow;
  };

}

