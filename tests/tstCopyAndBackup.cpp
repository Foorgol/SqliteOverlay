#include <memory>
#include <climits>
#include <filesystem>

#include <Sloppy/Utils.h>


#include <gtest/gtest.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"
#include "TabRow.h"
#include "KeyValueTab.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, CopyTable)
{
  auto db = getScenario01();

  // try invalid parameters
  ASSERT_FALSE(db.copyTable("", ""));
  ASSERT_FALSE(db.copyTable("t1", ""));
  ASSERT_FALSE(db.copyTable("t1", "t1"));
  ASSERT_FALSE(db.copyTable("sfsfsafd", "t2"));

  // copy whole table t1 to t1_copy
  ASSERT_TRUE(db.copyTable("t1", "t1_copy"));

  // make sure the new table exists and is filled
  ASSERT_TRUE(db.hasTable("t1_copy"));
  ASSERT_TRUE(DbTab(db, "t1", false).length() == DbTab(db, "t1_copy", false).length());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, BackupCopy)
{
  auto db = getScenario01();

  string bckFileName = genTestFilePath("backup.sqlite");

  // make sure there is no stale file from a previous run
  ASSERT_FALSE(Sloppy::isFile(bckFileName));

  // trigger the backup
  ASSERT_TRUE(db.backupToFile(bckFileName));

  // make sure the backup file has been created
  ASSERT_TRUE(Sloppy::isFile(bckFileName));

  // compare source and destination file
  db.close();
  Sloppy::MemFile srcFile{getSqliteFileName()};
  Sloppy::MemFile dstFile{bckFileName};
  ASSERT_TRUE(dstFile.size() > 0);
  ASSERT_EQ(srcFile.size(), dstFile.size());
  // a byte-by-byte comparison fails, perhaps
  // because SQLite writes some updates to the
  // header of the source database when calling close()

  // a rough comparison of the two databases (only
  // one column of t1)
  SqliteDatabase src{getSqliteFileName(), OpenMode::OpenExisting_RO};
  SqliteDatabase cpy{bckFileName, OpenMode::OpenExisting_RO};
  DbTab t1Src(src, "t1", true);
  DbTab t1Dst(cpy, "t1", true);
  auto srcIter = t1Src.tabRowIterator();
  auto dstIter = t1Dst.tabRowIterator();
  while (srcIter.hasData())
  {
    auto srcRow = *srcIter;
    auto dstRow = *dstIter;

    ASSERT_TRUE(srcRow["s"] == dstRow["s"]);
    ASSERT_TRUE(srcIter.rowid() == dstIter.rowid());

    ++srcIter;
    ++dstIter;
  }
  ASSERT_FALSE(dstIter.hasData());

  // delete the backup
  ASSERT_TRUE(std::filesystem::remove(bckFileName));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, RestoreFromCopy)
{
  auto db = getScenario01();

  string bckFileName = genTestFilePath("backup.sqlite");

  // make sure there is no stale file from a previous run
  ASSERT_FALSE(Sloppy::isFile(bckFileName));

  // trigger the backup
  ASSERT_TRUE(db.backupToFile(bckFileName));

  // make sure the backup file has been created
  ASSERT_TRUE(Sloppy::isFile(bckFileName));

  // change the source
  DbTab t1{db, "t1", false};
  t1.insertRow();
  ASSERT_EQ(6, t1.length());

  // restore from backup, overwriting the changes
  ASSERT_TRUE(db.restoreFromFile(bckFileName));
  ASSERT_EQ(5, t1.length());

  // delete the backup
  ASSERT_TRUE(std::filesystem::remove(bckFileName));
}
