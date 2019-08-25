/* 
 * File:   GenericObjectManager.h
 * Author: volker
 *
 * Created on March 2, 2014, 8:27 PM
 */

#ifndef SQLITE_OVERLAY_GENERICOBJECTMANAGER_H
#define	SQLITE_OVERLAY_GENERICOBJECTMANAGER_H

#include <vector>
#include <type_traits>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"
#include "TabRow.h"

namespace SqliteOverlay
{
  template<class T, typename DbClass>
  std::vector<T> rowVector2Objects(const DbClass& db, const std::vector<TabRow>& rows)
  {
    std::vector<T> result;
    for_each(rows.begin(), rows.end(), [&](const TabRow& r)
    {
      result.push_back(T{db, r});
    });
    return result;
  }

  //----------------------------------------------------------------------------

  template<class T, typename DbClass, typename ValType>
  std::vector<T> getObjectsByColumnValue(const DbClass& db, const DbTab& objectTab, const std::string& colName, const ValType& val)
  {
    const auto resultVector = objectTab.getRowsByColumnValue(colName, val);
    return rowVector2Objects<T>(db, resultVector);
  }

  //----------------------------------------------------------------------------

  template<class T, typename DbClass>
  std::vector<T> getObjectsByWhereClause(const DbClass& db, const DbTab& objectTab, const WhereClause& w)
  {
    const auto resultVector = objectTab.getRowsByWhereClause(w);
    return rowVector2Objects<T>(db, resultVector);
  }

  //----------------------------------------------------------------------------

  template<class T, typename DbClass>
  std::vector<T> getObjectsByWhereClause(const DbClass& db, const DbTab& objectTab, const std::string& w)
  {
    const auto resultVector = objectTab.getRowsByWhereClause(w);
    return rowVector2Objects<T>(db, resultVector);
  }

  //----------------------------------------------------------------------------

  template<class T, typename DbClass>
  std::vector<T> getAllObjects(const DbClass& db, const DbTab& objectTab)
  {
    const auto resultVector = objectTab.getAllRows();
    return rowVector2Objects<T>(db, resultVector);
  }

  //----------------------------------------------------------------------------

  template<class T, typename DbClass, typename ValType>
  std::optional<T> getSingleObjectByColumnValue(const DbClass& db, const DbTab& objectTab, const std::string& colName, const ValType& val)
  {
    auto r = objectTab.getSingleRowByColumnValue2(colName, val);

    if (r) return T{db, *r};
    return std::optional<T>{};

    // throw all other execeptions, e.g. BUSY
  }

  //----------------------------------------------------------------------------

  template<class T, typename DbClass>
  std::optional<T> getSingleObjectByWhereClause(const DbClass& db, const DbTab& objectTab, const WhereClause& w)
  {
    auto r = objectTab.getSingleRowByWhereClause2(w);

    if (r) return T{db, *r};
    return std::optional<T>{};

    // throw all other execeptions, e.g. BUSY
  }

  //----------------------------------------------------------------------------

  template<class T, typename DbClass>
  std::optional<T> getSingleObjectByWhereClause(const DbClass& db, const DbTab& objectTab, const std::string& w)
  {
    auto r = objectTab.getSingleRowByWhereClause2(w);

    if (r) return T{db, *r};
    return std::optional<T>{};

    // throw all other execeptions, e.g. BUSY
  }

  //----------------------------------------------------------------------------

  /** \brief Filters all rows that contain a given value in a given column; in
   * all rows that match the given value in the given column, look up an integer
   * in another given column; use that integer value as an ID for instanciating
   * a database object of type T, most likely on a second table.
   *
   * Example: a table has the column "ObjType" and "RefId". With this function
   * you can easily search for all rows in which "ObjType" equals "42" and then
   * use the value in "RefId" to instantiate a new object of type T.
   *
   * \returns A list of database objects of type T whose IDs were retrieved from
   * rows in a custom table that matched a specific column-value-pair.
   */
  template<class T, typename DbClass, typename ValType>
  std::vector<T> filterAndDereference(
      const DbClass& db,   ///< the specific, derived database we're operating on
      const DbTab& srcTab,   ///< table that contains the to-be-filtered column and the column with the reference IDs
      const std::string& filterColName,   ///< name of the column in `srcTab` that we should search in
      const ValType& filterValue,   ///< the value to look for in column 'filterColName'
      const std::string& referingCol   ///< the name of the column in `srcTab` that contains the reference IDs
      )
  {
    return filterAndDereference(db, srcTab.name(), filterColName, filterValue, referingCol);
  }

