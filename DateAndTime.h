#ifndef DATEANDTIME_H
#define DATEANDTIME_H

#include <string>
#include <memory>
#include <ctime>
#include <cstring>

#include <sqlite3.h>

#include "Logger.h"

using namespace std;

namespace SqliteOverlay
{  
  static constexpr int MIN_YEAR = 1900;
  static constexpr int MAX_YEAR = 2100;  // arbitrarily chosen by me

  // a wrapper class for tm for storing timestamps
  class CommonTimestamp
  {
  public:
    CommonTimestamp(int year, int month, int day, int hour, int min, int sec);
    time_t getRawTime() const;

    string getISODate() const;
    string getTime() const;
    string getTimestamp() const;

    static bool isValidDate(int year, int month, int day);
    static bool isLeapYear(int year);

    inline bool operator< (const CommonTimestamp& other) const
    {
      return (raw < other.raw);   // maybe I should use difftime() here...
    }
    inline bool operator> (const CommonTimestamp& other) const
    {
      return (other < (*this));
    }
    inline bool operator<= (const CommonTimestamp& other) const
    {
      return (!(other < *this));
    }
    inline bool operator>= (const CommonTimestamp& other) const
    {
      return (!(*this < other));
    }
    inline bool operator== (const CommonTimestamp& other) const
    {
      return (raw == other.raw);   // maybe I should use difftime() here...
    }
    inline bool operator!= (const CommonTimestamp& other) const
    {
      return (raw != other.raw);   // maybe I should use difftime() here...
    }

  protected:
    tm timestamp;
    time_t raw;
    string getFormattedString(const string& fmt) const;
  };

  // an extension of struct tm to clearly indicate that local time
  // is stored
  class UTCTimestamp;
  class LocalTimestamp : public CommonTimestamp
  {
  public:
    static constexpr int DST_AS_RIGHT_NOW = 4242;
    static constexpr int DST_GUESSED = 8888;
    LocalTimestamp(int year, int month, int day, int hour, int min, int sec, int dstHours = DST_GUESSED);
    LocalTimestamp(time_t rawTimeInUTC);
    LocalTimestamp();
    UTCTimestamp toUTC() const;
  };

  // an extension of struct tm to clearly indicate that UTC
  // is stored
  class UTCTimestamp : public CommonTimestamp
  {
  public:
    UTCTimestamp(int year, int month, int day, int hour, int min, int sec);
    UTCTimestamp(time_t rawTimeInUTC);
    UTCTimestamp();
    LocalTimestamp toLocalTime() const;
  };

}

#endif /* DATEANDTIME_H */
