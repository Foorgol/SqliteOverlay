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

#include "GenericDatabase.h"

using namespace dbOverlay;

class SampleDB : public GenericDatabase
{
public:
  virtual void populateTables();
  virtual void populateViews();
  
  SampleDB(DB_ENGINE t, QString srv, int port, QString name, QString user, QString pw) :
  GenericDatabase(t, srv, port, name, user, pw) {};
  SampleDB() : GenericDatabase() {};
  SampleDB(QString sqliteFilename, bool createNew) : GenericDatabase(sqliteFilename, createNew) {};
  
private:
};

#endif	/* SAMPLEDB_H */

