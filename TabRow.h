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

#ifndef TABROW_H
#define	TABROW_H

#include "SqliteDatabase.h"
#include "CommonTabularClass.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"

namespace SqliteOverlay
{

  /**
   * A class representing a single data row in a table (not in a view!)
   * 
   * @author volker
   */
  class TabRow
  {
  public:
    TabRow (SqliteDatabase* db, const string& _tabName, int _rowId, bool skipCheck = false);
    TabRow (SqliteDatabase* db, const string& _tabName, const WhereClause& where);
    virtual ~TabRow ();
    int getId() const;

    bool update(const ColumnValueClause& cvc) const;
    bool update(const string& colName, const int newVal) const;
    bool update(const string& colName, const double newVal) const;
    bool update(const string& colName, const string newVal) const;

    string operator[](const string& colName) const;
    int getInt(const string& colName) const;
    double getDouble(const string& colName) const;
    LocalTimestamp getLocalTime(const string& colName) const;
    UTCTimestamp getUTCTime(const string& colName) const;

    unique_ptr<ScalarQueryResult<int>> getInt2(const string& colName) const;
    unique_ptr<ScalarQueryResult<double>> getDouble2(const string& colName) const;
    unique_ptr<ScalarQueryResult<string>> getString2(const string& colName) const;
    unique_ptr<ScalarQueryResult<LocalTimestamp>> getLocalTime2(const string& colName) const;
    unique_ptr<ScalarQueryResult<UTCTimestamp>> getUTCTime2(const string& colName) const;

    inline bool operator== (const TabRow& other) const
    {
      return ((other.tabName == tabName) && (other.rowId == rowId) && (other.db == db));  // enforce the same db handle!
    }
    inline bool operator!= (const TabRow& other) const
    {
      return (!(this->operator == (other)));
    }

    SqliteDatabase* getDb();
    bool erase();
    
  protected:
    /**
     * the handle to the (parent) database
     */
    SqliteDatabase* db;
    
    /**
     * the name of the associated table
     */
    string tabName;
    
    /**
     * the unique index of the data row
     */
    int rowId;

    string cachedWhereStatementForRow;
    
    bool doInit(const WhereClause& where);

  private:

  };
}

#endif	/* TABROW_H */

