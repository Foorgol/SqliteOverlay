#pragma once

#include <algorithm>                      // for max
#include <cstdint>                        // for int64_t
#include <string>                         // for string, basic_string
#include <vector>                         // for vector, allocator

#include <Sloppy/DateTime/DateAndTime.h>  // for WallClockTimepoint_secs
#include <Sloppy/DateTime/date.h>         // for year_month_day
#include <Sloppy/json.hpp>                // for json

#include "SqlStatement.h"                 // for SqlStatement

namespace SqliteOverlay {

  class SqliteDatabase;

  /** \brief A class that handles everything that an INSERT, DELETE, UPDATE or SELECT clause
   * have in common; is not really designed for usage outside of `WhereClause` or `ColumnValueClause`. */
  class CommonClause
  {
  public:
    /** \brief Default, empty ctor */
    CommonClause() = default;

    /** \brief Default, empty dtor */
    virtual ~CommonClause() = default;

    template<class T>
    void addCol(
        std::string_view colName,
        T&& val
        )
    {
      addCol(colName, "=", std::forward<T>(val));
    }

    template<class T>
    void addCol(
        std::string_view colName,
        std::string_view op,
        const T& val
        )
    {
      ColValInfo cvi{
        .colName = std::string{colName},
        .type = ColValType::Int,   // dummy, will be overwritten later
        .indexInList = -1,         // dummy, will be overwritten later
        .op = std::string{op}
      };

      if constexpr(std::is_same_v<T, int> || std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
        intVals.push_back(val);
        cvi.indexInList = static_cast<int>(intVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
        intVals.push_back(static_cast<int>(val));
        cvi.indexInList = static_cast<int>(intVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, int64_t>) {
        longVals.push_back(val);
        cvi.type = ColValType::Long;
        cvi.indexInList = static_cast<int>(longVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, double>) {
        doubleVals.push_back(val);
        cvi.type = ColValType::Double;
        cvi.indexInList = static_cast<int>(doubleVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, bool>) {
        intVals.push_back(val ? 1 : 0);
        cvi.indexInList = static_cast<int>(intVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, std::string>) {
        stringVals.push_back(val);
        cvi.type = ColValType::String;
        cvi.indexInList = static_cast<int>(stringVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, std::string_view>) {
        stringVals.push_back(std::string{val});
        cvi.type = ColValType::String;
        cvi.indexInList = static_cast<int>(stringVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, nlohmann::json>) {
        stringVals.push_back(val.dump());
        cvi.type = ColValType::String;
        cvi.indexInList = static_cast<int>(stringVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, Sloppy::DateTime::WallClockTimepoint_secs>) {
        longVals.push_back(val.to_time_t());
        cvi.type = ColValType::Long;
        cvi.indexInList = static_cast<int>(longVals.size()) - 1;
      }
      else if constexpr(std::is_same_v<T, date::year_month_day>) {
        intVals.push_back(Sloppy::DateTime::intFromYmd(val));
        cvi.indexInList = static_cast<int>(intVals.size()) - 1;
      }
      else {
        static_assert (!std::is_same_v<T, T>, "addCol with unsupported value type");
      }

      colVals.push_back(cvi);
    }

    /** \brief Adds a C-string or string literal to the list of column values;
     * uses the default operation "=" (equals) between column name and value.
     *
     * Test case: yes
     *
     */
    void addCol(
        const std::string& colName,   ///< the name of the column that should contain the value
        const char* val   ///< the value itself
        )
    {
      stringVals.push_back(std::string{val});
      colVals.push_back(ColValInfo{colName, ColValType::String, static_cast<int>(stringVals.size()) - 1, ""});
    }

    /** \brief Adds a 'IS NULL' to the list of column values
     *
     * Test case: yes
     *
     */
    inline void addNullCol(
        const std::string& colName   ///< the name of the column that should be NULL
        )
    {
      colVals.push_back(ColValInfo{colName, ColValType::Null, -1, ""});
    }

    /** \brief Deletes all column names and values from the internal lists and
     * thus completey resets the object.
     *
     * Test case: yes
     *
     */
    virtual void clear();

    /** \returns `true` if this objects does not contain any column definitions at all
     *
     * Test case: as part of the WHERE-clause tests
     *
     */
    bool isEmpty() const;

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
        const std::string& sql   ///< the SQL statement as text with placeholders
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
      std::string colName;
      ColValType type;
      int indexInList;
      std::string op;
    };

    std::vector<int> intVals;
    std::vector<int64_t> longVals;
    std::vector<double> doubleVals;
    std::vector<std::string> stringVals;

    std::vector<ColValInfo> colVals;
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
        const std::string& tabName   ///< name of the table in which the new row should be inserted
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
        const std::string& tabName,   ///< name of the table in which the row should be updated
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

    /** \brief Adds a 'IS NOT NULL' to the list of column values
     *
     * Test case: yes
     *
     */
    inline void addNotNullCol(
        const std::string& colName
        )
    {
      colVals.push_back(ColValInfo{colName, ColValType::NotNull, -1, ""});
    }

    /** \brief Constructs a "`SELECT rowid`" or "`SELECT COUNT(*)`" statement for a given
     * database and table name, the statement using a WHERE clause
     * with the previously assigned column-value-pairs.
     *
     * For the special case of `countOnly = true` and no column values defined, we create
     * a SELECT clause that counts all rows in the table.
     *
     * \throws std::invalid argument if the table name is empty or if no column
     * values have been defined so far (unless `countOnly` is `true`).
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a SqlStatement object with a SELECT statement and all placeholders assigned
     *
     * Test case: yes
     *
     */
    SqlStatement getSelectStmt(
        const SqliteDatabase& db,   ///< the database on which to construct the statement
        const std::string& tabName,   ///< the table on which the SELECT query should be run
        bool countOnly   ///< `true`: retrieve only the number of matchtes ("SELECT COUNT(*) ... ") instead of the actual matches
        ) const;

    /** \brief Constructs a DELETE statement for a given
     * database and table name, the statement using a WHERE clause
     * with the previously assigned column-value-pairs.
     *
     * \throws std::invalid argument if the table name is empty or if no column
     * values have been defined so far
     *
     * \throws SqlStatementCreationError if the statement could not be created, most likely due to invalid SQL syntax
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * \returns a SqlStatement object with a DELETE statement and all placeholders assigned
     *
     * Test case: yes
     *
     */
    SqlStatement getDeleteStmt(
        const SqliteDatabase& db,   ///< the database on which to construct the statement
        const std::string& tabName   ///< the table on which the DELETE query should be run
        ) const;

    /** \brief For SELECT statements only: define a column that will be used
     * for ordering the results in ascending order.
     *
     * \note You can also provide a comma separated list of column values if you
     * want to have second or third order sorting.
     *
     * Test case: yes
     *
     */
    void setOrderColumn_Asc(
        const std::string& colName   ///< the name of the column(s) that should be used for sorting the SELECT results
        );

    /** \brief For SELECT statements only: define a column that will be used
     * for ordering the results in descending order.
     *
     * \note You can also provide a comma separated list of column values if you
     * want to have second or third order sorting.
     *
     * Test case: yes
     *
     */
    void setOrderColumn_Desc(
        const std::string& colName   ///< the name of the column(s) that should be used for sorting the SELECT results
        );

    /** \brief For SELECT statements only: limit the list of results to a maximum number of rows
     *
     * Test case: yes
     *
     */
    void setLimit(
        int _limit   ///< the limit for the number of returned rows (ignored if less than 1)
        );

    /** \brief Deletes all column names, values. limits and sort orders from the internal lists and
     * thus completey resets the object.
     *
     * Test case: yes
     *
     */
    void clear() override;

    /** \returns a string that contains the SQL text that you would expect after the `WHERE` in
     * a SELECT statement, for instance; all column values are represented by `?`-placeholders.
     */
    std::string getWherePartWithPlaceholders(
        bool includeOrderByAndLimit   ///< if `true`, a potentially defined `ORDER BY` and `LIMIT' statement is appended to the `WHERE` part
        ) const;

  private:
    std::string orderBy;
    int limit{0};
  };

}

