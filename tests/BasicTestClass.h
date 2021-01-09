#ifndef BASICTESTCLASS_H
#define	BASICTESTCLASS_H

#include <string>
#include <memory>
#include <gtest/gtest.h>
#include <filesystem>

#include "Sloppy/Logger/Logger.h"


class EmptyFixture
{

};

class BasicTestFixture : public ::testing::Test
{
protected:

  virtual void SetUp ();
  virtual void TearDown ();

  std::string getTestDir () const;
  std::string genTestFilePath(const std::string& fName) const;
  std::filesystem::path tstDirPath;
};

#endif /* BASICTESTCLASS_H */
