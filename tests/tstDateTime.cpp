#include <string>
#include <iostream>

#include <boost/date_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include <gtest/gtest.h>

#include "DateAndTime.h"

using namespace std;
using namespace SqliteOverlay;

TEST(CommonTimestamp, ValidDate)
{
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 0, 10));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 10, 0));
  ASSERT_FALSE(CommonTimestamp::isValidDate(1900, 2, 29));
  ASSERT_TRUE(CommonTimestamp::isValidDate(2000, 2, 29));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 2, 30));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 4, 31));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 6, 31));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 9, 31));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 11, 31));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 1, 32));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 3, 32));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 5, 32));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 7, 32));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 8, 32));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 10, 32));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2000, 12, 32));
  ASSERT_TRUE(CommonTimestamp::isValidDate(2000, 3, 26));
}

//----------------------------------------------------------------------------

TEST(CommonTimestamp, Comparison)
{
  using namespace boost::local_time;
  time_zone_ptr testZone{new posix_time_zone("TST-01:00:00")};

  LocalTimestamp t1(2000, 01, 01, 0, 0, 9, testZone);
  LocalTimestamp t2(2000, 01, 01, 0, 0, 10, testZone);
  LocalTimestamp t2a(2000, 01, 01, 0, 0, 10, testZone);

  // less than
  ASSERT_TRUE(t1 < t2);
  ASSERT_FALSE(t2 < t1);
  ASSERT_FALSE(t2 < t2a);
  ASSERT_FALSE(t2 < t2);

  // greater than
  ASSERT_TRUE(t2 > t1);
  ASSERT_FALSE(t1 > t2);
  ASSERT_FALSE(t2 > t2a);
  ASSERT_FALSE(t2 > t2);

  // less or equal
  ASSERT_TRUE(t1 <= t2);
  ASSERT_FALSE(t2 <= t1);
  ASSERT_TRUE(t2 <= t2a);
  ASSERT_TRUE(t2 <= t2);

  // greater or equal
  ASSERT_TRUE(t2 >= t1);
  ASSERT_FALSE(t1 >= t2);
  ASSERT_TRUE(t2 >= t2a);
  ASSERT_TRUE(t2 >= t2);

  // equal
  ASSERT_FALSE(t2 == t1);
  ASSERT_FALSE(t1 == t2);
  ASSERT_TRUE(t2 == t2a);
  ASSERT_TRUE(t2 == t2);

  // not equal
  ASSERT_TRUE(t2 != t1);
  ASSERT_TRUE(t1 != t2);
  ASSERT_FALSE(t2 != t2a);
  ASSERT_FALSE(t2 != t2);
}

//----------------------------------------------------------------------------

TEST(CommonTimestamp, BoostTime)
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  // make sure that boost's ptime uses UTC by default
  ptime ptUtc(date(1970, 1, 1));
  ASSERT_TRUE(to_time_t(ptUtc) == 0);

  // create a dummy time zone (UTC+2, no DST)
  typedef boost::date_time::local_adjustor<ptime, 2, no_dst> dummyAdjustor;

  // convert the ptUtc to local time
  ptime ptLocal = dummyAdjustor::utc_to_local(ptUtc);
  time_duration td = ptLocal.time_of_day();
  ASSERT_TRUE(td.hours() == 2);
  cout << ptLocal << endl;
}
