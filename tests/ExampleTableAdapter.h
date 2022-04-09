#pragma once

#include <Sloppy/NamedType.h>
#include <Sloppy/DateTime/DateAndTime.h>
#include <Sloppy/Utils.h>

#include "../Generics.h"

using ExampleId = Sloppy::NamedType<int, struct ExampleIdTag>;

struct ExampleObj {
  ExampleId id;
  std::optional<int> i;
  std::optional<double> f;
  std::string s;
  date::year_month_day d;
};

bool operator==(const ExampleObj& lhs, const ExampleObj& rhs) {
  return (
        (lhs.id == rhs.id) &&
        (lhs.i == rhs.i) &&
        (lhs.f == rhs.f) &&
        (lhs.s == rhs.s) &&
        (lhs.d == rhs.d)
        );
}

class ExampleAdapterClass {
public:
  using DbObj = ExampleObj;
  using IdType = ExampleId;

  static constexpr std::string_view TabName{"t1"};

  static constexpr int nCols = 5;

  enum class Col {
    id = 0,
    intCol,
    realCol,
    stringCol,
    dateCol,
  };

  static constexpr std::array<SqliteOverlay::ColumnDescription, nCols> ColDefs{
    SqliteOverlay::ColumnDescription{"rowid", SqliteOverlay::ColumnDataType::Integer, SqliteOverlay::ConflictClause::NotUsed, SqliteOverlay::ConflictClause::NotUsed, std::nullopt},
    SqliteOverlay::ColumnDescription{"i", SqliteOverlay::ColumnDataType::Integer, SqliteOverlay::ConflictClause::NotUsed, SqliteOverlay::ConflictClause::NotUsed, std::nullopt},
    SqliteOverlay::ColumnDescription{"f", SqliteOverlay::ColumnDataType::Float, SqliteOverlay::ConflictClause::NotUsed, SqliteOverlay::ConflictClause::NotUsed, std::nullopt},
    SqliteOverlay::ColumnDescription{"s", SqliteOverlay::ColumnDataType::Text, SqliteOverlay::ConflictClause::NotUsed, SqliteOverlay::ConflictClause::NotUsed, std::nullopt},
    SqliteOverlay::ColumnDescription{"d", SqliteOverlay::ColumnDataType::Integer, SqliteOverlay::ConflictClause::NotUsed, SqliteOverlay::ConflictClause::NotUsed, std::nullopt},
  };

  static constexpr std::string_view FullSelectColList{"rowid,i,f,s,d"};

  static ExampleObj fromSelectStmt(const SqliteOverlay::SqlStatement& stmt) {
    const std::string sDate = stmt.get<std::string>(4);
    const int y = std::stoi(sDate.substr(0, 4));
    const unsigned m = std::stoi(sDate.substr(5,2));
    const int d = std::stoi(sDate.substr(8,2));

    return ExampleObj{
      .id = ExampleId{stmt.get<int>(0)},
      .i = stmt.get2<int>(1),
      .f = stmt.get2<double>(2),
      .s = stmt.get<std::string>(3),
      .d = date::year_month_day{date::year{y} / date::month{m} / static_cast<unsigned>(d)}
    };
  }

  static void bindToStmt(const DbObj& obj, SqliteOverlay::SqlStatement& stmt) {
    stmt.bind(1, obj.id.get());
    obj.i ? stmt.bind(2, obj.i.value()) : stmt.bindNull(2);
    obj.f ? stmt.bind(3, obj.f.value()) : stmt.bindNull(3);
    stmt.bind(4, obj.s);
    std::string d = std::to_string(static_cast<int>(obj.d.year())) + "-";
    d += Sloppy::zeroPaddedNumber(static_cast<unsigned>(obj.d.month()), 2) + "-";
    d += Sloppy::zeroPaddedNumber(static_cast<unsigned>(obj.d.day()), 2);
    stmt.bind(5, d);
  }
};

using ExampleTable = SqliteOverlay::GenericTable<ExampleAdapterClass>;
