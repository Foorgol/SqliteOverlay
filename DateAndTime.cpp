#include <stdexcept>
#include <ctime>
#include <cstring>
#include <memory>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "DateAndTime.h"

using namespace std;

// a special hack to substitute the missing timegm() call
// under windows
//
// See the CMakeList.txt file for the definition of
// IS_WINDOWS_BUILD
#ifdef IS_WINDOWS_BUILD
  #define timegm _mkgmtime
#endif

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

  int CommonTimestamp::getDoW() const
  {
    return timestamp.tm_wday;
  }

  //----------------------------------------------------------------------------

  int CommonTimestamp::getYMD() const
  {
    return (timestamp.tm_year + MIN_YEAR) * 10000 + (timestamp.tm_mon+1) * 100 + timestamp.tm_mday;
  }

  //----------------------------------------------------------------------------

  bool CommonTimestamp::setTime(int hour, int min, int sec)
  {
    if (!(isValidTime(hour, min, sec))) return false;

    timestamp.tm_hour = hour;
    timestamp.tm_min = min;
    timestamp.tm_sec = sec;
    return true;
  }

  //----------------------------------------------------------------------------

  tuple<int, int, int> CommonTimestamp::getYearMonthDay() const
  {
    return make_tuple(timestamp.tm_year + MIN_YEAR, timestamp.tm_mon + 1, timestamp.tm_mday);
  }

  //----------------------------------------------------------------------------

  bool CommonTimestamp::isValidDate(int year, int month, int day)
  {
    boost::gregorian::date d{year, month, day};
    return (!(d.is_special()));
  }

  //----------------------------------------------------------------------------

  bool CommonTimestamp::isValidTime(int hour, int min, int sec)
  {
    return ((hour >= 0) && (hour < 24) && (min >=0) && (min < 60) && (sec >= 0) && (sec < 60));
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
    if (!isValidDate(year, month, day))
    {
      throw std::invalid_argument("Invalid date values!");
    }
    if (!isValidTime(hour, min, sec))
    {
      throw std::invalid_argument("Invalid time values!");
    }

    // call timegm() once to adjust all other field in the timestamp struct
    // and to get the time_t value
    raw = timegm(&timestamp);
  }

  //----------------------------------------------------------------------------

  UTCTimestamp::UTCTimestamp(int ymd, int hour, int min, int sec)
    : UTCTimestamp(ymd / 10000, (ymd % 10000) / 100, ymd % 100, hour, min, sec)
  {
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

  LocalTimestamp::LocalTimestamp(int year, int month, int day, int hour, int min, int sec, boost::local_time::time_zone_ptr tzp)
    : CommonTimestamp(year, month, day, hour, min, sec)
  {
    using namespace boost::gregorian;
    using namespace boost::posix_time;
    using namespace boost::local_time;

    // try to construct a date object
    date d;
    try
    {
      d = date{year, month, day};
    }
    catch (exception e)
    {
      throw std::invalid_argument("Invalid date values!");
    }

    // try to construct a time duration
    time_duration td{0,0,0, 0};
    try
    {
      td = time_duration{hour, min, sec, 0};
    }
    catch (exception e)
    {
      throw std::invalid_argument("Invalid time values!");
    }

    // make sure the time zone pointer is valid
    if (tzp == nullptr)
    {
      throw std::invalid_argument("Time zone pointer is empty");
    }

    // construct a local_date_time object
    local_date_time ldt{d, td, tzp, local_date_time::NOT_DATE_TIME_ON_ERROR};
    if (ldt.is_not_a_date_time())
    {
      throw std::invalid_argument("Local time is invalid or ambiguous");
    }

    // convert to UTC and store the result
    raw = to_time_t(ldt.utc_time());
    timestamp = to_tm(ldt);
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

  unique_ptr<LocalTimestamp> LocalTimestamp::fromISODate(const string& isoDate, boost::local_time::time_zone_ptr tzp, int hour, int min, int sec)
  {
    //
    // split the string into its components
    //

    // find the first "-"
    size_t posFirstDash = isoDate.find('-');
    if (posFirstDash != 4)    // the first dash must always be at position 4 (4-digit year!)
    {
      return nullptr;
    }
    string sYear = isoDate.substr(0, 4);

    // find the second "-"
    size_t posSecondDash = isoDate.find('-', 5);
    if (posSecondDash == string::npos)
    {
      return nullptr;
    }
    string sMonth = isoDate.substr(5, posSecondDash - 5 + 1);

    // get the day
    if (posSecondDash >= (isoDate.length() - 1))    // the dash is the last character ==> no day string
    {
      return nullptr;
    }
    string sDay = isoDate.substr(posSecondDash+1, string::npos);

    // try to convert the string into ints
    int year;
    int month;
    int day;
    try
    {
      year = stoi(sYear);
      month = stoi(sMonth);
      day = stoi(sDay);
    } catch (exception e) {
      return nullptr;
    }

    // try to construct a new LocalTimestamp from these ints
    LocalTimestamp* result;
    try
    {
      result = new LocalTimestamp(year, month, day, hour, min, sec, tzp);
    } catch (exception e) {
      return nullptr;   // invalid parameters
    }

    // return the result
    return upLocalTimestamp(result);
  }

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  TimePeriod::TimePeriod(const UTCTimestamp &_start)
    :start(_start), isOpenEnd(true)
  {

  }

  //----------------------------------------------------------------------------

  TimePeriod::TimePeriod(const UTCTimestamp &_start, const UTCTimestamp &_end)
    :start(_start), end(_end), isOpenEnd(false)
  {
    if (end < start)
    {
      throw invalid_argument("TimePeriod ctor: 'end'' may not be before start!");
    }
  }

  //----------------------------------------------------------------------------

  bool TimePeriod::hasOpenEnd() const
  {
    return isOpenEnd;
  }

  //----------------------------------------------------------------------------

  bool TimePeriod::isInPeriod(const UTCTimestamp &ts) const
  {
    return (determineRelationToPeriod(ts) == IS_IN_PERIOD);
  }

  //----------------------------------------------------------------------------

  int TimePeriod::determineRelationToPeriod(const UTCTimestamp &ts) const
  {
    if (ts < start) return IS_BEFORE_PERIOD;
    if (isOpenEnd) return IS_IN_PERIOD;

    if (ts > end) return IS_AFTER_PERIOD;
    return IS_IN_PERIOD;
  }

  //----------------------------------------------------------------------------

  long TimePeriod::getLength_Sec() const
  {
    if (isOpenEnd) return -1;

    return end.getRawTime() - start.getRawTime();
  }

  //----------------------------------------------------------------------------

  double TimePeriod::getLength_Minutes() const
  {
    long secs = getLength_Sec();

    return (secs < 0 ? -1 : secs / 60.0);
  }

  //----------------------------------------------------------------------------

  double TimePeriod::getLength_Hours() const
  {
    long secs = getLength_Sec();

    return (secs < 0 ? -1 : secs / (3600.0));
  }

  //----------------------------------------------------------------------------

  double TimePeriod::getLength_Days() const
  {
    long secs = getLength_Sec();

    return (secs < 0 ? -1 : secs / (3600.0 * 24.0));
  }

  //----------------------------------------------------------------------------

  double TimePeriod::getLength_Weeks() const
  {
    long secs = getLength_Sec();

    return (secs < 0 ? -1 : secs / (3600.0 * 24.0 * 7));
  }

  //----------------------------------------------------------------------------

  bool TimePeriod::setStart(const UTCTimestamp &_start)
  {
    if (_start > end) return false;

    start = _start;
    return true;
  }

  //----------------------------------------------------------------------------

  bool TimePeriod::setEnd(const UTCTimestamp &_end)
  {
    if (_end < start) return false;

    end = _end;
    isOpenEnd = false;

    return true;
  }

  //----------------------------------------------------------------------------

  bool TimePeriod::applyOffsetToStart(long secs)
  {
    UTCTimestamp newStart{start.getRawTime() + secs};

    if (newStart > end) return false;

    start = newStart;
    return true;
  }

  //----------------------------------------------------------------------------

  bool TimePeriod::applyOffsetToEnd(long secs)
  {
    if (isOpenEnd) return false;

    UTCTimestamp newEnd{end.getRawTime() + secs};

    if (newEnd < start) return false;

    end = newEnd;
    return true;
  }

  //----------------------------------------------------------------------------

  UTCTimestamp TimePeriod::getStartTime() const
  {
    return start;
  }

  //----------------------------------------------------------------------------

  upUTCTimestamp TimePeriod::getEndTime() const
  {
    if (isOpenEnd) return nullptr;

    return make_unique<UTCTimestamp>(end);
  }

  //----------------------------------------------------------------------------

  tuple<int, int, int> YearMonthDayFromInt(int ymd)
  {
    int year = ymd / 10000;
    int month = (ymd % 10000) / 100;
    int day = ymd % 100;

    return make_tuple(year, month, day);
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

}
