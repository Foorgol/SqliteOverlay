#ifndef STRING_HELPER_FUNCS_H
#define STRING_HELPER_FUNCS_H

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

namespace SqliteOverlay
{
  typedef vector<string> StringList;

  // trim from start
  inline std::string &string_ltrim(std::string &s) {
          s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
          return s;
  }

  // trim from end
  inline std::string &string_rtrim(std::string &s) {
          s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
          return s;
  }

  // trim from both ends
  inline std::string &string_trim(std::string &s) {
          return string_ltrim(string_rtrim(s));
  }

  // split string by delimiter character
  StringList& splitString(const string& s, char delim, StringList& elems);
  StringList splitString(const string& s, char delim);

  // replace substrings
  bool replaceString_First(string& src, const string& key, const string& value);
  int replaceString_All(string& src, const string& key, const string& value);

}
#endif
