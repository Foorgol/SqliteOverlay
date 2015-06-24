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
    void bindDouble(int argPos, double val);
    void bindString(int argPos, string& val);

    bool step(const Logger* log=nullptr);

    bool hasData() const;
    bool isDone() const;

    bool getInt(int colId, int* out) const;
    bool getDouble(int colId, double* out) const;
    bool getString(int colId, string* out) const;

    int getColType(int colId) const;
    int isNull(int colId) const;

  private:
    template<class T>
    bool getColumnValue_prep(int colId, T* out) const {
      if (out == nullptr) return false;
      if (!hasData()) return false;
      if (colId >= resultColCount) return false;
      return true;
    }

    SqlStatement(sqlite3* dbPtr, const string& sqlTxt);
    sqlite3_stmt* stmt;
    bool _hasData;
    bool _isDone;
    int resultColCount;
    int stepCount;
  };

  typedef unique_ptr<SqlStatement> upSqlStatement;
}
