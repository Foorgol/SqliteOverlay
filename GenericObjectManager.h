/* 
 * File:   GenericObjectManager.h
 * Author: volker
 *
 * Created on March 2, 2014, 8:27 PM
 */

#ifndef GENERICOBJECTMANAGER_H
#define	GENERICOBJECTMANAGER_H

#include <vector>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"

namespace SqliteOverlay
{
  class GenericObjectManager
  {
  public:
    GenericObjectManager (SqliteDatabase* _db);
    SqliteDatabase* getDatabaseHandle();
    
  protected:
    SqliteDatabase* db;

    template<class T>
    vector<T> getObjectsByColumnValue(const DbTab& objectTab, const string& colName, int val) const
    {
      DbTab::CachingRowIterator it = objectTab.getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getObjectsByColumnValue(const DbTab& objectTab, const string& colName, double val) const
    {
      DbTab::CachingRowIterator it = objectTab.getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getObjectsByColumnValue(const DbTab& objectTab, const string& colName, const string& val) const
    {
      DbTab::CachingRowIterator it = objectTab.getRowsByColumnValue(colName, val);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getObjectsByWhereClause(const DbTab& objectTab, const WhereClause& w) const
    {
      DbTab::CachingRowIterator it = objectTab.getRowsByWhereClause(w);
      return iterator2Objects<T>(it);
    }

    template<class T>
    vector<T> getAllObjects(const DbTab& objectTab) const
    {
      DbTab::CachingRowIterator it = objectTab.getAllRows();
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
    unique_ptr<T> getSingleObjectByColumnValue(const DbTab& objectTab, const string& colName, int val) const
    {
      try
      {
        TabRow r = objectTab.getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }

    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const DbTab& objectTab, const string& colName, double val) const
    {
      try
      {
        TabRow r = objectTab.getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }

    template<class T>
    unique_ptr<T> getSingleObjectByColumnValue(const DbTab& objectTab, const string& colName, const string& val) const
    {
      try
      {
        TabRow r = objectTab.getSingleRowByColumnValue(colName, val);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }

    template<class T>
    unique_ptr<T> getSingleObjectByWhereClause(const DbTab& objectTab, const WhereClause& w) const
    {
      try
      {
        TabRow r = objectTab.getSingleRowByWhereClause(w);
        return unique_ptr<T>(new T(db, r));
      } catch (std::exception e) {
      }
      return nullptr;
    }
  };

}

#endif	/* GENERICOBJECTMANAGER_H */

