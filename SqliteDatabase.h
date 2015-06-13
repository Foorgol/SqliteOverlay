#ifndef SQLITEDATABASE_H
#define	SQLITEDATABASE_H

#include <sqlite3.h>

#include <string>
#include <memory>


using namespace std;

namespace SqliteOverlay
{
  struct SqliteDeleter
  {
    void operator()(sqlite3* p);
  };

  class SqliteDatabase
  {
  public:
    SqliteDatabase(string& dbFileName, bool createNew=false);

  private:
    unique_ptr<sqlite3, SqliteDeleter> dbPtr;
  };
}

#endif  /* SQLITEDATABASE_H */
