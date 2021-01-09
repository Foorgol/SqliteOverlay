#include <filesystem>

#include "BasicTestClass.h"
#include "Sloppy/Logger/Logger.h"

namespace fs = std::filesystem;
using namespace Sloppy::Logger;

void BasicTestFixture::SetUp()
{
  // create a dir for temporary files created during testing
  tstDirPath = fs::temp_directory_path();
  if (!(fs::exists(tstDirPath)))
  {
    throw std::runtime_error("Could not create temporary directory for test files!");
  }
}

void BasicTestFixture::TearDown()
{
}

std::string BasicTestFixture::getTestDir() const
{
  return tstDirPath.native();
}

std::string BasicTestFixture::genTestFilePath(const std::string& fName) const
{
  fs::path p = tstDirPath;
  p /= fName;
  return p.native();
}
