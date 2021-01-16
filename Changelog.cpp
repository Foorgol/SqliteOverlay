
#include "Changelog.h"

namespace SqliteOverlay
{

  void changeLogCallback(void* customPtr, int modType, const char* _dbName, const char* _tabName, sqlite3_int64 id)
  {
    if (customPtr == nullptr) return;
    ChangeLogCallbackContext* ctx = reinterpret_cast<ChangeLogCallbackContext*>(customPtr);

    const std::string dbName{_dbName};

    std::lock_guard<std::mutex> lg{*(ctx->logMutex)};

    ctx->logPtr->emplace_back(modType, dbName, std::string{_tabName}, id);
  }

  //----------------------------------------------------------------------------


}
