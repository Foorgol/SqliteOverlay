/* 
 * File:   GenericObjectManager.h
 * Author: volker
 *
 * Created on March 2, 2014, 8:27 PM
 */

#ifndef SQLITE_OVERLAY_GENERICOBJECTMANAGER_H
#define	SQLITE_OVERLAY_GENERICOBJECTMANAGER_H

#include <vector>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"
#include "TabRow.h"

namespace SqliteOverlay
{
  template <class DB_CLASS_TYPE>
  class GenericObjectManager
  {
  public:
    GenericObjectManager (DB_CLASS_TYPE* _db, DbTab* _tab)
      :db(_db), tab(_tab)
    {
      if (db == nullptr)
      {
        throw invalid_argument("Received nullptr for database handle!");
      }
      if (tab == nullptr)
      {
        throw invalid_argument("Received nullptr for table handle!");
      }
    }

    //----------------------------------------------------------------------------

    GenericObjectManager (DB_CLASS_TYPE* _db, const string& tabName)
      :db(_db)
    {
      if (db == nullptr)
      {
        throw invalid_argument("Received nullptr for database handle!");
      }

      tab = db->getTab(tabName);
      if (db == nullptr)
      {
        throw invalid_argument("Received invalid table name for object manager!");
      }
    }

    //----------------------------------------------------------------------------

    DB_CLASS_TYPE* getDatabaseHandle()
    {
      return db;
    }

    //----------------------------------------------------------------------------

    int getObjCount() const
    {
      return tab->length();
    }

    //----------------------------------------------------------------------------

  protected:
    DB_CLASS_TYPE* db;
    DbTab* tab;

    template<class T>
    vector<T> getObjectsByColumnValue(const DbTab* objectTab, const string& colName, int val) const
    {
      DbTab::CachingRowIterator it = objectTab->getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }
    template<class T>
    vector<T> getObjectsByColumnValue(const string& colName, int val) const
    {
      DbTab::CachingRowIterator it = tab->getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getObjectsByColumnValue(const DbTab* objectTab, const string& colName, double val) const
    {
      DbTab::CachingRowIterator it = objectTab->getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }
    template<class T>
    vector<T> getObjectsByColumnValue(const string& colName, double val) const
    {
      DbTab::CachingRowIterator it = tab->getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getObjectsByColumnValue(const DbTab* objectTab, const string& colName, const string& val) const
    {
      DbTab::CachingRowIterator it = objectTab->getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }
    template<class T>
    vector<T> getObjectsByColumnValue(const string& colName, const string& val) const
    {
      DbTab::CachingRowIterator it = tab->getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getObjectsByWhereClause(const DbTab* objectTab, const WhereClause& w) const
    {
      DbTab::CachingRowIterator it = objectTab->getRowsByWhereClause(w);
      return iterator2Objects<T>(it);
    }
    template<class T>
    vector<T> getObjectsByWhereClause(const WhereClause& w) const
    {
      DbTab::CachingRowIterator it = tab->getRowsByWhereClause(w);
      return iterator2Objects<T>(it);
    }
    template<class T>
    vector<T> getObjectsByWhereClause(const string& w) const
    {
      DbTab::CachingRowIterator it = tab->getRowsByWhereClause(w);
      return iterator2Objects<T>(it);
    }
    template<class T>
    vector<T> getObjectsByWhereClause(const DbTab* objectTab, const string& w) const
    {
      DbTab::CachingRowIterator it = objectTab->getRowsByWhereClause(w);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getAllObjects(const DbTab* objectTab) const
    {
      DbTab::CachingRowIterator it = objectTab->getAllRows();
      return iterator2Objects<T>(it);
    }
    template<class T>
    vector<T> getAllObjects() const
    {
      DbTab::CachingRowIterator it = tab->getAllRows();
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> iterator2Objects(DbTab::CachingRowIterator& it) const
    {
      vector<T> result;
      while (it.pointsToElement())
      {
        result.push_back(T(db, *it));
        ++it;
      }
      return result;
    }

    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const DbTab* objectTab, const string& colName, int val) const
    {
      try
      {
        TabRow r = objectTab->getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }
    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const string& colName, int val) const
    {
      try
      {
        TabRow r = tab->getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }

    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const DbTab* objectTab, const string& colName, double val) const
    {
      try
      {
        TabRow r = objectTab->getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }
    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const string& colName, double val) const
    {
      try
      {
        TabRow r = tab->getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }

    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const DbTab* objectTab, const string& colName, const string& val) const
    {
      try
      {
        TabRow r = objectTab->getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }
    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const string& colName, const string& val) const
    {
      try
      {
        TabRow r = tab->getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }

    template<class T>
    unique_ptr<T> getSingleObjectByWhereClause(const DbTab* objectTab, const WhereClause& w) const
    {
      try
      {
        TabRow r = objectTab->getSingleRowByWhereClause(w);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }
    template<class T>
    unique_ptr<T> getSingleObjectByWhereClause(const WhereClause& w) const
    {
      try
      {
        TabRow r = tab->getSingleRowByWhereClause(w);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }
    template<class T>
    unique_ptr<T> getSingleObjectByWhereClause(const DbTab* objectTab, const string& w) const
    {
      try
      {
        TabRow r = objectTab->getSingleRowByWhereClause(w);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }
    template<class T>
    unique_ptr<T> getSingleObjectByWhereClause(const string& w) const
    {
      try
      {
        TabRow r = tab->getSingleRowByWhereClause(w);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }

    template<class T>
    vector<T> resolveMappingAndGetObjects(const string& mappingTabName, const string& keyColumnName, int keyColumnValue, const string& mappedIdColumnName) const
    {
      string sql = "SELECT " + mappedIdColumnName + " FROM " + mappingTabName + " WHERE ";
      sql += keyColumnName + " = " + to_string(keyColumnValue);

      int dbErr;
      auto qry = db->execContentQuery(sql, &dbErr);
      if (((dbErr != SQLITE_ROW) && (dbErr != SQLITE_DONE) && (dbErr != SQLITE_OK)) || (qry == nullptr))
      {
        return vector<T>();
      }

      vector<T> result;
      while (qry->hasData())
      {
        int refId;
        qry->getInt(0, &refId);
        auto obj = T(db, refId);
        result.push_back(obj);

        qry->step();
      }

      return result;
    }

    template<class T>
    unique_ptr<T> getSingleReferencedObject(const TabRow& r, const string& refColumnName) const
    {
      auto objId = r.getInt2(refColumnName);
      if (objId->isNull()) return nullptr;
      return unique_ptr<T>(new T(db, objId->get()));
    }

    template<class T>
    unique_ptr<T> getSingleReferencedObject(DbTab* srcTab, const WhereClause& w, const string& refColumnName) const
    {
      try
      {
        TabRow r = srcTab->getSingleRowByWhereClause(w);
        return getSingleReferencedObject(r, refColumnName);
      } catch (...) {
      }
      return nullptr;
    }

    template<class T>
    unique_ptr<T> getSingleReferencedObject(const WhereClause& w, const string& refColumnName) const
    {
      return getSingleReferencedObject<T>(tab, w, refColumnName);
    }

  };

}

#endif	/* GENERICOBJECTMANAGER_H */

