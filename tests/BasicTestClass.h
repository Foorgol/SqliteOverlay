#ifndef BASICTESTCLASS_H
#define	BASICTESTCLASS_H

#include <string>
#include <memory>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "Sloppy/Logger/Logger.h"

using namespace std;
namespace boostfs = boost::filesystem;
using namespace Sloppy::Logger;

class EmptyFixture
{

};

class BasicTestFixture : public ::testing::Test
{
protected:

  virtual void SetUp ();
  virtual void TearDown ();

  string getTestDir () const;
  string genTestFilePath(string fName) const;
  boostfs::path tstDirPath;
};

#endif /* BASICTESTCLASS_H */
