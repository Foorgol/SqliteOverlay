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

#include <QtCore/qjsonarray.h>
#include <memory>

#include "DbTab.h"
#include "HelperFunc.h"
#include "dbExceptions.h"

namespace dbOverlay
{
  QHash<QString, DbTab> DbTab::tabCache;

//----------------------------------------------------------------------------

  DbTab DbTab::getTab(GenericDatabase* _db, const QString& _tabName)
  {
    QHash<QString, DbTab>::const_iterator i = tabCache.find(_tabName);
    while (i != tabCache.end())
    {
      if (((*i).db == _db) && ((*i).tabName == _tabName))
      {
        return (*i);
      }
      
      i++;
    }
    
    if (tabCache.keys().contains(_tabName))
    {
      tabCache.remove(_tabName);
    }
    
    DbTab newTab(_db, _tabName);
    tabCache.insert(_tabName, newTab);
    
    return newTab;
  }

//----------------------------------------------------------------------------

  DbTab::~DbTab()
  {
  }

//----------------------------------------------------------------------------

  DbTab GenericDatabase::getTab(const QString& name)
  {
    return DbTab::getTab(this, name);
  }
  
  DbTab GenericDatabase::operator [](const QString& name)
  {
    return DbTab::getTab(this, name);
  }
  
//----------------------------------------------------------------------------

  DbTab::DbTab(GenericDatabase* _db, const QString& _tabName)
  : CommonTabularClass (_db, _tabName, false)
  {

  }

//----------------------------------------------------------------------------

  void DbTab::clearTabCache()
  {
    tabCache.clear();
  }

//----------------------------------------------------------------------------

  int DbTab::insertRow(const QVariantList& args)
  {
    if ((args.length() % 2) != 0)
    {
      throw std::invalid_argument("Insert row: Need an even number of arguments (column / value pairs)");
    }
    
    // split the combined column/values list in "args" into two separate lists
    QStringList colNames;
    QStringList placeholders;
    QVariantList vals;
    QVariantList::const_iterator i = args.begin();
    while (i != args.end())
    {
      colNames << (*i).toString();
      i++;
      vals << (*i);
      i++;
      placeholders << "?";
    }
    
    // create the SQL statement
    QString sql;
    if (args.length() > 0)
    {
      sql = "INSERT INTO " + tabName + " (";
      sql += commaSepStringFromList(colNames);
      sql += ") VALUES (";
      sql += commaSepStringFromList(placeholders);
      sql += ")";
    } else
    {
      // special case: insert default values only
      if (db->getDbType() == GenericDatabase::SQLITE)
      {
        sql = "INSERT INTO " + tabName + " DEFAULT VALUES";
      } else {
        sql = "INSERT INTO " + tabName + " () VALUES ()";
      }
      vals.clear();  // shouldn't be necessary...
    }
    
    // execute the insert
    int result = db->execNonQuery(sql, vals);
    if (result != 1)
    {
      //throw WriteDataFailedException(QString2String(tabName));
      return -1;
    }
    
    // get the ID of the last insert
    return db->getLastInsertId();
  }

//----------------------------------------------------------------------------

