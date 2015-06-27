#include <gtest/gtest.h>

#include "DateAndTime.h"

using namespace SqliteOverlay;

TEST(Timestamps, testTimeConversion)
{
  // create a fake localtime object
  LocalTimestamp lt(2000, 1, 1, 12, 0, 0);

  // convert to raw time
  time_t raw1 = lt.getRawTime();

  // construct UTC from this raw time
  UTCTimestamp utc(raw1);

  // convert to raw again
  time_t raw2 = utc.getRawTime();

  // both raws should be identical
  ASSERT_EQ(raw1, raw2);

  // construct a new LocalTimestamp, this time from raw
  LocalTimestamp lt2(raw2);

  // conversion back to raw should give identical values
  ASSERT_EQ(raw2, lt2.getRawTime());
}

TEST(Timestamps, testGetters)
{
  // create a fake localtime object
  LocalTimestamp lt(2000, 1, 1, 4, 3, 2);

  ASSERT_EQ("2000-01-01", lt.getISODate());
  ASSERT_EQ("04:03:02", lt.getTime());
  ASSERT_EQ("2000-01-01 04:03:02", lt.getTimestamp());
}
