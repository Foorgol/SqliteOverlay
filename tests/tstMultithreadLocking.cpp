#include <thread>
#include <iostream>

#include "DatabaseTestScenario.h"
#include "SqliteDatabase.h"

using namespace std;
using namespace SqliteOverlay;

enum class FakeRoles
{
  Main,
  SeparateThread
};
using DbLock = DatabaseLockHolder<FakeRoles>;

//----------------------------------------------------------------------------

void DummyLockThread(SqliteDatabase* db, int* flagVar)
{
  *flagVar = 0;
  DbLock lck{db, FakeRoles::SeparateThread};
  cout << "Entered!" << endl;
  *flagVar = 1;
}

//----------------------------------------------------------------------------

TEST_F(DatabaseTestScenario, MultithreadLocking)
{

  auto db = getScenario01();

  // get a lock for the "Main" thread
  unique_ptr<DbLock> l1 = make_unique<DbLock>(db.get(), FakeRoles::Main);
  ASSERT_TRUE(l1 != nullptr);

  // acquiring the lock again should not block
  DbLock l2{db.get(), FakeRoles::Main};

  // start a thread that also wants to get access
  int flag = 0;
  thread t{DummyLockThread, db.get(), &flag};
  this_thread::sleep_for(chrono::milliseconds{500});
  ASSERT_EQ(0, flag);  // 0 ==> thread is still blocking

  // release our database lock
  l1.reset();

  this_thread::sleep_for(chrono::milliseconds{100});
  ASSERT_EQ(1, flag);  // 1 ==> thread has started

  t.join();
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

