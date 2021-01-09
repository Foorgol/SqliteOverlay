
#include <iosfwd>     // for std
#include <stdexcept>  // for invalid_argument

#include "Defs.h"

using namespace std;

namespace std
{
  using namespace SqliteOverlay;

  string to_string(ColumnDataType cdt)
  {
    switch (cdt) {
    case ColumnDataType::Blob:
      return "BLOB";

    case ColumnDataType::Null:
      return "";

    case ColumnDataType::Text:
      return "TEXT";

    case ColumnDataType::Float:
      return "REAL";

    case ColumnDataType::Integer:
      return "INTEGER";
    }

    return "";
  }

  //----------------------------------------------------------------------------

  std::string to_string(ConsistencyAction ca)
  {
    switch (ca)
    {
      case ConsistencyAction::NoAction:
        return "NO ACTION";
      case ConsistencyAction::SetNull:
        return "SET NULL";
      case ConsistencyAction::SetDefault:
        return "SET DEFAULT";
      case ConsistencyAction::Cascade:
        return "CASCADE";
      case ConsistencyAction::Restrict:
        return "RESTRICT";
      default:
        return "";
    }
    return "";
  }

  //----------------------------------------------------------------------------

  std::string to_string(ConflictClause cc)
  {
    switch (cc)
    {
    case ConflictClause::Abort:
      return "ABORT";

    case ConflictClause::Fail:
      return "FAIL";

    case ConflictClause::Ignore:
      return "IGNORE";

    case ConflictClause::Replace:
      return "REPLACE";

    case ConflictClause::Rollback:
      return "ROLLBACK";

    default:
      return "";
    }

    return "";  // includes NoAction
  }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace SqliteOverlay
{

  ColumnDataType int2ColumnDataType(int i)
  {
    if ((i < 1) || (i > 5))
    {
      throw std::invalid_argument("invalid parameter for int2ColumnDataType()");
    }

    return static_cast<ColumnDataType>(i);
  }

  //----------------------------------------------------------------------------


}
