#include <boost/filesystem.hpp>

#include "BasicTestClass.h"
#include "Logger.h"

namespace boostfs = boost::filesystem;

void BasicTestFixture::SetUp()
{
  //qDebug() << "\n\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n";

  log = unique_ptr<SqliteOverlay::Logger>(new SqliteOverlay::Logger("UnitTest"));

  // create a dir for temporary files created during testing
  tstDirPath = boostfs::temp_directory_path();
  if (!(boostfs::exists(tstDirPath)))
  {
    throw std::runtime_error("Could not create temporary directory for test files!");
  }

  log->info("Using directory " + tstDirPath.native() + " for temporary files");
}

void BasicTestFixture::TearDown()
{
//  QString path = tstDir.path();

//  if (!(tstDir.remove()))
//  {
//    QString msg = "Could not remove temporary directory " + path;
//    QByteArray ba = msg.toLocal8Bit();
//    throw std::runtime_error(ba.data());
//  }

//  log.info("Deleted temporary directory " + tstDir.path() + " and all its contents");
}

string BasicTestFixture::getTestDir()
{
  return tstDirPath.native();
}

string BasicTestFixture::genTestFilePath(string fName)
{
  boostfs::path p = tstDirPath;
  p /= fName;
  return p.native();
}

void BasicTestFixture::printStartMsg(string _tcName)
{
  tcName = _tcName;
  //log.info("\n\n----------- Starting test case '" + tcName + "' -----------");
}

void BasicTestFixture::printEndMsg()
{
  //log.info("----------- End test case '" + tcName + "' -----------\n\n");
}
