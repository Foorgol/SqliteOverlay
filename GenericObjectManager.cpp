/* 
 * File:   GenericObjectManager.cpp
 * Author: volker
 * 
 * Created on March 2, 2014, 7:36 PM
 */


#include "GenericObjectManager.h"


namespace SqliteOverlay
{

  GenericObjectManager::GenericObjectManager(SqliteDatabase* _db, DbTab *_tab)
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

  GenericObjectManager::GenericObjectManager(SqliteDatabase *_db, const string &tabName)
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

  SqliteDatabase* GenericObjectManager::getDatabaseHandle()
  {
    return db;
  }

  //----------------------------------------------------------------------------

  int GenericObjectManager::getObjCount() const
  {
    return tab->length();
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
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------
    
}
