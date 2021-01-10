#include <thread>
#include <gtest/gtest.h>

#include <Sloppy/Crypto/Crypto.h>
#include <Sloppy/Timer.h>

#include "DatabaseTestScenario.h"
#include "SampleDB.h"
#include "ClausesAndQueries.h"
#include "DbTab.h"
#include "Transaction.h"

using namespace SqliteOverlay;

TEST_F(DatabaseTestScenario, BusyException)
{
  SampleDB con1 = getScenario01();
  auto con2 = con1.duplicateConnection(false);

  con2.setBusyTimeout(500);

  auto tr = con1.startTransaction(TransactionType::Exclusive);

  Sloppy::Timer t;
  ASSERT_THROW(con2.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"), BusyException);
  t.stop();
  ASSERT_TRUE(t.getTime__ms() >= 500);
  ASSERT_TRUE(t.getTime__ms() <= 510);

  tr.rollback();
  ASSERT_NO_THROW(con2.execScalarQueryInt("SELECT i FROM t1 WHERE rowid=1"));
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, BusyThreads)
{
  static constexpr int LoopLimit = 3000;
  static constexpr int MaxIdleTime_ms = 5;

  SampleDB con1 = getScenario01();
  auto con2 = con1.duplicateConnection(false);
  auto con3 = con1.duplicateConnection(false);

  con1.setBusyTimeout(10000);
  con2.setBusyTimeout(10000);
  con3.setBusyTimeout(10000);

  auto worker = [](const SqliteDatabase& db, TransactionType tt)
  {
    int i = 0;
    int nCalls = 0;
    int maxWait = 0;
    int minWait = 100000;
    DbTab t2{db, "t2", true};

    while (i < LoopLimit)
    {
      ++nCalls;
      Sloppy::Timer t;
      auto tr = db.startTransaction(tt);
      t.stop();
      if (t.getTime__ms() > maxWait) maxWait = t.getTime__ms();
      if (t.getTime__ms() < minWait) minWait = t.getTime__ms();

      SqlStatement stmt = db.prepStatement("SELECT rowid,i FROM t2 ORDER BY rowid DESC LIMIT 1");
      stmt.step();
      i = stmt.get<int>(1);
      if (i >= LoopLimit) break;

      ColumnValueClause cvc;
      cvc.addCol("i", i+2);

      int idleTime = Sloppy::Crypto::rng() % MaxIdleTime_ms;
      this_thread::sleep_for(chrono::milliseconds(idleTime));

      t2.insertRow(cvc);

      tr.commit();
      this_thread::sleep_for(chrono::milliseconds(idleTime));
    }

    cout << "THREAD: min/max wait time for transaction was " << minWait << " / " << maxWait << " ms! " << nCalls << " iterations." << endl;
  };

  for (TransactionType tt : {TransactionType::Immediate, TransactionType::Exclusive})
  {
    cout << "\n   --- TransactionType = ";
    switch (tt)
    {
    case TransactionType::Deferred:
      cout << "Deferred\n";
      break;
    case TransactionType::Immediate:
      cout << "Immediate\n";
      break;
    case TransactionType::Exclusive:
      cout << "Exclusive\n";
      break;
    }
    cout << endl;

    // initial value so that we don't have to deal with NULL
    // in the worker
    ColumnValueClause cvc;
    cvc.addCol("i", 0);
    DbTab tab2{con1, "t2", true};
    tab2.insertRow(cvc);

    thread t1([&](){worker(con1, tt);});
    thread t2([&](){worker(con2, tt);});
    thread t3([&](){worker(con3, tt);});
    t1.join();
    t2.join();
    t3.join();

    for (auto it = tab2.tabRowIterator(); it.hasData(); ++it)
    {
      ASSERT_EQ((it.rowid()-1)*2, it->get<int>("i") );
    }
    ASSERT_EQ(1 + LoopLimit/2, tab2.length());

    con1.execNonQuery("DELETE FROM t2 WHERE rowid>0");
  }
}

//----------------------------------------------------------------

