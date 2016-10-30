#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "DatabaseTestScenario.h"
#include "ActionMngr.h"

using namespace SqliteOverlay;

static constexpr const char* ActionTabName = "Actions";

enum class DemoActions
{
  A1,
  A2,
  A3
};

TEST_F(DatabaseTestScenario, ActionMngr_createTables)
{
  auto db = getScenario01();

  ASSERT_TRUE(db->getTab(ActionTabName) == nullptr);
  ActionMngr::ActionMngr am{db.get(), ActionTabName};
  ASSERT_TRUE(db->getTab(ActionTabName) != nullptr);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ActionMngr_createAction)
{
  auto db = getScenario01();
  ActionMngr::ActionMngr am{db.get(), ActionTabName};

  // create an action with infinite validity
  unique_ptr<ActionMngr::Action> a = am.createNewAction<DemoActions>(DemoActions::A2, -1);
  ASSERT_TRUE(a != nullptr);

  // make sure the table entries are correct
  DbTab* aTab = db->getTab(ActionTabName);
  ASSERT_EQ(1, aTab->length());
  TabRow r = (*aTab)[1];
  ASSERT_EQ(1, r.getInt(ActionMngr::PA_Action));
  string nonce = r[ActionMngr::PA_Nonce];
  ASSERT_EQ(ActionMngr::NonceLength, nonce.size());
  UTCTimestamp now;
  ASSERT_EQ(now.getRawTime(), r.getInt(ActionMngr::PA_CreatedOn));
  auto e = r.getInt2(ActionMngr::PA_ExpiresOn);
  ASSERT_TRUE(e->isNull());

  // create an action with 10 secs validity
  a = am.createNewAction<DemoActions>(DemoActions::A1, 10);
  ASSERT_TRUE(a != nullptr);

  // make sure the table entries are correct
  ASSERT_EQ(2, aTab->length());
  r = (*aTab)[2];
  ASSERT_EQ(0, r.getInt(ActionMngr::PA_Action));
  nonce = r[ActionMngr::PA_Nonce];
  ASSERT_EQ(ActionMngr::NonceLength, nonce.size());
  ASSERT_EQ(now.getRawTime(), r.getInt(ActionMngr::PA_CreatedOn));
  e = r.getInt2(ActionMngr::PA_ExpiresOn);
  ASSERT_FALSE(e->isNull());
  ASSERT_EQ(now.getRawTime() + 10, e->get());
}

//----------------------------------------------------------------


//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ActionMngr_delAction)
{
  auto db = getScenario01();
  ActionMngr::ActionMngr am{db.get(), ActionTabName};
  DbTab* aTab = db->getTab(ActionTabName);

  // create five actions with 10 secs validity
  for (int i=0; i<5 ; ++i)
  {
    auto a = am.createNewAction<DemoActions>(DemoActions::A3, 10);
    ASSERT_TRUE(a != nullptr);
  }
  ASSERT_EQ(5, aTab->length());

  // fake the expiration date of action #3 and #5
  UTCTimestamp fakeTime;
  fakeTime.applyOffset(-20);
  TabRow r = (*aTab)[3];
  ASSERT_TRUE(r.update(ActionMngr::PA_ExpiresOn, fakeTime));
  r = (*aTab)[5];
  ASSERT_TRUE(r.update(ActionMngr::PA_ExpiresOn, fakeTime));

  // trigger the removal of exipired actions
  ASSERT_TRUE(am.removeExpiredActions());

  // check the remaining actions
  ASSERT_EQ(3, aTab->length());
  ASSERT_THROW((*aTab)[3], std::invalid_argument);
  ASSERT_THROW((*aTab)[5], std::invalid_argument);
  ASSERT_NO_THROW((*aTab)[1]);
  ASSERT_NO_THROW((*aTab)[2]);
  ASSERT_NO_THROW((*aTab)[4]);

  // test the removal of a specific action
  auto a = am.getActionById(2);
  ASSERT_TRUE(a != nullptr);
  ASSERT_TRUE(am.removeAction(*a));
  ASSERT_EQ(2, aTab->length());
  ASSERT_NO_THROW((*aTab)[1]);
  ASSERT_THROW((*aTab)[2], std::invalid_argument);
  ASSERT_NO_THROW((*aTab)[4]);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ActionMngr_hasNonce)
{
  auto db = getScenario01();
  ActionMngr::ActionMngr am{db.get(), ActionTabName};
  DbTab* aTab = db->getTab(ActionTabName);

  // create five actions with 10 secs validity
  for (int i=0; i<5 ; ++i)
  {
    auto a = am.createNewAction<DemoActions>(DemoActions::A3, 10);
    ASSERT_TRUE(a != nullptr);
  }
  ASSERT_EQ(5, aTab->length());

  // grab a nonce directly from the table
  string nonce = (*aTab)[3][ActionMngr::PA_Nonce];
  ASSERT_EQ(ActionMngr::NonceLength, nonce.size());

  // test hasNonce()
  ASSERT_TRUE(am.hasNonce(nonce));
  ASSERT_FALSE(am.hasNonce("abc"));

  // test getActionByNonce
  auto a = am.getActionByNonce(nonce);
  ASSERT_TRUE(a != nullptr);
  ASSERT_EQ(3, a->getId());
  a = am.getActionByNonce("abc");
  ASSERT_TRUE(a == nullptr);
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ActionMngr_ActionGetters)
{
  auto db = getScenario01();
  ActionMngr::ActionMngr am{db.get(), ActionTabName};
  DbTab* aTab = db->getTab(ActionTabName);

  // create an action with 10 secs validity
  auto a = am.createNewAction<DemoActions>(DemoActions::A3, 10);
  ASSERT_TRUE(a != nullptr);
  TabRow r = (*aTab)[1];

  // test the getters
  ASSERT_EQ(r[ActionMngr::PA_Nonce], a->getNonce());
  ASSERT_EQ(DemoActions::A3, a->getType<DemoActions>());
  UTCTimestamp now;
  ASSERT_EQ(now, a->getCreationDate());
  now.applyOffset(10);
  ASSERT_EQ(now, *(a->getExpirationDate()));

  // test expiration
  ASSERT_FALSE(a->isExpired());
  now.applyOffset(-20);
  r.update(ActionMngr::PA_ExpiresOn, now);
  ASSERT_TRUE(a->isExpired());
}

//----------------------------------------------------------------

TEST_F(DatabaseTestScenario, ActionMngr_ActionOtherData)
{
  auto db = getScenario01();
  ActionMngr::ActionMngr am{db.get(), ActionTabName};
  DbTab* aTab = db->getTab(ActionTabName);

  // create an action with 10 secs validity
  auto a = am.createNewAction<DemoActions>(DemoActions::A3, 10);
  ASSERT_TRUE(a != nullptr);
  TabRow r = (*aTab)[1];

  // test non-existence of data
  ASSERT_FALSE(a->hasOtherData());
  ASSERT_TRUE(a->getOtherData2().isNull());

  // set other data
  ASSERT_TRUE(a->setOtherData("xyz123"));

  // test existence of other data
  ASSERT_TRUE(a->hasOtherData());
  ASSERT_FALSE(a->getOtherData2().isNull());
  ASSERT_EQ("xyz123", a->getOtherData());

  // remove other data from action
  ASSERT_TRUE(a->removeOtherData());

  // test non-existence of data
  ASSERT_FALSE(a->hasOtherData());
  ASSERT_TRUE(a->getOtherData2().isNull());
}


//----------------------------------------------------------------

