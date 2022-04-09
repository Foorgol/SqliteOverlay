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

bool equalsExampleObj(const ExampleTable::OptionalObjectOrError& o, int refId) {
  return (o.isOk() && o->has_value() && ((**o) == ExampleObjects[refId - 1]));
}

bool okayButEmpty(const ExampleTable::OptionalObjectOrError& o) {
  return (o.isOk() && !o->has_value());
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
  ASSERT_TRUE(okayButEmpty(obj));
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
  ASSERT_TRUE(okayButEmpty(obj));
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null,
          ExampleTable::Col::intCol, 84
          );
  ASSERT_TRUE(equalsExampleObj(obj, 3));
  obj = t.singleObjectByColumnValue(
          ExampleTable::Col::intCol, 80,
          ExampleTable::Col::realCol, ColumnValueComparisonOp::Null
          );
  ASSERT_TRUE(okayButEmpty(obj));
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
  ASSERT_TRUE(v.isOk());
  ASSERT_EQ(v->size(), 5);
  for (int idx = 0; idx < 5; ++idx) {
    ASSERT_EQ((*v)[idx], ExampleObjects[idx]);
  }

  v = t.objectsByColumnValue(ExampleTable::Col::realCol, ColumnValueComparisonOp::Null);
  ASSERT_TRUE(v.isOk());
  ASSERT_EQ(v->size(), 2);
  ASSERT_EQ((*v)[0], ExampleObjects[2]);
  ASSERT_EQ((*v)[1], ExampleObjects[3]);

  v = t.objectsByColumnValue(ExampleTable::Col::realCol, 42.42);
  ASSERT_TRUE(v.isOk());
  ASSERT_EQ(v->size(), 1);
  ASSERT_EQ((*v)[0], ExampleObjects[4]);

  v = t.objectsByColumnValue(ExampleTable::Col::realCol, ColumnValueComparisonOp::GreaterThan, 1000);
  ASSERT_TRUE(v.isOk());
  ASSERT_EQ(v->size(), 0);
}
