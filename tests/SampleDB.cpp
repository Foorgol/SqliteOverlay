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

#include "SampleDB.h"
#include "HelperFunc.h"

void SampleDB::populateTables()
{
  StringList col;
  col.push_back("i INTEGER");
  col.push_back("f DOUBLE");
  col.push_back("s VARCHAR(40)");
  col.push_back("d DATETIME");
  tableCreationHelper("t1", col);
  tableCreationHelper("t2", col);
}

void SampleDB::populateViews()
{
  viewCreationHelper("v1", "SELECT i,f,s FROM t1 WHERE i=84");
}
