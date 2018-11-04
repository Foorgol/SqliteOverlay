
#include "Defs.h"

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

}
