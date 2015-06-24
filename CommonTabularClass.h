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

#ifndef COMMONTABULARCLASS_H
#define	COMMONTABULARCLASS_H

#include <vector>

#include "SqliteDatabase.h"
#include "ClausesAndQueries.h"

namespace SqliteOverlay
{

  /**
   * A small struct with schema information about a column
   */
  class ColInfo
  {
  public:

    ColInfo (int _cid, const string& _name, const string& _type)
    : cid (_cid), name (_name), type (_type) { };

    int getId () const;
    string getColName () const;
    string getColType () const;
    
  protected:
    int cid;
    string name;
    string type;
    
  };
  
  typedef vector<ColInfo> ColInfoList;
  
  class CommonTabularClass
  {
  public:
    CommonTabularClass (SqliteDatabase* db, const string& tabName, bool _isView=false );
    virtual ~CommonTabularClass ();
    
    ColInfoList allColDefs() const;
    string getColType(const string& colName) const;
    string cid2name(int cid) const;
    int name2cid(const string& colName) const;
    bool hasColumn(const string& colName) const;
    bool hasColumn(int cid) const;
    int getMatchCountForWhereClause(const WhereClause& w) const;
    int getMatchCountForWhereClause(const string& where) const;
    int getMatchCountForColumnValue(const string& col, const string& val) const;
    int getMatchCountForColumnValue(const string& col, int val) const;
    int getMatchCountForColumnValue(const string& col, double val) const;
    int getMatchCountForColumnValueNull(const string& col) const;
    int length() const;

  protected:
    /**
     * the handle to the (parent) database
     */
    SqliteDatabase* db;
    
    /**
     * the name of the associated table or view
     */
    string tabName;
    
    /**
     * a tag whether we are a view or a tab
     */
    bool isView;

  private:

  };

}
#endif	/* COMMONTABULARCLASS_H */

