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
#include <QString>
#include <QList>

void SampleDB::populateTables()
{
  QList<QString> col;
  col << "i INTEGER" << "f DOUBLE" << "s VARCHAR(40)" << "d DATETIME";
  tableCreationHelper("t1", col);
  tableCreationHelper("t2", col);
}

void SampleDB::populateViews()
{
  viewCreationHelper("v1", "SELECT i,f,s FROM t1 WHERE i=84");
}
