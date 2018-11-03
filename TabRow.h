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
     * \throws std::invalid_argument if the database pointer or the table name
     * is empty or if the provided row ID was invalid
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    TabRow (
        const SqliteDatabase* db,   ///< the database that contains the table
        const string& _tabName,  ///< the table name
        int _rowId,   ///< the ID of the row, has to be in a column named "id"
        bool skipCheck = false   ///< if `true` the validity of the ID (read: the actual existence of the row) will *not* be checked
        );

    /**
     * \brief Constructor for the first row in the table that matches a custom WHERE clause.
     *
     * \throws std::invalid_argument if the database pointer or the table name
     * is empty or if the provided WHERE didn't produce any results
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    TabRow (
        const SqliteDatabase* db,   ///< the database that contains the table
        const string& _tabName,  ///< the table name
        const WhereClause& where   ///< the WHERE clause that identifies the row
        );

    /** \brief Empty default dtor */
    virtual ~TabRow () {}

    /** \returns the row's ID */
    int getId() const;

    /** \brief Updates one or more columns of the row to new values
     *
     * \throws BusyException if the database wasn't available for
     * updating the columns
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    void update(
        const ColumnValueClause& cvc
        ) const;

    /** \brief Updates a given column of the row to a new value; works for
     * the standard types int, double, float, bool, string
     *
     * \throws BusyException if the database wasn't available for
     * updating the column
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    template<typename T>
    void update(
        const string& colName,   ///< the name of the column to modify
        const T& newVal   ///< the new value for that column
        ) const
    {
      Sloppy::estring sql{cachedUpdateStatementForRow};
      sql.arg(colName);
      SqlStatement stmt = db->prepStatement(sql);
      stmt.bind(1, newVal);

      db->execNonQuery(stmt);
    }

    /** \brief Updates a given column of the row to a new value
     *
     * \throws BusyException if the database wasn't available for
     * updating the column
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    void update(
        const string& colName,   ///< the name of the column to modify
        const LocalTimestamp& newVal   ///< the new value for that column
        ) const;

    /** \brief Updates a given column of the row to a new value
     *
     * \throws BusyException if the database wasn't available for
     * updating the column
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    void update(
        const string& colName,   ///< the name of the column to modify
        const UTCTimestamp& newVal) const;

    /** \brief Updates a given column of the row to a new value
     *
     * \throws BusyException if the database wasn't available for
     * updating the column
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    void update(
        const string& colName,   ///< the name of the column to modify
        const greg::date& newVal   ///< the new value for that column
        ) const;

    /** \brief Updates a given column of the row to NULL
     *
     * \throws BusyException if the database wasn't available for
     * updating the column
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
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
     */
    string operator[](
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
     */
    UTCTimestamp getUTCTime(const string& colName) const;

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
     */
    greg::date getDate(const string& colName) const;

    /** \returns the contents of a given column as an integer or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     */
    optional<int> getInt2(const string& colName) const;

    /** \returns the contents of a given column as a double or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     */
    optional<double> getDouble2(const string& colName) const;

    /** \returns the contents of a given column as a string or NULL if the column was empty
     *
     * \throws std::invalid argument if the column name was empty
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \throws NoDataException if the query didn't return any data (e.g., invalid column name
     * or row has been deleted in the meantime)
     */
    optional<string> getString2(const string& colName) const;

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
     */
    optional<LocalTimestamp> getLocalTime2(const string& colName, boost::local_time::time_zone_ptr tzp) const;

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
    optional<UTCTimestamp> getUTCTime2(const string& colName) const;

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
     */
    optional<greg::date> getDate2(const string& colName) const;

    /** \brief Overload operator for "is equal", compares table name,
     * row ID and database pointer.
     *
     * Thus, it returns only `true` if both TabRows belong
     * to the same SqliteDatabase pointer (read: to the same
     * database connection)!
     */
    inline bool operator== (const TabRow& other) const
    {
      return ((other.tabName == tabName) && (other.rowId == rowId) && (other.db == db));  // enforce the same db handle!
    }

    /** \brief Overload operator for "is not equal", compares table name,
     * row ID and database pointer.
     *
     * Thus, it returns only `false` if both TabRows belong
     * to the same SqliteDatabase pointer (read: to the same
     * database connection)!
     */
    inline bool operator!= (const TabRow& other) const
    {
      return (!(this->operator == (other)));
    }

    /** \returns the pointer to the database we're using */
    const SqliteDatabase* getDb() const;

    /** \brief Erases the row from the table.
     *
     * \warning This TabRow instance may not be used anymore
     * after calling `erase()`!
     *
     * \throws BusyException if the database wasn't available for
     * deleting the column
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    void erase() const;
    
  protected:
    /**
     * the handle to the (parent) database
     */
    const SqliteDatabase* const db;
    
    /**
     * the name of the associated table
     */
    const string tabName;
    
    /**
     * the unique index of the data row
     */
    int rowId;

    string cachedWhereStatementForRow;
    Sloppy::estring cachedUpdateStatementForRow;
    
  private:

  };
}

#endif	/* TABROW_H */

