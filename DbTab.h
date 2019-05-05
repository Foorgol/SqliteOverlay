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

  // forward
  template<typename T>
  class SingleColumnIterator;

  // forward
  class TabRowIterator;

  /** \brief A class that represents a table in a database
   */
  class DbTab : public CommonTabularClass
  {

  public:
    /** \brief Standard ctor that creates a new DbTab object for a given database
     * and table name.
     *
     * \throws std::invalid_argument if the table name is empty
     *
     * \throws NoSuchTable if the name check for the provided table name fails
     *
     * Test case: yes
     *
     */
    DbTab (
        const SqliteDatabase& db,   ///< the database that contains the table
        const string& tabName,   ///< the table's name (case sensitive)
        bool forceNameCheck   ///< `true`: check whether a table of the provided name exists
        );

    /** \brief Empty dtor */
    virtual ~DbTab () {}

    DbTab(const DbTab& other) = default;

    DbTab(DbTab&& other) = default;

    DbTab& operator=(const DbTab& other) = default;

    DbTab& operator=(DbTab&& other) = default;

    /** \returns the name of the table we're linked to
     */
    const string& name() const
    {
      return tabName;
    }

    /** \brief Appends a new row with given column values to the table
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the ID of the newly created row
     *
     * Test case: yes
     *
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
     *
     * Test case: yes
     *
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
     *
     * Test case: not yet
     *
     */
    TabRow operator[](
        const int id   ///< the rowid of the requested row
        ) const;

    /**
     * \brief Provides access to the first row in the table that matches a custom WHERE clause.
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws std::invalid_argument if no such row exists or if the WHERE clause is empty
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a TabRow instance representing the requested row
     *
     * Test case: not yet
     *
     */
    TabRow operator[](
        const WhereClause& w   ///< the WHERE clause that identifies the row
        ) const;

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
     *
     * Test case: yes
     *
     */
    optional<TabRow> get2(
        const int id   ///< the rowid of the requested row
        ) const;

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
     *
     * Test case: yes
     *
     */
    optional<TabRow> get2(
        const WhereClause& w   ///< the WHERE clause that identifies the row
        ) const;

    /** \returns the first row that contains a given value in a given column
     *
     * \throws NoDataException if no such row exists or if the column name was invalid
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    template<typename T>
    TabRow getSingleRowByColumnValue(
        const string& col,   ///< the column name
        const T& val   ///< the value to search for in the column
        ) const
    {
      SqlStatement stmt;
      try {
        stmt = db.get().prepStatement(cachedSelectSql + col + "=?");
      } catch (SqlStatementCreationError) {
        throw NoDataException();
      }

      stmt.bind(1, val);

      // don't use execScalarQueryInt here so that we can save some checks / calls
      stmt.step();
      if (!stmt.hasData())
      {
        throw NoDataException{"call to TabRow::getSingleRowByColumnValue() but the query didn't return any data"};
      }
      int id = stmt.getInt(0);

      return TabRow(db, tabName, id, true);
    }

    /** \returns an 'optional<TabRow>' that contains the first row that contains a given
     * value in a given column or that is empty if no such row exists
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    template<typename T>
    optional<TabRow> getSingleRowByColumnValue2(
        const string& col,   ///< the column name
        const T& val   ///< the value to search for in the column
        ) const
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
     *
     * Test case: yes
     *
     */
    TabRow getSingleRowByColumnValueNull(
        const string& col   ///< the name of the column that should be searched for a NULL value
        ) const;

    /** \returns an 'optional<TabRow>' that contains the first row that contains NULL
     * in a given column or that is empty if no such row exists
     *
     * \throws BusyException if the statement couldn't be executed because the DB was busy
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    optional<TabRow> getSingleRowByColumnValueNull2(
        const string& col   ///< the name of the column that should be searched for a NULL value
        ) const;

    /**
     * \brief Provides access to the first row in the table that matches a custom WHERE clause.
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws NoDataException if no such row exists
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a TabRow instance representing the requested row
     *
     * Test case: not necessary, identical with `operator[](const WhereClause&)`
     * and with the `TabRow` ctor
     *
     */
    TabRow getSingleRowByWhereClause(
        const WhereClause& w   ///< the WHERE clause that identifies the row
        ) const;

    /**
     * \brief Provides access to the first row in the table that matches a custom WHERE clause.
     *
     * \throws BusyException if the database wasn't available for checking the
     * validity of the rowId
     *
     * \throws NoDataException if no such row exists
     *
     * \throws SqliteStatementCreationError if the provided WHERE clause contained invalid SQL
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a TabRow instance representing the requested row
     *
     * Test case: yes
     *
     */
    TabRow getSingleRowByWhereClause(
        const string& w   /// a string that should be appended after the keyword "WHERE" in the SQL statement
        ) const;

    /** \brief Retrieves a (potentially empty) list of all `TabRow` instances that match a given WHERE clause
     *
     * If any column name in the WHERE clause is invalid, no exception is thrown. Instead,
     * we simply return an empty result set.
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws std::invalid_argument if the provided WHERE clause is empty
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a `vector` of all `TabRow`s that match the WHERE clause
     *
     * Test case: yes
     *
     */
    vector<TabRow> getRowsByWhereClause(
        const WhereClause& w   ///< the WHERE clause for the query
        ) const;

    /** \brief Retrieves a (potentially empty) list of all `TabRow` instances that match a given WHERE clause
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a `vector` of all `TabRow`s that match the WHERE clause
     *
     * Test case: yes
     *
     */
    vector<TabRow> getRowsByWhereClause(
        const string& w   /// a string that should be appended after the keyword "WHERE" in the SQL statement
        ) const;

    /** \brief Retrieves a (potentially empty) list of all `TabRow` instances that
     * match a NULL value in a given clause.
     *
     * Invalid, non-empty column names do not throw an exception; instead,
     * they return an empty result set.
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws std::invalid_argument if the provided column name is empty
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a `vector` of all matching `TabRow`s
     *
     * Test case: yes
     *
     */
    vector<TabRow> getRowsByColumnValueNull(
        const string& col   ///< the name of the column that should be searched for NULL values
        ) const;

    /** \brief Retrieves a (potentially empty) list of all `TabRow` instances that
     * contain a given value in a given column
     *
     * Invalid, non-existing column names do not throw an exception; instead,
     * they return an empty result set.
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws std::invalid_argument if the provided column name is empty
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a `vector` of all `TabRow`s that match the given value in the given volumn
     *
     * Test case: yes
     *
     */
    template<typename T>
    vector<TabRow> getRowsByColumnValue(
        const string& col,   ///< the name of the column
        const T& val   ///< the column value that should be matched
        ) const
    {
      if (col.empty())
      {
        throw std::invalid_argument("getRowsByColumnValue(): called with empty column name!");
      }

      SqlStatement stmt;
      try
      {
        stmt = db.get().prepStatement(cachedSelectSql + col + "=?");
      } catch (SqlStatementCreationError) {
        return vector<TabRow>{};
      }

      stmt.bind(1, val);
      return statementResultsToVector(stmt);
    }

    /** \returns a `vector` with all rows in the table
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    vector<TabRow> getAllRows() const;

    /** \brief Deletes all rows that match a given WHERE clause
     *
     * \throws SqliteStatementCreationError if the provided WHERE clause contained invalid column names
     *
     * \throws std::invalid_argument if the provided WHERE clause was empty
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the number of rows that have been deleted
     *
     * Test case: yes
     *
     */
    int deleteRowsByWhereClause(
        const WhereClause& where
        ) const;

    /** \brief Deletes all rows that match a given value in a given column
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws SqliteStatementCreationError if the provided column name was invalid
     *
     * \throws std::invalid_argument if the provided column name is empty
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the number of rows that have been deleted
     *
     * Test case: yes
     *
     */
    template<typename T>
    int deleteRowsByColumnValue(
        const string& col,   ///< the column name
        const T& val    ///< the column value that identifies the matching rows
        ) const
    {
      if (col.empty())
      {
        throw std::invalid_argument("deleteRowsByColumnValue(): called with empty row name");
      }

      SqlStatement stmt = db.get().prepStatement("DELETE FROM " + tabName + " WHERE " + col + "=?");
      stmt.bind(1, val);
      stmt.step();

      return db.get().getRowsAffected();
    }

    /** \brief Deletes ALL rows from the table
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns the number of rows that have been deleted
     *
     * Test case: yes
     *
     */
    int clear() const;

    /** \brief Adds a new column without default value to the table
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws std::invalid_argument if the column name is empty
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns 'true' if the column has been added to the table, `false` if the column already exists
     * or if the column was requested to be NOT NULL (because that requires a default value)
     *
     * Test case: not yet
     *
     */
    bool addColumn(
        const string& colName,   ///< the name of the new column
        ColumnDataType type   ///< the basic column type
        ) const
    {
      // check if the column exists
      if (hasColumn(colName))
      {
        return false;
      }

      string constraints = buildColumnConstraint(ConflictClause::NotUsed, ConflictClause::NotUsed);

      // forward any exceptions to the caller
      //
      // use an empty, dummy default value that won't be processed anyway
      addColumn_exec(colName, type, constraints);

      return true;
    }

    /** \brief Adds a new column with a default value to the table
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws std::invalid_argument if the column name is empty
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns 'true' if the column has been added to the table, `false` if the column already exists
     *
     * Test case: not yet
     *
     */
    template<typename T>
    bool addColumn(
        const string& colName,   ///< the name of the new column
        ColumnDataType type,   ///< the basic column type
        ConflictClause notNullConflictClause,   ///< NotUsed = column may contain NULL values
        const T& defaultValue ///< the default value for the new column; mandatory if "NOT NULL" is set
        ) const
    {
      // check if the column exists
      if (hasColumn(colName))
      {
        return false;
      }

      string constraints = buildColumnConstraint(ConflictClause::NotUsed, notNullConflictClause, defaultValue);

      // forward any exceptions to the caller
      addColumn_exec(colName, type, constraints);

      return true;
    }

    /** \brief Adds a new column with a FOREIGN-KEY constraint to the table
     *
     * \note Adding foreign key columns with non-NULL default values violates the SQL logic
     * and throws an exception if foreign keys are enforced. And enforcing foreign
     * keys is the default for this lib.
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns 'true' if the column has been added to the table
     *
     * Test case: partially
     *
     */
    template<typename T>
    bool addColumn_foreignKey(
        const string& colName,   ///< the name of the new column
        const string& referedTabName,   ///< the name of the referred table
        const string& referedColName,   ///< the name of the refered to column (we use 'id' if empty)
        ConsistencyAction onDelete,   ///< what to do if the refered row is deleted
        ConsistencyAction onUpdate,   ///< what to do if the refered data is updated
        ConflictClause notNullConflictClause,   ///< NotUsed = column may contain NULL values
        const optional<T>& defaultValue ///< the (optional) default value for the new column; mandatory if "NOT NULL" is set
        ) const
    {
      // check if the column exists
      if (hasColumn(colName))
      {
        return false;
      }

      // use 'id' as the default for the target column
      //
      // note: you can't use "rowid" for this purpose
      // if the table has been created with the TableCreator, a referable
      // column named "id" which is an alias for "rowid" has automatically
      // been created.
      const string refCol{referedColName.empty() ? "id" : referedColName};

      // determine the type of the referenced column, unless it's `id`
      ColumnDataType colType{ColumnDataType::Integer};
      if (refCol != "id")
      {
        DbTab otherTab{db, referedTabName, true};
        ColumnAffinity a = otherTab.colAffinity(refCol);
        switch (a)
        {
        case ColumnAffinity::Real:
          colType = ColumnDataType::Float;
          break;

        case ColumnAffinity::Blob:
          colType = ColumnDataType::Blob;
          break;

        case ColumnAffinity::Text:
          colType = ColumnDataType::Text;
          break;
        }
      }

      // build a foreign key constraint
      string constraints{};
      if (defaultValue.has_value())
      {
        constraints = buildColumnConstraint(ConflictClause::NotUsed, notNullConflictClause, defaultValue.value());
      } else {
        constraints = buildColumnConstraint(ConflictClause::NotUsed, notNullConflictClause);
      }
      constraints += " " + buildForeignKeyClause(referedTabName, onDelete, onUpdate, refCol);

      addColumn_exec(colName, colType, constraints);

      return true;
    }

    /** \brief Adds a new column with a FOREIGN-KEY constraint to the table
     *
     * \note Adding foreign key columns with non-NULL default values violates the SQL logic
     * and throws an exception if foreign keys are enforced. And enforcing foreign
     * keys is the default for this lib.
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns 'true' if the column has been added to the table
     *
     * Test case: partially
     *
     */
    template<typename T>
    bool addColumn_foreignKey(
        const string& colName,   ///< the name of the new column
        const string& referedTabName,   ///< the name of the referred table
        const string& referedColName,   ///< the name of the refered to column (we use 'id' if empty)
        ConsistencyAction onDelete,   ///< what to do if the refered row is deleted
        ConsistencyAction onUpdate,   ///< what to do if the refered data is updated
        ConflictClause notNullConflictClause,   ///< NotUsed = column may contain NULL values
        const T& defaultValue ///< the default value for the new column; mandatory if "NOT NULL" is set
        ) const
    {
      return addColumn_foreignKey(
            colName, referedTabName, referedColName,
            onDelete, onUpdate,
            notNullConflictClause, optional<T>{defaultValue}
            );
    }

    bool addColumn_foreignKey(
        const string& colName,   ///< the name of the new column
        const string& referedTabName,   ///< the name of the referred table
        const string& referedColName,   ///< the name of the refered to column (we use 'id' if empty)
        ConsistencyAction onDelete,   ///< what to do if the refered row is deleted
        ConsistencyAction onUpdate,   ///< what to do if the refered data is updated
        ConflictClause notNullConflictClause,   ///< NotUsed = column may contain NULL values
        const char* defaultValue ///< the default value for the new column; mandatory if "NOT NULL" is set
        ) const
    {
      return addColumn_foreignKey(
            colName, referedTabName, referedColName,
            onDelete, onUpdate,
            notNullConflictClause, optional<string>{string{defaultValue}}
            );
    }

    /** \brief Adds a new column with a FOREIGN-KEY constraint to the table
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns 'true' if the column has been added to the table
     *
     * Test case: partially
     *
     */
    bool addColumn_foreignKey(
        const string& colName,   ///< the name of the new column
        const string& referedTabName,   ///< the name of the referred table
        const string& referedColName,   ///< the name of the refered to column (we use 'id' if empty)
        ConsistencyAction onDelete,   ///< what to do if the refered row is deleted
        ConsistencyAction onUpdate   ///< what to do if the refered data is updated
        ) const
    {
      return addColumn_foreignKey(
            colName, referedTabName, referedColName,
            onDelete, onUpdate,
            ConflictClause::NotUsed, optional<int>{}  // the optional<int> is just an empty dummy value
            );
    }

    /** \returns 'true' if a row with the provided `rowid` exists in the table
     *
     * \throws BusyException if the database wasn't available
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    bool hasRowId(
        int id   ///< the `rowid` in question
        ) const;

    /** \brief Checks whether all rows of a column satisfy a given constraint, e.g. "alphanumeric".
     *
     * \note Consider calling this function from within a transaction in order to avoid that the
     * constraint is violated by another database connection after the check.
     *
     * \throws std::invalid argument if the column name was empty or invalid
     *
     * \throws BusyException if the database wasn't available for
     * reading the column
     *
     * \returns a list of rowid that violate the constraint; if empty, all rows comply with the constraint
     */
    vector<int> checkConstraint(
          const string& colName,   ///< the name of the column to check
          Sloppy::ValueConstraint c,  ///< the constraint to check the column content against
          int firstRowId = -1
        ) const;

    /** \returns a SingleColumnIterator that iterates over all values / rows of a column,
     * if not restricted by an optional row range
     */
    template<typename T>
    SingleColumnIterator<T> singleColumnIterator(
          const string& colName,   ///< the column that shall be iterated over
          int minRowId = -1,   ///< return only rows with a rowid greater or equal to this values
          int maxRowId = -1   ///< return only rows with a rowid less or equal to this values
          )
    {
      return SingleColumnIterator<T>{db.get(), tabName, colName, minRowId, maxRowId};
    }

    /** \returns a SingleColumnIterator that iterates over all values / rows of a column
     * that match a WHERE clause
     */
    template<typename T>
    SingleColumnIterator<T> singleColumnIterator(
          const string& colName,   ///< the column that shall be iterated over
          const WhereClause& w   ///< a WHERE clause that narrows down the number of returned rows; the WHERE can include conditions on other columns than just 'colName', of course
          )
    {
      return SingleColumnIterator<T>{db.get(), tabName, colName, w};
    }

    /** \returns a TabRowIterator that iterates over all rows
     * if not restricted by an optional row range
     */
    TabRowIterator tabRowIterator(
          int minRowId = -1,   ///< return only rows with a rowid greater or equal to this values
          int maxRowId = -1   ///< return only rows with a rowid less or equal to this values
          );


    /** \returns a TabRowIterator that iterates over all rows
     * that match a WHERE clause
     */
    TabRowIterator tabRowIterator(
          const WhereClause& w   ///< a WHERE clause that narrows down the number of returned rows; the WHERE can include conditions on other columns than just 'colName', of course
          );

  protected:
    void addColumn_exec(
        const string& colName,
        ColumnDataType colType,
        const string& constraints
        ) const;

    vector<TabRow> statementResultsToVector(SqlStatement& stmt) const;

  private:
    string cachedSelectSql;
  };

  /** \brief An iterator-like class that allows for easy iteration over
   * all values / all rows in a dedicated column.
   *
   * This is essentially a wrapper around an underlying SqlStatement that
   * selects the given range of rows and the specified column. Rows are always
   * iterated in ascending rowid order.
   *
   * Typical usage could look like this:
   * \code
   * for (auto it = tab.singleColumnIterator<int>("MyCol"); it.hasData(); ++it)
   * {
   *   doSomething();
   * }
   * \endcode
   *
   * \note That this class' interface does not comply with the interface
   * of a "usual" iterator. Such a "usual" iterator would, among other things,
   * require to be `copy-constructible` and `copy-assignable` which is
   * difficult due to our underlying SqlStatement.
   *
   * \note As long as the iterator has not finished, we have a running statement
   * open on the database and on our connection. Keep this in mind when mixing this
   * iterator with transactions and/or concurrent database connections.
   *
   * I'm not sure what happens if you add or remove rows while this iterator and
   * its underlying SqlStatement are active.
   *
   */
  template<typename T>
  class SingleColumnIterator
  {
  public:
    /** \brief Ctor with most basic parameters (all strings and ints, except
     * for the database.
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     */
    SingleColumnIterator(
        const SqliteDatabase& db,   ///< the database that contains the table
        const string& tabName,   ///< the table that contains the column
        const string& colName,   ///< the column that shall be iterated over
        int minRowId = -1,   ///< return only rows with a rowid greater or equal to this values
        int maxRowId = -1   ///< return only rows with a rowid less or equal to this values
        )
    {
      WhereClause w;
      if (minRowId > 0)
      {
        w.addCol("rowid", ">=", minRowId);
      }
      if (maxRowId > 0)
      {
        w.addCol("rowid", "<=", maxRowId);
      }
      init(db, tabName, colName, w);
    }

    /** \brief Ctor with a `DbTab` for identifying the database and the table
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     */
    SingleColumnIterator(
        const DbTab& tab,   ///< the table that contains the column
        const string& colName,   ///< the column that shall be iterated over
        int minRowId = -1,   ///< return only rows with a rowid greater or equal to this values
        int maxRowId = -1   ///< return only rows with a rowid less or equal to this values
        )
      :SingleColumnIterator(tab.dbRef(), tab.name(), colName, minRowId, maxRowId) {}

    /** \brief Ctor with a `DbTab` for identifying the database and the table
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     *
     * \note Rows are always iterated in ascending rowid order regardless of
     * any order setting in the WhereClause.
     */
    SingleColumnIterator(
        const DbTab& tab,   ///< the table that contains the column
        const string& colName,   ///< the column that shall be iterated over
        const WhereClause& w   ///< a WHERE clause that narrows down the number of returned rows; the WHERE can include conditions on other columns than just 'colName', of course
        )
      :SingleColumnIterator(tab.dbRef(), tab.name(), colName, w) {}

    /** \brief Ctor with most basic parameters (all strings and ints, except
     * for the database.
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     *
     * \note Rows are always iterated in ascending rowid order regardless of
     * any order setting in the WhereClause.
     */
    SingleColumnIterator(
        const SqliteDatabase& db,   ///< the database that contains the table
        const string& tabName,   ///< the table that contains the column
        const string& colName,   ///< the column that shall be iterated over
        const WhereClause& w   ///< a WHERE clause that narrows down the number of returned rows; the WHERE can include conditions on other columns than just 'colName', of course
        )
    {
      init(db, tabName, colName, w);
    }

    /** \brief Empty dtor */
    ~SingleColumnIterator() {}

    /** \brief Default move ctor, boils down to the move ctor
     * of the underlying SqlStatement
     */
    SingleColumnIterator(SingleColumnIterator&& other) = default;

    /** \brief Deleted copy ctor because we can't copy
     * of the underlying SqlStatement
     */
    SingleColumnIterator(const SingleColumnIterator& other) = delete;

    /** \brief Default move assignment, boils down to the move assignment
     * of the underlying SqlStatement
     */
    SingleColumnIterator& operator=(SingleColumnIterator&& other) = default;

    /** \brief Deleted copy assignment because we can't copy
     * of the underlying SqlStatement
     */
    SingleColumnIterator& operator=(const SingleColumnIterator& other) = delete;

    /** \brief Advances to the next row
     *
     * \returns `true` if we found another data row and 'false' if we are beyond the last row
     */
    bool operator++()
    {
      return stmt.step();
    }

    /** \returns 'true' if the iterator points to a data row, 'false' otherwise
     */
    bool hasData() const
    {
      return stmt.hasData();
    }

    /** \returns the column value at the current iterator position
     *
     * \throws NullValueException if the column contains NULL at the current position
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     */
    T operator*() const
    {
      T result;
      stmt.get(1, result);
      return result;
    }

    /** Access to the column value; saves one copy operation compared to
     * `operator*()`.
     *
     * \throws NullValueException if the column contains NULL at the current position
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     *
     * \returns the column value at the current iterator position
     *
     */
    void get(T& result) const
    {
      stmt.get(1, result);
    }

    /** \returns the column value at the current iterator position as an `optional` in
     * order to nicely handle NULL values
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     */
    optional<T> get2() const
    {
      optional<T> result;
      stmt.get(1, result);
      return result;
    }

    /** \returns the rowid at the current iterator position
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     */
    int rowid() const
    {
      return stmt.getInt(0);
    }

  protected:
    void init(
        const SqliteDatabase& db,   ///< the database that contains the table
        const string& tabName,   ///< the table that contains the column
        const string& colName,   ///< the column that shall be iterated over
        const WhereClause& w   ///< any applicable row filter
        )
    {
      string sql = "SELECT rowid," + colName + " FROM " +  tabName;
      if (!w.isEmpty())
      {
        sql += " WHERE " + w.getWherePartWithPlaceholders();
      }
      sql += " ORDER BY rowid ASC";

      stmt = w.createStatementAndBindValuesToPlaceholders(db, sql);
      stmt.step();
    }

  private:
    SqlStatement stmt;
  };


  /** \brief An iterator-like class that allows for easy iteration over
   * all rows of a table
   *
   * This is essentially a wrapper around an underlying SqlStatement that
   * selects the given range of rows. Rows are always
   * iterated in ascending rowid order.
   *
   * Typical usage could look like this:
   * \code
   * for (auto it = tab.rowIterator(); it.hasData(); ++it)
   * {
   *   doSomething();
   * }
   * \endcode
   *
   * \note That this class' interface does not comply with the interface
   * of a "usual" iterator. Such a "usual" iterator would, among other things,
   * require to be `copy-constructible` and `copy-assignable` which is
   * difficult due to our underlying SqlStatement.
   *
   * \note As long as the iterator has not finished, we have a running statement
   * open on the database and on our connection. Keep this in mind when mixing this
   * iterator with transactions and/or concurrent database connections.
   *
   * I'm not sure what happens if you add or remove rows while this iterator and
   * its underlying SqlStatement are active.
   *
   */
  class TabRowIterator
  {
  public:
    /** \brief Ctor with most basic parameters (all strings and ints, except
     * for the database.
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     */
    TabRowIterator(
        const SqliteDatabase& _db,   ///< the database that contains the table
        const string& _tabName,   ///< the table that contains the column
        int minRowId = -1,   ///< return only rows with a rowid greater or equal to this values
        int maxRowId = -1   ///< return only rows with a rowid less or equal to this values
        );

    /** \brief Ctor with a `DbTab` for identifying the database and the table
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     */
    TabRowIterator(
        const DbTab& tab,   ///< the table that contains the column
        int minRowId = -1,   ///< return only rows with a rowid greater or equal to this values
        int maxRowId = -1   ///< return only rows with a rowid less or equal to this values
        )
      :TabRowIterator(tab.dbRef(), tab.name(), minRowId, maxRowId) {}

    /** \brief Ctor with a `DbTab` for identifying the database and the table
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     *
     * \note Rows are always iterated in ascending rowid order regardless of
     * any order setting in the WhereClause.
     */
    TabRowIterator(
        const DbTab& tab,   ///< the table that contains the column
        const WhereClause& w   ///< a WHERE clause that narrows down the number of returned rows; the WHERE can include conditions on other columns than just 'colName', of course
        )
      :TabRowIterator(tab.dbRef(), tab.name(), w) {}

    /** \brief Ctor with most basic parameters (all strings and ints, except
     * for the database.
     *
     * Directly after construction, the iterator points already at the first
     * result row. There is no need for an inital "++()" in order to yield the
     * first row.
     *
     * \note Rows are always iterated in ascending rowid order regardless of
     * any order setting in the WhereClause.
     */
    TabRowIterator(
        const SqliteDatabase& _db,   ///< the database that contains the table
        const string& _tabName,   ///< the table that contains the column
        const WhereClause& w   ///< a WHERE clause that narrows down the number of returned rows; the WHERE can include conditions on other columns than just 'colName', of course
        );

    /** \brief Empty, unspecific dtor; we rely on the dtors of our members */
    ~TabRowIterator() {}

    /** \brief Default move ctor, boils down to the move ctor
     * of the underlying SqlStatement
     */
    TabRowIterator(TabRowIterator&& other) = default;

    /** \brief Deleted copy ctor because we can't copy
     * of the underlying SqlStatement
     */
    TabRowIterator(const TabRowIterator& other) = delete;

    /** \brief Default move assignment, boils down to the move assignment
     * of the underlying SqlStatement
     */
    TabRowIterator& operator=(TabRowIterator&& other) = default;

    /** \brief Deleted copy assignment because we can't copy
     * of the underlying SqlStatement
     */
    TabRowIterator& operator=(const TabRowIterator& other) = delete;

    /** \brief Advances to the next row and prepares a new TabRow instance
     * for that row.
     *
     * \returns `true` if we found another data row and 'false' if we are beyond the last row
     */
    bool operator++();

    /** \returns 'true' if the iterator points to a data row, 'false' otherwise
     */
    bool hasData() const
    {
      return stmt.hasData();
    }

    /** \returns the TabRow at the current iterator position
     *
     * The returned reference is only valid as long as the the iterator is not
     * incremented. And, of course, as long as the iterator itself is alive.
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     */
    TabRow& operator*() const;

    /** \returns the a pointer to a TabRow instance for the current iterator position
     *
     * The returned pointer is only valid as long as the the iterator is not
     * incremented. And, of course, as long as the iterator itself is alive.
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     */
    TabRow* operator->() const;

    /** \returns the rowid at the current iterator position
     *
     * \throws NoDataException if the statement didn't return any data or is already finished
     */
    int rowid() const
    {
      return stmt.getInt(0);
    }

  protected:
    void init(
        const WhereClause& w   ///< any applicable row filter
        );

  private:
    SqlStatement stmt;
    unique_ptr<TabRow> curRow;
    reference_wrapper<const SqliteDatabase> db;
    string tabName;
  };
}
#endif	/* DBTAB_H */

