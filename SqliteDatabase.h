#ifndef SQLITEDATABASE_H
#define	SQLITEDATABASE_H

#include <sqlite3.h>

#include <string>
#include <memory>

#include "Logger.h"

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
    bool isAlive() const;

  protected:
    /**
     * @brief dbPtr the internal database handle
     */
    unique_ptr<sqlite3, SqliteDeleter> dbPtr;

    /**
     * A counter for executed queries; for debugging purposes only
     */
    long queryCounter;

    /**
     * A logger instance for debug messages
     */
    unique_ptr<Logger> log;
  };
}

#endif  /* SQLITEDATABASE_H */
