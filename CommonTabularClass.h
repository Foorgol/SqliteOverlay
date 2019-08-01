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

#include <Sloppy/CSV.h>

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
        int colId,   ///< the zero-based column id
        const std::string& colName,   ///< the column name
        const std::string& colType   ///< the column type as used in the "CREATE TABLE" statement
        );

    /** \returns the column's zero-based ID */
    int id () const;

    /** \returns the column's name */
    std::string name () const;

    /** \returns the column's declared type, as provided in the CREATE TABLE statement */
    std::string declType() const;

    /** \returns the column's type (or more precise: the column's type affinity) */
    ColumnAffinity affinity () const;
    
  private:
    int _id;
    std::string _name;
    std::string _declType;
    ColumnAffinity _affinity;
    
  };
  
  /** \brief Convenience definition for a list of column descriptions
   */
  typedef std::vector<ColInfo> ColInfoList;
  
  /** \brief A class that encapsulates common functions for tables and views.
   */
  class CommonTabularClass
  {
  public:
    /** \brief Ctor for a given table/view in a given database
     *
     * \throws std::invalid_argument if the table name is empty
     *
     * \throws NoSuchTable if the name check for the provided table name fails
     *
     * Test case: yes
     *
     */
    CommonTabularClass (
        const SqliteDatabase& _db,   ///< pointer to the database that contains the table / the view
        const std::string& _tabName,   ///< the table's / view's name (case sensitive)
        bool _isView,   ///< `true`: the name identifies a view; `false`: the name identifies a table
        bool forceNameCheck   ///< `true`: check whether a table/view of the provided name exists
        );

    /** \brief (Empty) Dtor
     */
    virtual ~CommonTabularClass () {}

    CommonTabularClass(const CommonTabularClass& other) = default;
    
    CommonTabularClass(CommonTabularClass&& other) = default;

    CommonTabularClass& operator=(const CommonTabularClass& other) = default;

    CommonTabularClass& operator=(CommonTabularClass&& other) = default;

    /** \returns a list of all column definitions in the table
     *
     * Test case: yes
     *
     */
    ColInfoList allColDefs() const;

    /** \returns the column's type affinity
     *
     * Test case: yes
     *
     */
    ColumnAffinity colAffinity(
        const std::string& colName   ///< the name of the column to get the type for
        ) const;

    /** \returns the column's declared type as provided in the CREATE TABLE statement
     *
     * \throws std::invalid_argument if the provided column name is invalid or empty
     *
     * Test case: yes
     *
     */
    std::string colDeclType(
        const std::string& colName   ///< the name of the column to get the type for
        ) const;

    /** \returns the name for a given zero-based column id or an empty string
     * if the column does not exist
     *
     * \throws std::invalid_argument if the provided column name is invalid or empty
     *
     * Test case: yes
     *
     */
    std::string cid2name(
        int cid   ///< the zero-based ID of the column to get the name for
        ) const;

    /** \returns the column's zero-based ID
     *
     * \throws std::invalid_argument if the provided column name is invalid or empty
     *
     * Test case: yes
     *
     */
    int name2cid(
        const std::string& colName   ///< the name of the column to get the ID for
        ) const;

    /** \returns `true` if the table has a column of the given name (case-sensitive)
     *
     * Test case: yes
     *
     */
    bool hasColumn(const std::string& colName) const;

    /** \returns `true` if the table has a column of the given zero-based ID
     *
     * Test case: yes
     *
     */
    bool hasColumn(int cid) const;

    /** \returns the number of rows that match the conditions defined in a WHERE clause
     *
     * \throws std::invalid argument if no column values have been defined in the WHERE clause
     *
     * \throws SqlStatementCreationError if the WHERE clause contained invalid column names
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    int getMatchCountForWhereClause(
        const WhereClause& w   ///< the WHERE clause for which the number of matching rows shall be returned
        ) const;

    /** \returns the number of rows that match the conditions defined in a WHERE clause
     *
     * \throws std::invalid argument if the provided string is empty
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    int getMatchCountForWhereClause(
        const std::string& where   ///< everything after "WHERE ..." in the SELECT statement
        ) const;

    /** \returns the number of rows in which a given column contains a given value
     *
     * \throws std::invalid argument if the column name is empty or invalid
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    template<typename T>
    int getMatchCountForColumnValue(
        const std::string& col,   ///< the column name
        const T& val   ///< the column value
        ) const
    {
      if (col.empty())
      {
        throw std::invalid_argument("getMatchCountForColumnValue(): empty column name");
      }

      std::string sql = sqlColumnCount + col + "=?";
      try
      {
        SqlStatement stmt = db.get().prepStatement(sql);
        stmt.bind(1, val);
        stmt.step();
        return stmt.getInt(0);
      }
      catch (SqlStatementCreationError e)
      {
        throw std::invalid_argument("getMatchCountForColumnValue(): invalid column name");
      }
    }

    /** \returns the number of rows in which a given column contains a given value
     *
     * \throws std::invalid argument if the column name is empty
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    int getMatchCountForColumnValueNull(
        const std::string& col  ///< the column name
        ) const;

    /** \returns the number of rows in the table
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * Test case: yes
     *
     */
    int length() const;

    /** \returns a reference to the underlying database instance
     */
    const SqliteDatabase& dbRef() const
    {
      return db.get();
    }

    /** \brief Exports the contents of the table or view as CSV data.
     *
     * \note The implementation of this function is not optimized for large quantities of data.
     *
     * \warning If the SQL statement does not yield any data rows, we'll return a completely empty
     * table WITHOUT headers, even if the inclusion of headers was requested by the caller!
     *
     * \throws InvalidColumnException if any of the result columns contains BLOB data because
     * that can't be properly exported to CSV.
     *
     * \throws InvalidColumnException if any of the result columns has an invalid name (not unique,
     * contains commas or contains quotation marks) and headers names shall be included in the CSV table.
     *
     * \returns a (potentially empty) CSV_Table instance with the contents of the table / view
     */
    Sloppy::CSV_Table toCSV(
        bool includeHeaders   ///< include column headers in the first CSV table row yes/no
        ) const;

    /** \brief Exports the contents of the table or view as CSV data.
     *
     * \note The implementation of this function is not optimized for large quantities of data.
     *
     * \warning If the SQL statement does not yield any data rows, we'll return a completely empty
     * table WITHOUT headers, even if the inclusion of headers was requested by the caller!
     *
     * \throws InvalidColumnException if any of the result columns contains BLOB data because
     * that can't be properly exported to CSV.
     *
     * \throws InvalidColumnException if any of the result columns has an invalid name (not unique,
     * contains commas or contains quotation marks) and headers names shall be included in the CSV table.
     *
     * \throws SqlStatementCreationError if the construction of the underlying SQL statement failed, for
     * instance due to invalid column names provided by the caller.
     *
     * \returns a (potentially empty) CSV_Table instance with the contents of the table / view
     */
    Sloppy::CSV_Table toCSV(
        std::vector<std::string> colNames,   ///< list of columns that shall be exported (empty = all columns)
        WhereClause w,   ///< where clause that limits the rows that shall be exported (empty = all rows)
        bool includeHeaders   ///< include column headers in the first CSV table row yes/no
        ) const;

  protected:
    /**
     * the handle to the (parent) database
     */
    std::reference_wrapper<const SqliteDatabase> db;
    
    /**
     * the name of the associated table or view
     */
    std::string tabName;
    
    /**
     * a tag whether we are a view or a tab
     */
    bool isView;

    std::string sqlColumnCount;

  private:

  };

}
#endif	/* COMMONTABULARCLASS_H */

