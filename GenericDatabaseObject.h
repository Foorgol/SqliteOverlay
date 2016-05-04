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

  template <class DB_CLASS_TYPE>
  class GenericDatabaseObject
  {
  public:
    GenericDatabaseObject (DB_CLASS_TYPE* _db, const string& _tabName, int _id)
      : db(_db), row(TabRow(_db, _tabName, _id)) {}

    GenericDatabaseObject (DB_CLASS_TYPE* _db, const TabRow& _row)
      :db(_db), row(_row) {}

    inline int getId () const
    {
      return row.getId();
    }

    inline bool operator== (const GenericDatabaseObject<DB_CLASS_TYPE>& other) const
    {
      return (other.row == row);
    }

    inline bool operator!= (const GenericDatabaseObject<DB_CLASS_TYPE>& other) const
    {
      return (!(this->operator == (other)));
    }

    inline DB_CLASS_TYPE* getDatabaseHandle() const
    {
      return db;
    }
    
  protected:
    DB_CLASS_TYPE* db;
    TabRow row;

  };
}

#endif	/* GENERICDATABASEOBJECT_H */

