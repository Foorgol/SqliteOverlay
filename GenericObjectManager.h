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
  template<class DB_CLASS = SqliteDatabase>
  class GenericObjectManager
  {
  public:
    using DatabaseClass = DB_CLASS;

    GenericObjectManager (const DatabaseClass& _db, DbTab _tab)
      :db(_db), tab(_tab)
    {
      static_assert (std::is_base_of_v<SqliteDatabase, DB_CLASS>, "DB classes must be derived from SqliteDatabase");
    }

    //----------------------------------------------------------------------------

    GenericObjectManager (const DatabaseClass& _db, const string& tabName)
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
    reference_wrapper<const DB_CLASS> db;
    DbTab tab;

    template<class T, typename ValType>
    vector<T> getObjectsByColumnValue(const DbTab& objectTab, const string& colName, const ValType& val) const
    {
      const auto& resultVector = objectTab.getRowsByColumnValue(colName, val);
      return vector2Objects<T>(resultVector);
    }

    template<class T, typename ValType>
    vector<T> getObjectsByColumnValue(const string& colName, const ValType& val) const
    {
      return getObjectsByColumnValue<T>(tab, colName, val);
    }

    template<class T>
    vector<T> getObjectsByWhereClause(const DbTab& objectTab, const WhereClause& w) const
    {
      const auto& resultVector = objectTab.getRowsByWhereClause(w);
      return vector2Objects<T>(resultVector);
    }

    template<class T>
    vector<T> getObjectsByWhereClause(const WhereClause& w) const
    {
      return getObjectsByWhereClause<T>(tab, w);
    }

    template<class T>
    vector<T> getObjectsByWhereClause(const DbTab& objectTab, const string& w) const
    {
      const auto& resultVector = objectTab.getRowsByWhereClause(w);
      return vector2Objects<T>(resultVector);
    }

    template<class T>
    vector<T> getObjectsByWhereClause(const string& w) const
    {
      return getObjectsByWhereClause<T>(tab, w);
    }

    template<class T>
    vector<T> getAllObjects(const DbTab& objectTab) const
    {
      const auto& resultVector = objectTab.getAllRows();
      return vector2Objects<T>(resultVector);
    }

    template<class T>
    vector<T> getAllObjects() const
    {
      return getAllObjects<T>(tab);
    }

    template<class T>
    vector<T> vector2Objects(const vector<TabRow>& rows) const
    {
      vector<T> result;
      for_each(rows.begin(), rows.end(), [&](const TabRow& r)
      {
        result.push_back(T{db, r});
      });
      return result;
    }

    template<class T, typename ValType>
    optional<T> getSingleObjectByColumnValue(const DbTab& objectTab, const string& colName, const ValType& val) const
    {
      try
      {
        TabRow r = objectTab.getSingleRowByColumnValue(colName, val);
        return T{db, r};
      } catch (NoDataException e) {
        return optional<T>{};
      }

      // throw all other execeptions, e.g. BUSY
    }

    template<class T, typename ValType>
    optional<T> getSingleObjectByColumnValue(const string& colName, const ValType& val) const
    {
      return getSingleObjectByColumnValue<T>(tab, colName, val);
    }


    template<class T>
    optional<T> getSingleObjectByWhereClause(const DbTab& objectTab, const WhereClause& w) const
    {
      try
      {
        TabRow r = objectTab.getSingleRowByWhereClause(w);
        return T{db, r};
      } catch (NoDataException e) {
        return optional<T>{};
      }

      // throw all other execeptions, e.g. BUSY
    }

    template<class T>
    optional<T> getSingleObjectByWhereClause(const WhereClause& w) const
    {
      return getSingleObjectByWhereClause<T>(tab, w);
    }

    template<class T>
    optional<T> getSingleObjectByWhereClause(const DbTab& objectTab, const string& w) const
    {
      try
      {
        TabRow r = objectTab.getSingleRowByWhereClause(w);
        return T{db, r};
      } catch (NoDataException e) {
        return optional<T>{};
      }

      // throw all other execeptions, e.g. BUSY
    }
    template<class T>
    optional<T> getSingleObjectByWhereClause(const string& w) const
    {
      return getSingleObjectByWhereClause<T>(tab, w);
    }

    /** \brief Filters all rows that contain a given value in a given column; in
     * all rows that match the given value in the given column, look up an integer
     * in another given column; use that integer value as an ID for instanciating
     * a database object of type T.
     *
     * Example: a table has the column "RefId" and "ObjType". With this function
     * you can easily search for all rows in which "ObjType" equals "42" and then
     * use the value in "RefId" to instantiate a new object of type T.
     *
     * \returns A list of database objects of type T whose IDs were retrieved from
     * rows in a custom table that matched a specific column-value-pair.
     */
    template<class T>
    vector<T> resolveMappingAndGetObjects(
        const string& mappingTabName,   ///< the name of the table that contains the references and the key column
        const string& keyColumnName,   ///< the column in that table that contains the key
        int keyColumnValue,   ///< the value that the result rows should match
        const string& mappedIdColumnName   ///< the name of the column that holds the requested object IDs
        ) const
    {
      string sql = "SELECT " + mappedIdColumnName + " FROM " + mappingTabName + " WHERE ";
      sql += keyColumnName + " = " + to_string(keyColumnValue);

      auto stmt = db.get().prepStatement(sql);
      stmt.step();

      vector<T> result;
      while (stmt.hasData())
      {
        auto obj = T(db.get(), stmt.getInt(0));
        result.push_back(obj);
        stmt.step();
      }

      return result;
    }

    /** \brief Takes the value of a given column in a given row and uses
     * it to instantiate a new database object of type T
     */
    template<class T>
    optional<T> getSingleReferencedObject(const TabRow& r, const string& refColumnName) const
    {
      auto objId = r.getInt2(refColumnName);
      return (objId.has_value()) ? T(db.get(), objId.value()) : optional<T>{};
    }

    /** \brief Takes the value of a given column in a row that is the first match in a WHERE clause
     * and uses that column to instantiate a new database object of type T
     */
    template<class T>
    optional<T> getSingleReferencedObject(const DbTab& srcTab, const WhereClause& w, const string& refColumnName) const
    {
      optional<TabRow> row = srcTab.getSingleRowByWhereClause2(w);
      if (row.has_value())
      {
        return getSingleReferencedObject(row.value(), refColumnName);
      }
      return optional<TabRow>{};
    }

    /** \brief Takes the value of a given column in a row that is the first match in a WHERE clause
     * and uses that column to instantiate a new database object of type T
     */
    template<class T>
    optional<T> getSingleReferencedObject(const WhereClause& w, const string& refColumnName) const
    {
      return getSingleReferencedObject<T>(tab, w, refColumnName);
    }

  };

}

#endif	/* GENERICOBJECTMANAGER_H */

