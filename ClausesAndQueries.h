#ifndef SQLITE_OVERLAY_CLAUSESANDQUERIES_H
#define	SQLITE_OVERLAY_CLAUSESANDQUERIES_H

#include <vector>

#include <Sloppy/DateTime/DateAndTime.h>

#include "SqliteDatabase.h"

using namespace Sloppy::DateTime;

namespace SqliteOverlay {

  /** \brief A class that handles everything that an INSERT, DELETE, UPDATE or SELECT clause
   * have in common; is not really designed for usage outside of `WhereClause` or `ColumnValueClause`. */
  class CommonClause
  {
  public:
    /** \brief Default, empty ctor */
    CommonClause() = default;

    /** \brief Default, empty dtor */
    virtual ~CommonClause() = default;

    /** \brief Adds an integer to the list of column values;
     * uses the default operation "=" (equals) between column name and value.
     *
     * Test case: yes
     *
     */
    inline void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        int val   ///< the value itself
        )
    {
      intVals.push_back(val);
      colVals.push_back(ColValInfo{colName, ColValType::Int, static_cast<int>(intVals.size()) - 1, ""});
    }

    /** \brief Adds a long int to the list of column values;
     * uses the default operation "=" (equals) between column name and value.
     *
     * Test case: yes
     *
     */
    inline void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        long val   ///< the value itself
        )
    {
      longVals.push_back(val);
      colVals.push_back(ColValInfo{colName, ColValType::Long, static_cast<int>(longVals.size()) - 1, ""});
    }

    /** \brief Adds a double to the list of column values;
     * uses the default operation "=" (equals) between column name and value.
     *
     * Test case: yes
     *
     */
    inline void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        double val   ///< the value itself
        )
    {
      doubleVals.push_back(val);
      colVals.push_back(ColValInfo{colName, ColValType::Double, static_cast<int>(doubleVals.size()) - 1, ""});
    }

    /** \brief Adds a string to the list of column values;
     * uses the default operation "=" (equals) between column name and value.
     *
     * Test case: yes
     *
     */
    void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const string& val   ///< the value itself
        )
    {
      stringVals.push_back(val);
      colVals.push_back(ColValInfo{colName, ColValType::String, static_cast<int>(stringVals.size()) - 1, ""});
    }

    /** \brief Adds a 'IS NULL' to the list of column values
     *
     * Test case: yes
     *
     */
    inline void addNullCol(
        const string& colName   ///< the name of the column that should be NULL
        )
    {
      colVals.push_back(ColValInfo{colName, ColValType::Null, -1, ""});
    }

    /** \brief Adds a timestamp to the list of column values;
     * uses the default operation "=" (equals) between column name and value.
     *
     * Test case: yes
     *
     */
    inline void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const CommonTimestamp* pTimestamp   ///< the value itself
        )
    {
      addCol(colName, pTimestamp->getRawTime());
    }

    /** \brief Adds a date to the list of column values;
     * uses the default operation "=" (equals) between column name and value.
     *
     * Test case: yes
     *
     */
    inline void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const boost::gregorian::date& d   ///< the value itself
        )
    {
      addCol(colName, boost::gregorian::to_int(d));
    }

    /** \brief Deletes all column names and values from the internal lists and
     * thus completey resets the object.
     *
     * Test case: yes
     *
     */
    void clear();

    /** \brief Takes a SQL string with placeholders, converts it into a
     * SqlStatement object and binds all values to the placeholders.
     *
     * The binding to the placeholders takes place in the sequence the column
     * values have been added by calling `addXXXCol()'.
     *
     * Test case: yes, as part of `getUpdateStmt()` and `getInsertStmt()`
     *
     */
    SqlStatement createStatementAndBindValuesToPlaceholders(
        const SqliteDatabase& db,  ///< the database on which to construct the statement
        const string& sql   ///< the SQL statement as text with placeholders
        ) const;

  protected:
    enum ColValType
    {
      Int,
      Long,
      Double,
      String,
      Null,
      NotNull,
    };

    struct ColValInfo
    {
      string colName;
      ColValType type;
      int indexInList;
      string op;
    };

    vector<int> intVals;
    vector<long> longVals;
    vector<double> doubleVals;
    vector<string> stringVals;

    vector<ColValInfo> colVals;
  };

  //----------------------------------------------------------------------------

  /** \brief Constructs INSERT or UDPATE statements with a list columns and their values
   */
  class ColumnValueClause : public CommonClause
  {
  public:
    ColumnValueClause()
      :CommonClause() {}

    /** \brief Constructs an INSERT statement for a given database and table name
     * from the previously provided column values.
     *
     * If no column values have been defined, the resulting statement will
     * be, "`INSERT INTO xxx DEFAULT VALUES`".
     *
     * \throws std::invalid argument if the database pointer is `nullptr` or the table name empty
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a SqlStatement object with an INSERT statement and all placeholders assigned
     *
     * Test case: yes
     *
     */
    SqlStatement getInsertStmt(
        const SqliteDatabase& db,   ///< pointer to the database that contains the target table
        const string& tabName   ///< name of the table in which the new row should be inserted
        ) const;

    /** \brief Constructs an UPDATE statement for a given combination of
     * database, table name and row ID; the UPDATE statement re-assigns
     * the previously provided column values.
     *
     * \throws std::invalid argument if the database pointer is `nullptr`, the table name empty or if no column
     * values have been defined so far
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a SqlStatement object with an UPDATE statement and all placeholders assigned
     *
     * Test case: yes
     *
     */
    SqlStatement getUpdateStmt(
        const SqliteDatabase& db,   ///< pointer to the database that contains the target table
        const string& tabName,   ///< name of the table in which the row should be updated
        int rowId   ///< the ID of the row that should be updated
        ) const;

    /** \returns `true` if this objects contains any column definitions at all */
    bool hasColumns() const;

  private:
  };

  //----------------------------------------------------------------------------

  /** \brief Constructs a WHERE clause from a list columns and their values
   */
  class WhereClause : public CommonClause
  {
  public:
    WhereClause()
      :CommonClause() {}

    using CommonClause::addCol;
    using CommonClause::addNullCol;


    /** \brief Adds an integer to the list of column values with a custom
     * operator (e.g., "!=") between column name and value. */
    void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const string& op,   ///< the operator between column name and value
        int val   ///< the value itself
        );

    /** \brief Adds a long integer to the list of column values with a custom
     * operator (e.g., "!=") between column name and value. */
    void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const string& op,   ///< the operator between column name and value
        long val   ///< the value itself
        );

    /** \brief Adds a double to the list of column values with a custom
     * operator (e.g., "!=") between column name and value. */
    void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const string& op,   ///< the operator between column name and value
        double val   ///< the value itself
        );

    /** \brief Adds a string to the list of column values with a custom
     * operator (e.g., "!=") between column name and value. */
    void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const string& op,   ///< the operator between column name and value
        const string& val   ///< the value itself
        );

    /** \brief Adds a timestamp to the list of column values with a custom
     * operator (e.g., "!=") between column name and value. */
    void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const string& op,   ///< the operator between column name and value
        const CommonTimestamp* pTimestamp   ///< the value itself
        );

    /** \brief Adds a 'IS NOT NULL' to the list of column values */
    inline void addNotNullCol(
        const string& colName
        )
    {
      colVals.push_back(ColValInfo{colName, ColValType::NotNull, -1, ""});
    }

    /** \brief Adds a date to the list of column values with a custom
     * operator (e.g., "!=") between column name and value. */
    inline void addCol(
        const string& colName,   ///< the name of the column that should contain the value
        const string& op,   ///< the operator between column name and value
        const boost::gregorian::date& d   ///< the value itself
        )
    {
      addCol(colName, op, boost::gregorian::to_int(d));
    }

    /** \brief Constructs a "`SELECT id`" or "`SELECT COUNT(*)`" statement for a given
     * database and table name, the statement using a WHERE clause
     * with the previously assigned column-value-pairs.
     *
     * \throws std::invalid argument if the database pointer is `nullptr`, the table name empty or if no column
     * values have been defined so far
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a SqlStatement object with a SELECT statement and all placeholders assigned
     */
    SqlStatement getSelectStmt(
        const SqliteDatabase& db,   ///< the database on which to construct the statement
        const string& tabName,   ///< the table on which the SELECT query should be run
        bool countOnly   ///< `true`: retrieve only the number of matchtes ("SELECT COUNT(*) ... ") instead of the actual matches
        ) const;

    /** \brief Constructs a DELETE statement for a given
     * database and table name, the statement using a WHERE clause
     * with the previously assigned column-value-pairs.
     *
     * \throws std::invalid argument if the database pointer is `nullptr`, the table name empty or if no column
     * values have been defined so far
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a SqlStatement object with a DELETE statement and all placeholders assigned
     */
    SqlStatement getDeleteStmt(
        const SqliteDatabase& db,   ///< the database on which to construct the statement
        const string& tabName   ///< the table on which the DELETE query should be run
        ) const;

    /** \returns `true` if this objects does not contain any column definitions at all */
    bool isEmpty() const;

    /** \brief For SELECT statements: define a column that will be used
     * for ordering the results in ascending order.
     *
     * \note You can also provide a comma separated list of column values if you
     * want to have second or third order sorting.
     */
    void setOrderColumn_Asc(
        const string& colName   ///< the name of the column(s) that should be used for sorting the SELECT results
        );

    /** \brief For SELECT statements: define a column that will be used
     * for ordering the results in descending order.
     *
     * \note You can also provide a comma separated list of column values if you
     * want to have second or third order sorting.
     */
    void setOrderColumn_Desc(
        const string& colName   ///< the name of the column(s) that should be used for sorting the SELECT results
        );

    /** \brief For SELECT statements: limit the list of results to a maximum number of rows
     */
    void setLimit(
        int _limit   ///< the limit for the number of returned rows (ignored if less than 1)
        );

  protected:
    string getWherePartWithPlaceholders() const;

  private:
    string orderBy;
    int limit{0};
  };

}

#endif  /* CLAUSESANDQUERIES_H */
