#include <boost/filesystem.hpp>

#include "BasicTestClass.h"
#include "Sloppy/Logger/Logger.h"

namespace boostfs = boost::filesystem;
using namespace Sloppy::Logger;

void BasicTestFixture::SetUp()
{
  // create a dir for temporary files created during testing
  tstDirPath = boostfs::temp_directory_path();
  if (!(boostfs::exists(tstDirPath)))
  {
    throw std::runtime_error("Could not create temporary directory for test files!");
  }
}

void BasicTestFixture::TearDown()
{
}

string BasicTestFixture::getTestDir() const
{
  return tstDirPath.native();
}

string BasicTestFixture::genTestFilePath(string fName) const
{
  boostfs::path p = tstDirPath;
  p /= fName;
  return p.native();
}
