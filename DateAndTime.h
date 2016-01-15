#ifndef SQLITE_OVERLAY_DATEANDTIME_H
#define SQLITE_OVERLAY_DATEANDTIME_H

#include <string>
#include <memory>
#include <ctime>
#include <cstring>
#include <tuple>

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
    int getDoW() const;
    int getYMD() const;
    bool setTime(int hour, int min, int sec);
    tuple<int, int, int> getYearMonthDay() const;

    static bool isValidDate(int year, int month, int day);
    static bool isValidTime(int hour, int min, int sec);
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

    static unique_ptr<LocalTimestamp> fromISODate(const string& isoDate, int hour=12, int min=0, int sec=0, int dstHours = DST_GUESSED);
  };

  // an extension of struct tm to clearly indicate that UTC
  // is stored
  class UTCTimestamp : public CommonTimestamp
  {
  public:
    UTCTimestamp(int year, int month, int day, int hour, int min, int sec);
    UTCTimestamp(int ymd, int hour=12, int min=0, int sec=0);
    UTCTimestamp(time_t rawTimeInUTC);
    UTCTimestamp();
    LocalTimestamp toLocalTime() const;
  };

  typedef unique_ptr<LocalTimestamp> upLocalTimestamp;
  typedef unique_ptr<UTCTimestamp> upUTCTimestamp;

  // a time period
  class TimePeriod
  {
  public:
    static constexpr int IS_BEFORE_PERIOD = -1;
    static constexpr int IS_IN_PERIOD = 0;
    static constexpr int IS_AFTER_PERIOD = 1;

    TimePeriod(const UTCTimestamp& _start);
    TimePeriod(const UTCTimestamp& _start, const UTCTimestamp& _end);
    bool hasOpenEnd() const;
    bool isInPeriod(const UTCTimestamp& ts) const;
    int determineRelationToPeriod(const UTCTimestamp& ts) const;
    long getLength_Sec() const;
    double getLength_Minutes() const;
    double getLength_Hours() const;
    double getLength_Days() const;
    double getLength_Weeks() const;
    virtual bool setStart(const UTCTimestamp& _start);
    virtual bool setEnd(const UTCTimestamp& _end);

    virtual bool applyOffsetToStart(long secs);
    virtual bool applyOffsetToEnd(long secs);

    UTCTimestamp getStartTime() const;
    upUTCTimestamp getEndTime() const;

  protected:
    UTCTimestamp start;
    UTCTimestamp end;
    bool isOpenEnd;
  };

  tuple<int, int, int> YearMonthDayFromInt(int ymd);
}

#endif /* DATEANDTIME_H */
