/* 
 * File:   GenericDatabaseObject.cpp
 * Author: nyoknvk1
 * 
 * Created on 18. Februar 2014, 14:25
 */

#include "GenericDatabaseObject.h"

namespace SqliteOverlay
{

  /**
   * Constructor for a database object identified by table name and ID
   *
   * @param _tabName the name of the table the object refers to
   * @param _id the ID of the object in the table
   *
   */
GenericDatabaseObject::GenericDatabaseObject(SqliteDatabase* _db, const string& _tabName, int _id)
: db(_db), row(TabRow(_db, _tabName, _id))
{
}

//----------------------------------------------------------------------------

  GenericDatabaseObject::GenericDatabaseObject(SqliteDatabase* _db, const TabRow& _row)
  :db(_db), row(_row)
  {
  }


//----------------------------------------------------------------------------

  int GenericDatabaseObject::getId() const
  {
    return row.getId();
  }

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

    
//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

}
