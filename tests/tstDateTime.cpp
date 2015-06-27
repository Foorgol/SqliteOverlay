#include <string>
#include <gtest/gtest.h>

#include "DateAndTime.h"

using namespace std;
using namespace SqliteOverlay;

TEST(CommonTimestamp, LeapYear)
{
  ASSERT_TRUE(CommonTimestamp::isLeapYear(4));
  ASSERT_TRUE(CommonTimestamp::isLeapYear(8));
  ASSERT_TRUE(CommonTimestamp::isLeapYear(2000));
  ASSERT_FALSE(CommonTimestamp::isLeapYear(1));
  ASSERT_FALSE(CommonTimestamp::isLeapYear(3));
  ASSERT_FALSE(CommonTimestamp::isLeapYear(100));
  ASSERT_FALSE(CommonTimestamp::isLeapYear(200));
}

//----------------------------------------------------------------------------

TEST(CommonTimestamp, ValidDate)
{
  ASSERT_FALSE(CommonTimestamp::isValidDate(1899, 10, 10));
  ASSERT_FALSE(CommonTimestamp::isValidDate(2101, 10, 10));
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
