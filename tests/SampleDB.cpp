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

#include <boost/date_time/local_time/local_time.hpp>

#include "SampleDB.h"
//#include "TableCreator.h"

void SampleDB::populateTables()
{
  /*TableCreator tc{this};
  tc.addInt("i");
  tc.addVarchar("s", 40);
  tc.addCol("f", "DOUBLE");
  tc.addCol("d", "DATETIME");
  tc.createTableAndResetCreator("t1");

  tc.addInt("i");
  tc.addVarchar("s", 40);
  tc.addCol("f", "DOUBLE");
  tc.addCol("d", "DATETIME");
  tc.createTableAndResetCreator("t2");*/
}

void SampleDB::populateViews()
{
  //viewCreationHelper("v1", "SELECT i,f,s FROM t1 WHERE i=84", nullptr);
}
