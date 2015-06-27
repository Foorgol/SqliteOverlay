#ifndef DATEANDTIME_H
#define DATEANDTIME_H

#include <string>
#include <memory>
#include <ctime>

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
    virtual time_t getRawTime() const = 0;

    string getISODate() const;
    string getTime() const;
    string getTimestamp() const;

    static bool isValidDate(int year, int month, int day);
    static bool isLeapYear(int year);

  private:
    tm timestamp;
    string getFormattedString(const string& fmt) const;
  };

  // an extension of struct tm to clearly indicate that local time
  // is stored
  class LocalTimestamp : public CommonTimestamp
  {
  public:
    LocalTimestamp(int year, int month, int day, int hour, int min, int sec);
    LocalTimestamp(time_t rawTimeInUTC);
    LocalTimestamp();
    virtual time_t getRawTime() const override;

  private:
    tm timestamp;
  };

  // an extension of struct tm to clearly indicate that UTC
  // is stored
  class UTCTimestamp : public CommonTimestamp
  {
  public:
    UTCTimestamp(int year, int month, int day, int hour, int min, int sec);
    UTCTimestamp(time_t rawTimeInUTC);
    UTCTimestamp();
    virtual time_t getRawTime() const override;

  private:
    tm timestamp;
  };

}

#endif /* DATEANDTIME_H */
