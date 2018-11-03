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

#ifndef SQLITE_OVERLAY_DBTAB_H
#define	SQLITE_OVERLAY_DBTAB_H

#include <vector>
#include <optional>

#include "CommonTabularClass.h"
#include "ClausesAndQueries.h"
#include "SqliteExceptions.h"
#include "TabRow.h"

namespace SqliteOverlay
{

  /** \brief A class that represents a table in a database
   */
  class DbTab : public CommonTabularClass
  {
    friend class SqliteDatabase;

  public:
    /** \brief Standard ctor that creates a new DbTab object for a given database
     * and table name.
     *
     * \throws std::invalid_argument if the database pointer or the table name is empty
     *
     * \throws NoSuchTable if the name check for the provided table name fails
     */
    DbTab (
        SqliteDatabase* db,   ///< pointer to the database that contains the table
        const string& tabName,   ///< the table's name (case sensitive)
        bool forceNameCheck   ///< `true`: check whether a table/view of the provided name exists
        );

    /** \brief Empty dtor */
    virtual ~DbTab ();

    /** \brief Appends a new row with given column values to the table
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the ID of the newly created row
     */
    int insertRow(
        const ColumnValueClause& ic   ///< the (potentially empty) column values for the new row
        ) const;

    /** \brief Appends a new row with default column values to the table
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the ID of the newly created row
     */
    int insertRow() const;

    /** \brief Provides access to a single row in the table, the row being identified
     * by its ID
     *
     * \warning For performance reasons, *this operator does not check the validity of the provided row ID*!
     * If in doubt whether the ID exists, call `hasRowId()` or `get2()`.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a TabRow instance representing the requested row
     */
    TabRow operator[](const int id) const;

    /**
     * \brief Provides access to the first row in the table that matches a custom WHERE clause.
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a TabRow instance representing the requested row
     */
    TabRow operator[](const WhereClause& w ) const;

    /** \brief Provides access to a single row in the table, the row being identified
     * by its ID
     *
     * This function can deal with invalid row IDs by returning an empty `optional<>'
     * if the provided ID was invalid.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an 'optional<TabRow>' that contains the requested row or that is empty if
     * the ID was invalid
     */
    optional<TabRow> get2(int id) const;

    /** \brief Provides access to the first row in the table that matches a custom WHERE clause.
     *
     * This function can deal with invalid row IDs by returning an empty `optional<>'
     * if the provided WHERE clause didn't yield any matches.
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns an 'optional<TabRow>' that contains the requested row or that is empty if
     * the ID was invalid
     */
    optional<TabRow> get2(const WhereClause& w) const;

    /** \returns the first row that contains a given value in a given column
     *
     * \throws NoDataException if no such row exists
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    template<typename T>
    TabRow getSingleRowByColumnValue(const string& col, const T& val) const
    {
      SqlStatement stmt = getSelectStmtForColumn(col);
      stmt.bind(1, val);
      int id = db->execScalarQueryInt(stmt);

      return TabRow(db, tabName, id, true);
    }

    /** \returns an 'optional<TabRow>' that contains the first row that contains a given
     * value in a given column or that is empty if no such row exists
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    template<typename T>
    optional<TabRow> getSingleRowByColumnValue2(const string& col, const T& val) const
    {
      try
      {
        // try a regular lookup and let the compiler
        // pick the best function (int, double, string, ...)
        return getSingleRowByColumnValue(col, val);
      }
      catch (NoDataException& e)
      {
        // return an empty optional if no such column exists
        return optional<TabRow>{};
      }
      catch (...)
      {
        // rethrow any other exceptio
        throw;
      }

      return optional<TabRow>{};   // we should never reach this point...
    }

    /** \returns the first row that contains NULL in a given column
     *
     * \throws NoDataException if no such row exists
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    TabRow getSingleRowByColumnValueNull(const string& col) const;

    /** \returns an 'optional<TabRow>' that contains the first row that contains NULL
     * in a given column or that is empty if no such row exists
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     */
    optional<TabRow> getSingleRowByColumnValueNull2(const string& col) const;

    TabRow getSingleRowByWhereClause(const WhereClause& w) const;
    TabRow getSingleRowByWhereClause(const string& w) const;
    vector<TabRow> getRowsByWhereClause(const WhereClause& w) const;
    vector<TabRow> getRowsByWhereClause(const string& w, bool isWhereClauseOnly=true) const;
    vector<TabRow> getRowsByColumnValue(const string& col, int val) const;
    vector<TabRow> getRowsByColumnValue(const string& col, double val) const;
    vector<TabRow> getRowsByColumnValue(const string& col, const string& val) const;
    vector<TabRow> getRowsByColumnValueNull(const string& col) const;
    vector<TabRow> getAllRows() const;
    int deleteRowsByWhereClause(const WhereClause& where, int* errCodeOut=nullptr) const;
    int deleteRowsByColumnValue(const string& col, const int val, int* errCodeOut=nullptr) const;
    int deleteRowsByColumnValue(const string& col, const double val, int* errCodeOut=nullptr) const;
    int deleteRowsByColumnValue(const string& col, const string& val, int* errCodeOut=nullptr) const;
    int clear(int* errCodeOut=nullptr) const;

    bool addColumn(const string& colName, const string& typeStr, bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                   bool hasDefault=false, const string& defaultValue="", bool returnErrorIfExists=false);
    bool addColumn_Varchar(const string& colName, int len, bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                   bool hasDefault=false, const string& defaultValue="", bool returnErrorIfExists=false);
    bool addColumn_int(const string& colName, bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                   bool hasDefault=false, int defaultValue=0, bool returnErrorIfExists=false);
    bool addColumn_text(const string& colName, bool notNull=false, CONFLICT_CLAUSE notNullConflictClause=CONFLICT_CLAUSE::__NOT_SET,
                   bool hasDefault=false, const string& defaultValue="", bool returnErrorIfExists=false);
    bool addColumn_foreignKey(const string& colName, const string& referedTabName,
                              CONSISTENCY_ACTION onDelete=CONSISTENCY_ACTION::__NOT_SET,
                              CONSISTENCY_ACTION onUpdate=CONSISTENCY_ACTION::__NOT_SET,
                              bool returnErrorIfExists=false);

    bool hasRowId(int id) const;

  protected:
    SqlStatement getSelectStmtForColumn(const string& colName) const;

  private:
    bool addColumn(const string& colName, const string& typeStr, const string& constraints);
    string cachedSelectSql;
  };

}

#endif	/* DBTAB_H */