  //----------------------------------------------------------------------------

  /** \brief Filters all rows that contain a given value in a given column; in
   * all rows that match the given value in the given column, look up an integer
   * in another given column; use that integer value as an ID for instanciating
   * a database object of type T, most likely on a second table.
   *
   * Example: a table has the column "ObjType" and "RefId". With this function
   * you can easily search for all rows in which "ObjType" equals "42" and then
   * use the value in "RefId" to instantiate a new object of type T.
   *
   * \returns A list of database objects of type T whose IDs were retrieved from
   * rows in a custom table that matched a specific column-value-pair.
   */
  template<class T, typename DbClass, typename ValType>
  std::vector<T> filterAndDereference(
      const DbClass& db,   ///< the specific, derived database we're operating on
      const std::string srcTabName,   ///< table that contains the to-be-filtered column and the column with the reference IDs
      const std::string& filterColName,   ///< name of the column in `srcTab` that we should search in
      const ValType& filterValue,   ///< the value to look for in column 'filterColName'
      const std::string& referingCol   ///< the name of the column in `srcTab` that contains the reference IDs
      )
  {
    std::string sql{
      "SELECT " + referingCol + " FROM " + srcTabName +
      " WHERE " + filterColName + " = ?"
    };

    auto stmt = db.prepStatement(sql);
    stmt.bind(1, filterValue);

    std::vector<T> result;
    for (stmt.step(); stmt.hasData(); ++stmt)
    {
      result.push_back(T{db, stmt.getInt(0)});
    }

    return result;
  }

  //----------------------------------------------------------------------------

  /** \brief Takes the value of a given column in a given row and uses
   * it to instantiate a new database object of type T
   */
  template<class T, typename DbClass>
  std::optional<T> getSingleReferencedObject(
      const DbClass& db,   ///< the specific, derived database we're operating on
      const TabRow& r,   ///< the row containing the reference ID
      const std::string& refColumnName   ///< the name of the column containing the reference ID
      )
  {
    auto objId = r.getInt2(refColumnName);
    return (objId.has_value()) ? T(db, *objId) : std::optional<T>{};
  }

  //----------------------------------------------------------------------------

  /** \brief Looks up a single row using a WHERE clause and uses value from
   * a given column in that row for instantiating a new database object of type T
   */
  template<class T, typename DbClass>
  std::optional<T> getSingleReferencedObject(
      const DbClass& db,   ///< the specific, derived database we're operating on
      const DbTab& srcTab,   ///< the table on which we should run the WHERE clause
      const WhereClause& w,   ///< the WHERE clause itself
      const std::string& refColumnName  ///< the name of the column in `srcTab` that contains the reference IDs
      )
  {
    std::optional<TabRow> row = srcTab.getSingleRowByWhereClause2(w);
    if (row.has_value())
    {
      return getSingleReferencedObject(db, row.value(), refColumnName);
    }
    return std::optional<TabRow>{};
  }

  //----------------------------------------------------------------------------

