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

  class GenericDatabaseObject
  {
  public:
    GenericDatabaseObject (SqliteDatabase* _db, const string& _tabName, int _id);
    GenericDatabaseObject (SqliteDatabase* _db, const TabRow& _row);
    int getId () const;

    inline bool operator== (const GenericDatabaseObject& other) const
    {
      return (other.row == row);
    }

    inline bool operator!= (const GenericDatabaseObject& other) const
    {
      return (!(this->operator == (other)));
    }
    
  protected:
    SqliteDatabase* db;
    TabRow row;

  };
}

#endif	/* GENERICDATABASEOBJECT_H */

