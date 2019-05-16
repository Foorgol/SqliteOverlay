/* 
 * File:   GenericDatabaseObject.h
 * Author: nyoknvk1
 *
 * Created on 18. Februar 2014, 14:25
 */

#ifndef SQLITE_OVERLAY_GENERICDATABASEOBJECT_H
#define	SQLITE_OVERLAY_GENERICDATABASEOBJECT_H

#include <string>
#include "TabRow.h"
#include "SqliteDatabase.h"

namespace SqliteOverlay
{
  template<class DB_CLASS>
  class GenericDatabaseObject
  {
  public:
    using DatabaseClass = DB_CLASS;

    GenericDatabaseObject (const DB_CLASS& _db, const string& _tabName, int _id)
      : db(_db), row(TabRow(_db, _tabName, _id))
    {
      static_assert (std::is_base_of_v<SqliteDatabase, DB_CLASS>, "DB classes must be derived from SqliteDatabase");
    }

    GenericDatabaseObject (DB_CLASS& _db, const TabRow& _row)
      :db(_db), row(_row)
    {
      static_assert (std::is_base_of_v<SqliteDatabase, DB_CLASS>, "DB classes must be derived from SqliteDatabase");
    }

    inline int getId () const
    {
      return row.id();
    }

    inline bool operator== (const GenericDatabaseObject& other) const
    {
      return (other.row == row);
    }

    inline bool operator!= (const GenericDatabaseObject& other) const
    {
      return (!(this->operator == (other)));
    }

    const DatabaseClass& getDatabaseHandle() const
    {
      return db;
    }
    
  protected:
    reference_wrapper<const DB_CLASS> db;
    TabRow row;

  };
}

#endif	/* GENERICDATABASEOBJECT_H */