  /** \brief Looks up a single row using a column/value-pair and uses value from
   * a given column in that row for instantiating a new database object of type T
   */
  template<class T, typename DbClass, typename ValType>
  std::optional<T> getSingleReferencedObject(
      const DbClass& db,   ///< the specific, derived database we're operating on
      const DbTab& srcTab,   ///< the table on which we should run the WHERE clause
      const std::string& filterCol,   ///< the column to be used for filtering
      const ValType& filterVal,   ///< the value to search for in `filterCol`
      const std::string& refColumnName  ///< the name of the column in `srcTab` that contains the reference IDs
      )
  {
    std::optional<TabRow> row = srcTab.getSingleRowByColumnValue2(filterCol, filterVal);
    if (row.has_value())
    {
      return getSingleReferencedObject(db, row.value(), refColumnName);
    }
    return std::optional<TabRow>{};
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  template<class DB_CLASS = SqliteDatabase>
  class GenericObjectManager
  {
  public:
    using DatabaseClass = DB_CLASS;

    GenericObjectManager (const DatabaseClass& _db, const DbTab& _tab)
      :db(_db), tab(_tab)
    {
      static_assert (std::is_base_of_v<SqliteDatabase, DB_CLASS>, "DB classes must be derived from SqliteDatabase");
    }

    //----------------------------------------------------------------------------

    GenericObjectManager (const DatabaseClass& _db, const std::string& tabName)
      :db(_db), tab{db, tabName, true}
    {
      static_assert (std::is_base_of_v<SqliteDatabase, DB_CLASS>, "DB classes must be derived from SqliteDatabase");
    }

    //----------------------------------------------------------------------------

    const DatabaseClass& getDatabaseHandle()
    {
      return db.get();
    }

    //----------------------------------------------------------------------------

    int getObjCount() const
    {
      return tab.length();
    }

    //----------------------------------------------------------------------------

  protected:
    const DB_CLASS& db;
    DbTab tab;

    template<class T, typename ValType>
    std::vector<T> getObjectsByColumnValue(const std::string& colName, const ValType& val) const
    {
      return SqliteOverlay::getObjectsByColumnValue<T, DB_CLASS, ValType>(db, tab, colName, val);
    }

    template<class T>
    std::vector<T> getObjectsByWhereClause(const WhereClause& w) const
    {
      return SqliteOverlay::getObjectsByWhereClause<T, DB_CLASS>(db, tab, w);
    }

    template<class T>
    std::vector<T> getObjectsByWhereClause(const std::string& w) const
    {
      return SqliteOverlay::getObjectsByWhereClause<T, DB_CLASS>(db, tab, w);
    }

    template<class T>
    std::vector<T> getAllObjects() const
    {
      return SqliteOverlay::getAllObjects<T, DB_CLASS>(db, tab);
    }

    template<class T, typename ValType>
    std::optional<T> getSingleObjectByColumnValue(const std::string& colName, const ValType& val) const
    {
      return SqliteOverlay::getSingleObjectByColumnValue<T, DB_CLASS>(db, tab, colName, val);
    }

    template<class T>
    std::optional<T> getSingleObjectByWhereClause(const WhereClause& w) const
    {
      return SqliteOverlay::getSingleObjectByWhereClause<T, DB_CLASS>(db, tab, w);
    }

    template<class T>
    std::optional<T> getSingleObjectByWhereClause(const std::string& w) const
    {
      return SqliteOverlay::getSingleObjectByWhereClause<T, DB_CLASS>(db, tab, w);
    }

    /** \brief Filters all rows that contain a given value in a given column; in
     * all rows that match the given value in the given column, look up an integer
     * in another given column; use that integer value as an ID for instanciating
     * a database object of type T, most likely on a second table.
     *
     * Example: a table has the column "ObjType" and "RefId". With this function
     * you can easily search for all rows in which "ObjType" equals "42" and then
     * use the value in "RefId" to instantiate a new object of type T.
     *
     * \returns A list of database objects of type T whose IDs were retrieved from
     * rows in a custom table that matched a specific column-value-pair.
     */
    template<class T, typename ValType>
    std::vector<T> filterAndDereference(
        const std::string& filterColName,   ///< name of the column in "our" tab that we should search in
        const ValType& filterValue,   ///< the value to look for in column 'filterColName'
        const std::string& referingCol   ///< the name of the column in "our" tab that contains the reference IDs
        ) const
    {
      return SqliteOverlay::filterAndDereference<T, DB_CLASS, ValType>(db, tab, filterColName, filterValue, referingCol);
    }

    /** \brief Takes the value of a given column in a given row and uses
     * it to instantiate a new database object of type T
     */
    template<class T>
    std::optional<T> getSingleReferencedObject(const TabRow& r, const std::string& refColumnName) const
    {
      return SqliteOverlay::getSingleReferencedObject<T, DB_CLASS>(db, r, refColumnName);
    }

    /** \brief Takes the value of a given column in a row that is the first match in a WHERE clause
     * and uses that column to instantiate a new database object of type T
     */
    template<class T>
    std::optional<T> getSingleReferencedObject(const WhereClause& w, const std::string& refColumnName) const
    {
      return SqliteOverlay::getSingleReferencedObject<T, DB_CLASS>(db, tab, w, refColumnName);
    }

    /** \brief Looks up a single row using a column/value-pair and uses value from
     * a given column in that row for instantiating a new database object of type T
     */
    template<class T, typename ValType>
    std::optional<T> getSingleReferencedObject(
        const std::string& filterCol,   ///< the column to be used for filtering
        const ValType& filterVal,   ///< the value to search for in `filterCol`
        const std::string& refColumnName  ///< the name of the column in `srcTab` that contains the reference IDs
        ) const
    {
      return SqliteOverlay::getSingleReferencedObject<T, DB_CLASS, ValType>(db, tab, filterCol, filterVal, refColumnName);
    }
  };

}

#endif	/* GENERICOBJECTMANAGER_H */

