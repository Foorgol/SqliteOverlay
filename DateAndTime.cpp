#include <stdexcept>
#include <ctime>
#include <cstring>

#include "DateAndTime.h"

using namespace std;

namespace SqliteOverlay
{
  CommonTimestamp::CommonTimestamp(int year, int month, int day, int hour, int min, int sec)
  {
    timestamp.tm_year = year - 1900;
    timestamp.tm_mon = month - 1;
    timestamp.tm_mday = day;
    timestamp.tm_hour = hour;
    timestamp.tm_min = min;
    timestamp.tm_sec = sec;

    // this value has to be set by derived classes like
    // UTCTimestamp or LocalTimestamp
    raw = -1;
  }

  //----------------------------------------------------------------------------

  time_t CommonTimestamp::getRawTime() const
  {
    return raw;
  }

  //----------------------------------------------------------------------------

  string CommonTimestamp::getISODate() const
  {
    return getFormattedString("%Y-%m-%d");
  }

  //----------------------------------------------------------------------------

  string CommonTimestamp::getTime() const
  {
    return getFormattedString("%H:%M:%S");
  }

  //----------------------------------------------------------------------------

  string CommonTimestamp::getTimestamp() const
  {
    return getFormattedString("%Y-%m-%d %H:%M:%S");
  }

  //----------------------------------------------------------------------------

  bool CommonTimestamp::isValidDate(int year, int month, int day)
  {
    // check lower boundaries
    if ((year < MIN_YEAR) || (month < 1) || (day < 1))
    {
      return false;
    }

    // check max boundaries
    if ((year > MAX_YEAR) || (month > 12) || (day > 31))
    {
      return false;
    }

    // check month / days combinations
    int maxFebDays = (isLeapYear(year)) ? 29 : 28;
    if ((month == 2) && (day > maxFebDays))
    {
      return false;
    }
    if ((month == 4) || (month == 6) || (month == 9) || (month == 11))
    {
      if (day > 30)
      {
        return false;
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------

  bool CommonTimestamp::isLeapYear(int year)
  {
    if ((year % 4) != 0)
    {
      return false;
    }

    if ((year % 100) != 0)
    {
      return true;
    }

    if ((year % 400) == 0)
    {
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------

  string CommonTimestamp::getFormattedString(const string& fmt) const
  {
    char buf[80];
    strftime(buf, 80, fmt.c_str(), &timestamp);
    string result = string(buf);

    return result;
  }

  //----------------------------------------------------------------------------

  UTCTimestamp::UTCTimestamp(int year, int month, int day, int hour, int min, int sec)
    : CommonTimestamp(year, month, day, hour, min, sec)
  {
    // call timegm() once to adjust all other field in the timestamp struct
    // and to get the time_t value
    raw = timegm(&timestamp);
  }

  //----------------------------------------------------------------------------

  UTCTimestamp::UTCTimestamp(time_t rawTimeInUTC)
    :CommonTimestamp(0,0,0,0,0,0)  // dummy values, will be overwritten anyway
  {
    // convert raw time to tm
    tm* tmp = gmtime(&rawTimeInUTC);

    // copy the contents over to our local struct
    memcpy(&timestamp, tmp, sizeof(tm));

    // call timegm() once to adjust all other field in the timestamp struct
    timegm(&timestamp);

    // store the provided raw value
    raw = rawTimeInUTC;
  }

  //----------------------------------------------------------------------------

  UTCTimestamp::UTCTimestamp()
    :UTCTimestamp(time(0))
  {

  }

  //----------------------------------------------------------------------------

  LocalTimestamp UTCTimestamp::toLocalTime() const
  {
    return LocalTimestamp(raw);
  }

  //----------------------------------------------------------------------------

  LocalTimestamp::LocalTimestamp(int year, int month, int day, int hour, int min, int sec, int dstHours)
    : CommonTimestamp(year, month, day, hour, min, sec)
  {
    if (dstHours == DST_AS_RIGHT_NOW)
    {
      // retriee a tm of the current time
      // and read the dstHours from it
      time_t now = time(0);
      struct tm* locTime = localtime(&now);
      dstHours = locTime->tm_isdst;
    }

    if (dstHours == DST_GUESSED)
    {
      // a little helper function to (re-)set the values in timestamp
      auto resetTimestamp = [&]() {
        timestamp.tm_year = year - 1900;
        timestamp.tm_mon = month - 1;
        timestamp.tm_mday = day;
        timestamp.tm_hour = hour;
      };

      dstHours = 0;
      timestamp.tm_isdst = 0;

      // call mktime() to adjust timezone settings etc.
      mktime(&timestamp);

      // maybe mktime has modified the values of tm_hour (and in corner cases
      // of day, month or year) because we provided wrong DST settings
      //
      // In this case, it's most likely sufficient to reset the values
      // for date and time and call mktime() again, because the previous
      // call to mktime() has applied the right DST and timezone settings
      // to "timestamp".
      resetTimestamp();
      mktime(&timestamp);
      dstHours = timestamp.tm_isdst;

      // Maybe we still have a mismatch. In this case, we need to
      // determine the correct DST value.
      // But instead of calculating the correct DST (which is a nightmare if
      // year, month and date also changed), we use brute force and try
      // different DST settings until we get the intended result
      if (timestamp.tm_hour != hour)
      {
        int guessedDST = 1;
        while (guessedDST != 12)
        {
          dstHours = guessedDST;   // try positive DST
          timestamp.tm_isdst = dstHours;
          resetTimestamp();
          mktime(&timestamp);
          if (timestamp.tm_hour == hour)
          {
            break;
          }

          dstHours = -guessedDST;   // try negative DST
          timestamp.tm_isdst = dstHours;
          resetTimestamp();
          mktime(&timestamp);
          if (timestamp.tm_hour == hour)
          {
            break;
          }

          ++guessedDST;
        }

        if (guessedDST == 12)
        {
          throw std::runtime_error("Couldn't guess DST correctly!");
        }
      }
    }


    // at this point, "dstHours" contains either...
    //   * the user-provided value;
    //   * the guessed DST value as per algorithm above; or
    //   * the current value of the local timezone


    // call mktime() once to adjust all other field in the timestamp struct
    // and to get the time_t value
    timestamp.tm_isdst = dstHours;
    raw = mktime(&timestamp);
  }

  //----------------------------------------------------------------------------

  LocalTimestamp::LocalTimestamp(time_t rawTimeInUTC)
    :CommonTimestamp(0,0,0,0,0,0)  // dummy values, will be overwritten anyway
  {
    // convert raw time to tm
    tm* tmp = localtime(&rawTimeInUTC);

    // copy the contents over to our local struct
    memcpy(&timestamp, tmp, sizeof(tm));

    // call mktime() once to adjust all other field in the timestamp struct
    mktime(&timestamp);

    // store the provided raw value
    raw = rawTimeInUTC;
  }

  //----------------------------------------------------------------------------

  LocalTimestamp::LocalTimestamp()
    :LocalTimestamp(time(0))
  {

  }

  //----------------------------------------------------------------------------

  UTCTimestamp LocalTimestamp::toUTC() const
  {
    return UTCTimestamp(raw);
  }

  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------

}
