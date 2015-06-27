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

//----------------------------------------------------------------------------

TEST(Timestamps, testEpoch)
{
  // THIS test only works on a computer with
  // CET / CEST timezone settings!

  // create a fake localtime object (CEST)
  LocalTimestamp lt(2015, 6, 27, 12, 0, 0, 1);

  // 2015-06-27 is in summer time, to CEST applies
  //
  // CEST is 2 hours ahead of UTC / GMT, so the
  // equivalent UTC time is 2015-06-27, 10:00:00
  UTCTimestamp utc(2015, 6, 27, 10, 0, 0);

  // the epoch value for this UTC date is, according
  // to an internet converter:
  time_t expectedEpochVal = 1435399200;

  // convert local time to raw
  time_t raw1 = lt.getRawTime();
  ASSERT_EQ(expectedEpochVal, raw1);

  // convert UTC time to raw
  time_t raw2 = utc.getRawTime();
  ASSERT_EQ(expectedEpochVal, raw2);

  // create a timestamp from the epoch value
  lt = LocalTimestamp(expectedEpochVal);
  ASSERT_EQ("2015-06-27 12:00:00", lt.getTimestamp());

  // repeat the test with a timestamp
  // in CET (winter time)

  // create a fake localtime object (CET)
  lt = LocalTimestamp(2015, 1, 27, 11, 0, 0, 0);

  // 2015-01-27 is in winter time, to CET applies
  //
  // CET is 1 hour ahead of UTC / GMT, so the
  // equivalent UTC time is 2015-01-27, 10:00:00
  utc = UTCTimestamp(2015, 1, 27, 10, 0, 0);

  // the epoch value for this UTC date is, according
  // to an internet converter:
  expectedEpochVal = 1422352800;

  // convert local time to raw
  raw1 = lt.getRawTime();
  ASSERT_EQ(expectedEpochVal, raw1);

  // convert UTC time to raw
  raw2 = utc.getRawTime();
  ASSERT_EQ(expectedEpochVal, raw2);

  // create a timestamp from the epoch value
  lt = LocalTimestamp(expectedEpochVal);
  ASSERT_EQ("2015-01-27 11:00:00", lt.getTimestamp());
}

//----------------------------------------------------------------------------

TEST(Timestamps, testGetters)
{
  // create a fake localtime object
  LocalTimestamp lt(2000, 1, 1, 8, 3, 2, 0);

  ASSERT_EQ("2000-01-01", lt.getISODate());
  ASSERT_EQ("08:03:02", lt.getTime());
  ASSERT_EQ("2000-01-01 08:03:02", lt.getTimestamp());
}
