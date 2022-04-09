#include <optional>

#include <gtest/gtest.h>

#include <Sloppy/NamedType.h>
#include <Sloppy/DateTime/DateAndTime.h>

#include "DatabaseTestScenario.h"
#include "ExampleTableAdapter.h"

using namespace SqliteOverlay;

inline const std::array<ExampleObj, 5> ExampleObjects = {
  ExampleObj{
    .id = ExampleId{1},
    .i = 42,
    .f = 23.23,
    .s = "Hallo",
    .d = Sloppy::DateTime::WallClockTimepoint_secs().ymd()
  },
  ExampleObj{
    .id = ExampleId{2},
    .i = std::nullopt,
    .f = 666.66,
    .s = "Hi",
    .d = Sloppy::DateTime::WallClockTimepoint_secs().ymd()
  },
  ExampleObj{
    .id = ExampleId{3},
    .i = 84,
    .f = std::nullopt,
    .s = "äöüÄÖÜ",
    .d = Sloppy::DateTime::WallClockTimepoint_secs().ymd()
  },
  ExampleObj{
    .id = ExampleId{4},
    .i = 84,
    .f = std::nullopt,
    .s = "Ho",
    .d = Sloppy::DateTime::WallClockTimepoint_secs().ymd()
  },
  ExampleObj{
    .id = ExampleId{5},
    .i = 84,
    .f = 42.42,
    .s = "Ho",
    .d = Sloppy::DateTime::WallClockTimepoint_secs().ymd()
  }
};

bool equalsExampleObj(const ExampleTable::OptObj& o, int refId) {
  return (o.has_value() && (*o == ExampleObjects[refId - 1]));
}

TEST_F(DatabaseTestScenario, Generics_SelectSingle)
{
  SampleDB db = getScenario01();

  ExampleTable t{&db};

  auto obj = t.singleObjectById(ExampleId{1});
  ASSERT_TRUE(equalsExampleObj(obj, 1));

  // single column equals given value
  obj = t.singleObjectByColumnValue(ExampleTable::Col::stringCol, "Ho");
  ASSERT_TRUE(equalsExampleObj(obj, 4));

  // AND of two columns
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::stringCol, "Ho",
          ExampleTable::Col::realCol, 42.42
          );
  ASSERT_TRUE(equalsExampleObj(obj, 5));

  // NULL column
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::intCol, ColumnValueComparisonOp::Null
          );
  ASSERT_TRUE(equalsExampleObj(obj, 2));

  // NOT NULL column
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::intCol, ColumnValueComparisonOp::NotNull
          );
  ASSERT_TRUE(equalsExampleObj(obj, 1));

  // NULL column combined with comparison
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null,
          ExampleTable::Col::intCol, ColumnValueComparisonOp::LessThan, 80
          );
  ASSERT_FALSE(obj);
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null,
          ExampleTable::Col::intCol, ColumnValueComparisonOp::GreaterThan, 80
          );
  ASSERT_TRUE(equalsExampleObj(obj, 3));

  // NULL column combined with direct value
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null,
          ExampleTable::Col::intCol, 80
          );
  ASSERT_FALSE(obj);
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null,
          ExampleTable::Col::intCol, 84
          );
  ASSERT_TRUE(equalsExampleObj(obj, 3));
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::intCol, 80,
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null
          );
  ASSERT_FALSE(obj);
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::intCol, 84,
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null
          );
  ASSERT_TRUE(equalsExampleObj(obj, 3));
}

//------------------------------------------------------------------

