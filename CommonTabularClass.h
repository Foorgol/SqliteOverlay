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

#ifndef SQLITE_OVERLAY_COMMONTABULARCLASS_H
#define	SQLITE_OVERLAY_COMMONTABULARCLASS_H

#include <vector>

#include "SqliteDatabase.h"
#include "ClausesAndQueries.h"
#include "SqlStatement.h"
#include "Defs.h"

namespace SqliteOverlay
{

  /** \brief A small struct with schema information about a column
   */
  class ColInfo
  {
  public:

    /** \brief Ctor for a set of column parameters as returned by
     * "PRAGMA table_info()".
     *
     * The string containing the type name is converted into one
     * of SQLites basic types as described in the chapter "Determination
     * of Column Affinity" [here](https://www.sqlite.org/datatype3.html).
     */
    ColInfo (
        int _cid,   ///< the zero-based column id
        const string& _name,   ///< the column name
        const string& _type   ///< the column type as used in the "CREATE TABLE" statement
        );

    /** \returns the column's zero-based ID */
    int getId () const;

    /** \returns the column's name */
    string getColName () const;

    /** \returns the column's type (or more precise: the column's type affinity) */
    ColumnDataType getColType () const;
    
  private:
    const int cid;
    const string name;
    const ColumnDataType type;
    
  };
  
  /** \brief Convenience definition for a list of column descriptions
   */
  typedef vector<ColInfo> ColInfoList;
  
  /** \brief A class that encapsulates common functions for tables and views.
   */
  class CommonTabularClass
  {
  public:
    /** \brief Ctor for a given table/view in a given database
     *
     * \throws std::invalid_argument if the database pointer or the table name is empty
     *
     * \throws NoSuchTable if the name check for the provided table name fails
     */
    CommonTabularClass (
        SqliteDatabase* db,   ///< pointer to the database that contains the table / the view
        const string& tabName,   ///< the table's / view's name (case sensitive)
        bool _isView,   ///< `true`: the name identifies a view; `false`: the name identifies a table
        bool forceNameCheck   ///< `true`: check whether a table/view of the provided name exists
        );

    /** \brief (Empty) Dtor
     */
    virtual ~CommonTabularClass () {}
    
    /** \returns a list of all column definitions in the table
     */
    ColInfoList allColDefs() const;

    /** \returns the column's type (or more precise: the column's type affinity)
     * with the special value NULL if the column does not exist */
    ColumnDataType getColType(
        const string& colName   ///< the name of the column to get the type for
        ) const;

    /** \returns the name for a given zero-based column id or an empty string
     * if the column does not exist */
    string cid2name(
        int cid   ///< the zero-based ID of the column to get the name for
        ) const;

    /** \returns the column's zero-based ID
     * with the special value -1 if the column does not exist */
    int name2cid(
        const string& colName   ///< the name of the column to get the ID for
        ) const;

    /** \returns `true` if the table has a column of the given name (case-sensitive) */
    bool hasColumn(const string& colName) const;

    /** \returns `true` if the table has a column of the given zero-based ID */
    bool hasColumn(int cid) const;

    /** \returns the number of rows that match the conditions defined in a WHERE clause
     *
     * \throws std::invalid argument if no column values have been defined in the WHERE clause
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    int getMatchCountForWhereClause(
        const WhereClause& w   ///< the WHERE clause for which the number of matching rows shall be returned
        ) const;

    /** \returns the number of rows that match the conditions defined in a WHERE clause
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    int getMatchCountForWhereClause(
        const string& where   ///< everything after "WHERE ..." in the SELECT statement
        ) const;

    /** \returns the number of rows in which a given column contains a given value
     *
     * \throws std::invalid argument if the column name is empty
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    int getMatchCountForColumnValue(
        const string& col,   ///< the column name
        const string& val   ///< the column value
        ) const;

    /** \returns the number of rows in which a given column contains a given value
     *
     * \throws std::invalid argument if the column name is empty
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    int getMatchCountForColumnValue(
        const string& col,   ///< the column name
        int val   ///< the column value
        ) const;

    /** \returns the number of rows in which a given column contains a given value
     *
     * \throws std::invalid argument if the column name is empty
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    int getMatchCountForColumnValue(
        const string& col,   ///< the column name
        double val   ///< the column value
        ) const;

    /** \returns the number of rows in which a given column contains a given value
     *
     * \throws std::invalid argument if the column name is empty
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    int getMatchCountForColumnValue(
        const string& col,   ///< the column name
        const CommonTimestamp* pTimestamp   ///< the column value
        ) const;

    /** \returns the number of rows in which a given column contains a given value
     *
     * \throws std::invalid argument if the column name is empty
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    int getMatchCountForColumnValueNull(
        const string& col  ///< the column name
        ) const;

    /** \returns the number of rows in the table
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     */
    int length() const;

  protected:
    /**
     * the handle to the (parent) database
     */
    const SqliteDatabase* const db;
    
    /**
     * the name of the associated table or view
     */
    const string tabName;
    
    /**
     * a tag whether we are a view or a tab
     */
    const bool isView;

  private:

  };

}
#endif	/* COMMONTABULARCLASS_H */

