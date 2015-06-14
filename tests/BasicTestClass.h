#ifndef BASICTESTCLASS_H
#define	BASICTESTCLASS_H

#include <string>
#include <memory>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "Logger.h"

using namespace std;
namespace boostfs = boost::filesystem;

class EmptyFixture
{

};

class BasicTestFixture : public ::testing::Test
{
protected:
  static constexpr char DB_TEST_FILE_NAME[] = "SqliteTestDB.db";

  virtual void SetUp ();
  virtual void TearDown ();

  string getTestDir ();
  string genTestFilePath(string fName);
  boostfs::path tstDirPath;
  unique_ptr<SqliteOverlay::Logger> log;
  void printStartMsg(string _tcName);
  void printEndMsg();

private:
  string tcName;
};

#endif /* BASICTESTCLASS_H */
