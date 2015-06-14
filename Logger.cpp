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

#include <iostream>
#include "Logger.h"

namespace SqliteOverlay
{

    Logger::Logger(const string& senderName)  : sender(senderName), threshold(LVL_INFO), defaultLevel(LVL_CRITICAL) {
        info("####### '" + sender + "' starts logging #####");
    }

    Logger::~Logger()
    {
        info("####### '" + sender + "' stops logging #####");
    }
    
    Logger::Logger()
    {
      sender = "<<invalid>>";
    }
    
    void Logger::warn (const string& msg)
    {
        log(LVL_WARN, msg);
    }
    
    void Logger::critical (const string& msg)
    {
        log(LVL_CRITICAL, msg);
    }
    
    void Logger::info (const string& msg)
    {
        log(LVL_INFO, msg);
    }
    
    void Logger::log(const int logLvl, const string& msg)
    {
        if (logLvl < threshold) return;
        
        string txt;
        if (logLvl == LVL_CRITICAL) txt = "CRITICAL";
        else if (logLvl == LVL_WARN) txt = "Warning";
        else txt = "Info";
        
        txt += " - " + sender + ": " + msg;
        
        clog << txt << endl;
    }
    
    void Logger::log(const string& msg)
    {
        log(defaultLevel, msg);
    }
    
    void Logger::setDefaultLevel(const int newDefaultLvl)
    {
        if (!(isValidLevel(newDefaultLvl)))
        {
            return;
        }
        
        defaultLevel = newDefaultLvl;
    }
    
    void Logger::setLevel(const int newLvl)
    {
        if (!(isValidLevel(newLvl)))
        {
            return;
        }
        
        threshold = newLvl;
    }
    
    bool Logger::isValidLevel(const int lvl) {
        return ((lvl >= LVL_WARN) && (lvl <= LVL_CRITICAL));
    }


}