TEST_F(DatabaseTestScenario, Generics_SelectMultiple)
{
  SampleDB db = getScenario01();

  ExampleTable t{&db};

  auto v = t.allObj();
  ASSERT_EQ(v.size(), 5);
  for (int idx = 0; idx < 5; ++idx) {
    ASSERT_EQ(v[idx], ExampleObjects[idx]);
  }

  v = t.objectsByColumnValue(ExampleTable::Col::realCol, ColumnValueComparisonOp::Null);
  ASSERT_EQ(v.size(), 2);
  ASSERT_EQ(v[0], ExampleObjects[2]);
  ASSERT_EQ(v[1], ExampleObjects[3]);

  v = t.objectsByColumnValue(ExampleTable::Col::realCol, 42.42);
  ASSERT_EQ(v.size(), 1);
  ASSERT_EQ(v[0], ExampleObjects[4]);

  v = t.objectsByColumnValue(ExampleTable::Col::realCol, ColumnValueComparisonOp::GreaterThan, 1000);
  ASSERT_EQ(v.size(), 0);
}

//------------------------------------------------------------------

TEST_F(DatabaseTestScenario, Generics_Insert)
{
  SampleDB db = getScenario01();

  ExampleTable t{&db};
  ExampleObj newObj {
    .id = ExampleId{999},
    .i = 100,
    .f = 3.1415,
    .s = "my insert",
    .d = Sloppy::DateTime::WallClockTimepoint_secs().ymd()
  };
  auto newId = t.insert(newObj);
  ASSERT_EQ(newId, ExampleId{6});
  newObj.id = ExampleId{6};
  const auto readbackObj = t.singleObjectById(newObj.id);
  ASSERT_TRUE(readbackObj);
  ASSERT_EQ(*readbackObj, newObj);
}

//------------------------------------------------------------------

TEST_F(DatabaseTestScenario, Generics_Count)
{
  SampleDB db = getScenario01();

  ExampleTable t{&db};

  auto n = t.objCount();
  ASSERT_EQ(n, 5);
}

//------------------------------------------------------------------

TEST_F(DatabaseTestScenario, Generics_Delete)
{
  SampleDB db = getScenario01();

  ExampleTable t{&db};

  auto n = t.del(ExampleId{100});
  ASSERT_EQ(n, 0);
  ASSERT_EQ(t.objCount(), 5);

  n = t.del(ExampleId{5});
  ASSERT_EQ(n, 1);
  ASSERT_EQ(t.objCount(), 4);
  auto check = t.singleObjectById(ExampleId{5});
  ASSERT_FALSE(check);

  n = t.del(
        ExampleTable::Col::realCol, ColumnValueComparisonOp::Null
        );
  ASSERT_EQ(n, 2);
  ASSERT_EQ(t.objCount(), 2);
  check = t.singleObjectById(ExampleId{3});
  ASSERT_FALSE(check);
  check = t.singleObjectById(ExampleId{4});
  ASSERT_FALSE(check);

  n = t.del(ExampleTable::Col::realCol, 23.23);
  ASSERT_EQ(n, 1);
  ASSERT_EQ(t.objCount(), 1);
  check = t.singleObjectById(ExampleId{1});
  ASSERT_FALSE(check);

  // only ID 2 survived
  check = t.singleObjectById(ExampleId{2});
  ASSERT_TRUE(equalsExampleObj(check, 2));
}

//------------------------------------------------------------------

TEST_F(DatabaseTestScenario, Generics_UpdateSingle)
{
  SampleDB db = getScenario01();

  ExampleTable t{&db};

  auto n = t.updateSingle(ExampleId{2}, ExampleTable::Col::intCol, 4242);
  ASSERT_EQ(n, 1);
  auto check = t.singleObjectById(ExampleId{2});
  ASSERT_TRUE(check);
  ASSERT_EQ(check->i, 4242);

  n = t.updateSingle(ExampleId{2},
                     ExampleTable::Col::intCol, 9999,
                     ExampleTable::Col::stringCol, "xyz"
                     );
  ASSERT_EQ(n, 1);
  check = t.singleObjectById(ExampleId{2});
  ASSERT_TRUE(check);
  ASSERT_EQ(check->i, 9999);
  ASSERT_EQ(check->s, "xyz");
}
