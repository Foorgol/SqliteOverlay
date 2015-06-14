/*
 * Copyright © 2014 Volker Knollmann
 * 
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file or visit
 * http://www.wtfpl.net/ for more details.
 * 
 * This program comes without any warranty. Use it at your own risk or
 * don't use it at all.
 */

#include <stdexcept>
#include "HelperFunc.h"

namespace SqliteOverlay
{
  string commaSepStringFromStringList(const StringList& lst)
  {
    string result;
    
    for (size_t i=0; i<lst.size(); i++)
    {
      string v = lst.at(i);
      if (i > 0)
      {
        result += ", ";
      }
      result += v;
    }

    return result;
  }

//----------------------------------------------------------------------------
    

//----------------------------------------------------------------------------

  /**
   * Takes columnname / value pairs and constructs a WHERE clause using
   * placeholders ("?") and proper handling of NULL.
   * 
   * @param args a list of column / value pairs
   * 
   * @return a QVariantList with the WHERE-clause at index 0 and the parameters for the placeholders at index 1+
   */
//  QVariantList prepWhereClause(const QVariantList& args)
//  {
//    QString whereClause = "";

//    // check for an EVEN number of arguments
//    if ((args.length() % 2) != 0)
//    {
//      throw std::invalid_argument("Need an even number of arguments (column / value pairs)");
//    }

//    // for the return value, create a list of the parameter objects
//    // plus the WHERE-clause at index 0
//    QVariantList result;

//    for (int i=0; i < args.length(); i += 2)
//    {
//      whereClause += args[i].toString();
//      if (args[i+1].isNull()) whereClause += " IS NULL";
//      else
//      {
//        whereClause += " = ?";
//        result.append(args[i+1]);
//      }
//      if (i != (args.length() - 2)) whereClause += " AND ";
//    }

//    result.insert(0, whereClause);

//    return result;
//  }

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