  int DbTab::insertRow()
  {
    QVariantList emptyList;
    return insertRow(emptyList);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::operator [](const int id) const
  {
    return TabRow(db, tabName, id);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::operator [](const QVariantList& args) const
  {
    return TabRow(db, tabName, args);
  }

//----------------------------------------------------------------------------

  DbTab::CachingRowIterator::CachingRowIterator(GenericDatabase* _db, const QString& _tabName, QSqlQuery& qry)
  : db(_db), tabName(_tabName)
  {
    // no checks for the validity of _db and _tabName here, because
    // we assume that the constructor is only called internally with
    // pre-checked values
    idList = QList<int>();
    
    // make sure that the query is valid
    if (!(qry.isActive()))
    {
      throw std::invalid_argument("CachingRowIterator: inactive query provided to constructor");
    }
    if (!(qry.isSelect()))
    {
      throw std::invalid_argument("CachingRowIterator: non-SELECT query provided to constructor");
    }
    
    // iterate over all matches/results in the query and cache their row IDs
    if (qry.first())
    {
      do
      {
        idList << qry.value(0).toInt();  // we implicitly assume that column 0 is the ID column!
      }  while (qry.next());
    }
    
    qry.finish();
    listIter = idList.begin();
  }


//----------------------------------------------------------------------------

  bool DbTab::CachingRowIterator::isEnd() const
  {
    return (listIter == idList.end());
  }

//----------------------------------------------------------------------------

  bool DbTab::CachingRowIterator::isValid() const
  {
    return (idList.length() != 0);
  }

//----------------------------------------------------------------------------

  void DbTab::CachingRowIterator::operator ++()
  {
    listIter++;
  }

//----------------------------------------------------------------------------

  TabRow DbTab::CachingRowIterator::operator *() const
  {
    int id = *listIter;
    
    return TabRow(db, tabName, id, true);
  }

//----------------------------------------------------------------------------

  int DbTab::CachingRowIterator::length() const
  {
    return idList.length();
  }

//----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByWhereClause(const QString& where, const QVariantList& args) const
  {
    QString sql = "SELECT id FROM " + tabName + " WHERE " + where;
    unique_ptr<QSqlQuery> qry = unique_ptr<QSqlQuery>(db->execContentQuery(sql, args));
    if (qry == NULL) {
      throw std::invalid_argument("getRowsByWhereClause: invalid query!");
    }
    
    DbTab::CachingRowIterator result = DbTab::CachingRowIterator(db, tabName, *qry);
    return result;
  }

//----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByColumnValue(const QVariantList& args) const
  {
    QVariantList qvl;
    
    qvl = prepWhereClause(args);  // don't catch exception, forward them to caller
    
    QString sql = "SELECT id FROM " + tabName + " WHERE " + qvl.at(0).toString();
    qvl.removeFirst();
    unique_ptr<QSqlQuery> qry = unique_ptr<QSqlQuery>(db->execContentQuery(sql, qvl));
    if (qry == NULL) {
      throw std::invalid_argument("getRowsByColumnValue: invalid query!");
    }
    
    DbTab::CachingRowIterator result = DbTab::CachingRowIterator(db, tabName, *qry);
    return result;
  }

//----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getRowsByColumnValue(const QString& col, const QVariant& val) const
  {
    QVariantList qvl;
    qvl << col << val;

    return getRowsByColumnValue(qvl);
  }

//----------------------------------------------------------------------------

  DbTab::CachingRowIterator DbTab::getAllRows() const
  {
    QString sql = "SELECT id FROM " + tabName;
    unique_ptr<QSqlQuery> qry = unique_ptr<QSqlQuery>(db->execContentQuery(sql));
    if (qry == NULL) {
      throw std::invalid_argument("getAllRows: invalid table, has no ID column!");
    }
    
    DbTab::CachingRowIterator result = DbTab::CachingRowIterator(db, tabName, *qry);
    return result;
  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByColumnValue(const QVariantList& args) const
  {
    DbTab::CachingRowIterator it = getRowsByColumnValue(args);
    if (!(it.isValid()))
    {
      throw std::invalid_argument("getSingleRowByColumnValue: no match!");
    }
    if (it.length() == 0)
    {
      throw std::invalid_argument("getSingleRowByColumnValue: no match!");
    }
    
    return (*it);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByColumnValue(const QString& col, const QVariant& val) const
  {
    QVariantList qvl;
    qvl << col << val;
    return getSingleRowByColumnValue(qvl);
  }

//----------------------------------------------------------------------------

  TabRow DbTab::getSingleRowByWhereClause(const QString& where, const QVariantList& args) const
  {
    DbTab::CachingRowIterator it = getRowsByWhereClause(where, args);
    if (!(it.isValid()))
    {
      throw std::invalid_argument("getSingleRowByWhereClause: no match!");
    }
    if (it.length() == 0)
    {
      throw std::invalid_argument("getSingleRowByWhereClause: no match!");
    }
    
    return (*it);
  }

//----------------------------------------------------------------------------

  int DbTab::deleteRowsByWhereClause(const QString& where, const QVariantList& args) const
  {
    QString sql = "DELETE FROM " + tabName + " WHERE " + where;
    return db->execNonQuery(sql, args);
  }

//----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const QVariantList& args) const
  {
    QVariantList qvl;
    
    qvl = prepWhereClause(args);  // don't catch exception, forward them to caller
    
    QString sql = "DELETE FROM " + tabName + " WHERE " + qvl.at(0).toString();
    qvl.removeFirst();
    return db->execNonQuery(sql, qvl);
  }

//----------------------------------------------------------------------------

  int DbTab::deleteRowsByColumnValue(const QString& col, const QVariant& val) const
  {
    QVariantList qvl;
    qvl << col << val;
    return deleteRowsByColumnValue(qvl);
  }

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

}
