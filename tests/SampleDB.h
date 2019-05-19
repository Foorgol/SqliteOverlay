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

#ifndef SAMPLEDB_H
#define	SAMPLEDB_H

#include "SqliteDatabase.h"

using namespace SqliteOverlay;

class SampleDB : public SqliteDatabase
{
public:
  virtual void populateTables();
  virtual void populateViews();
  
  SampleDB() : SqliteDatabase() {};
  SampleDB(std::string sqliteFilename, SqliteOverlay::OpenMode om)
    : SqliteDatabase(sqliteFilename, om) {};
  
private:
};

#endif	/* SAMPLEDB_H */

