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

#ifndef LOGGER_H
#define	LOGGER_H

#include <string>

using namespace std;

namespace SqliteOverlay
{

  class Logger
  {
  public:
    Logger (const string& senderName);
    Logger ();
    virtual ~Logger ();

    void warn (const string& msg);
    void critical (const string& msg);
    void info (const string& msg);
    void log(const int logLvl, const string& msg);
    void log(const string& msg);
    void setDefaultLevel(const int newDefaultLvl);
    void setLevel(const int newLvl);
    
    static const int LVL_CRITICAL = 2;
    static const int LVL_WARN = 1;
    static const int LVL_INFO = 0;
    
  private:

    string sender;
    int threshold;
    int defaultLevel;
    
    bool isValidLevel(const int lvl);
    
  };
}

#endif	/* LOGGER_H */

