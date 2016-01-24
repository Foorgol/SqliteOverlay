
#include "StringHelper.h"

namespace SqliteOverlay {

  StringList&splitString(const string& s, char delim, StringList& elems)
  {
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
    {
      elems.push_back(item);
    }

    return elems;
  }

  //----------------------------------------------------------------------------

  StringList splitString(const string& s, char delim)
  {
    StringList elems;
    splitString(s, delim, elems);
    return elems;
  }

  //----------------------------------------------------------------------------

  bool replaceString_First(string& src, const string& key, const string& value)
  {
    if (src.empty()) return false;
    if (key.empty()) return false;

    // find first occurence of "key"
    size_t startPos = src.find(key);
    if (startPos == string::npos) return false;

    // replace this first occurence with the new value
    src.replace(startPos, key.length(), value);
    return true;
  }

  //----------------------------------------------------------------------------

  int replaceString_All(string& src, const string& key, const string& value)
  {
    while (replaceString_First(src, key, value));
  }

  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------

}

