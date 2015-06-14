#include <string>
#include <memory>

#include <sqlite3.h>

#include "Logger.h"

using namespace std;

namespace SqliteOverlay
{
  class SqlStatement
  {
  public:
    static unique_ptr<SqlStatement> get(sqlite3* dbPtr, const string& sqlTxt, const Logger* log=nullptr);
    ~SqlStatement();

    void bindInt(int argPos, int val);
    //void bindInt(string argName, int val);

    bool step(const Logger* log=nullptr);

    bool hasData() const;
    bool isDone() const;

    bool getInt(int colId, int* out) const;

  private:
    SqlStatement(sqlite3* dbPtr, const string& sqlTxt);
    sqlite3_stmt* stmt;
    bool _hasData;
    bool _isDone;
    int resultColCount;
    int stepCount;
  };

  typedef unique_ptr<SqlStatement> upSqlStatement;
}
