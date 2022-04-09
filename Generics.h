#pragma once

#include <string_view>
#include <string>
#include <array>

#include <Sloppy/ResultOrError.h>

#include "SqliteDatabase.h"
#include "SqlStatement.h"

namespace SqliteOverlay {
  struct ForeignKeyDescription {
    std::string_view referredTableName;
    std::string_view referredColumnName;
  };

  struct ColumnDescription {
    std::string_view name;
    SqliteOverlay::ColumnDataType dataType;
    SqliteOverlay::ConflictClause uniqueConflictClause;
    SqliteOverlay::ConflictClause notNullConflictClause;
    std::optional<ForeignKeyDescription> foreignKey;
  };

  template <class T> concept EnumType = std::is_enum_v<T>;
  template <class ContainerT, class DataT, int n> concept ConstStdArray =
      std::is_same_v<ContainerT, const std::array<DataT, n>>;

  template <class T>
  concept ViewAdapterClass =
      EnumType<typename T::Col> &&  // we need an enum with column names
      std::is_same_v<decltype (T::nCols), const int> &&   // we the number of columns
      std::is_same_v<decltype (T::TabName), const std::string_view> &&   // we need the name of the underlying SQLite table
      ConstStdArray<decltype (T::ColDefs), ColumnDescription, T::nCols> &&   // we need a list of column names
      std::is_same_v<decltype (T::FullSelectColList), const std::string_view> &&   // we need a comma-separated string of column names for SELECT, INSERT, ...
      std::is_aggregate_v<typename T::DbObj> &&   // we need a standardized name for the database object struct
      requires (T v, const SqliteOverlay::SqlStatement& stmt) {
        { T::fromSelectStmt(stmt) } -> std::same_as<typename T::DbObj>;   // we need a function that fills/generates an object from a SELECT statement
      };

  template <class T>
  concept TableAdapterClass =
      ViewAdapterClass<T> &&  // we need everything from a ViewAdapter plus ....
      (std::is_same_v<typename T::IdType, int> || std::is_same_v<typename T::IdType::UnderlyingType, int>) &&   // an "int" or "named type type" as ID type
      std::is_same_v<decltype (T::DbObj::id), typename T::IdType> &&   // the database object must have an "id" field
      (static_cast<int>(T::Col::id) == 0) &&  // there must be a column enum named "id" and it must have index 0
      requires (T v, SqliteOverlay::SqlStatement& stmt, const typename T::DbObj obj) {
        { T::bindToStmt(obj, stmt) };   // we need a function that binds values of a database object to an SQL statement
      };

  enum class DbOperationError {
    Busy,
    Constraint,
    NoSuchObject,
    Other
  };

  enum class ColumnValueComparisonOp {
    Equals,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    Null,
    NotNull
  };

  constexpr std::array<std::string_view, 8> ComparisonOp2String {
    std::string_view{"="},
    std::string_view{"!="},
    std::string_view{"<"},
    std::string_view{"<="},
    std::string_view{">"},
    std::string_view{">="},
    std::string_view{" IS NULL"},
    std::string_view{" IS NOT NULL"},
  };

  template<ViewAdapterClass AC>
  class GenericView {
  public:
    using Col = typename AC::Col;

    using ObjList = std::vector<typename AC::DbObj>;
    using OptOpject = std::optional<typename AC::DbObj>;
    using SingleObjectOrError = Sloppy::ResultOrError<typename AC::DbObj, DbOperationError>;
    using OptionalObjectOrError = Sloppy::ResultOrError<std::optional<typename AC::DbObj>, DbOperationError>;
    using ListOrError = Sloppy::ResultOrError<ObjList, DbOperationError>;

    explicit GenericView(SqliteOverlay::SqliteDatabase* db) noexcept
      : dbPtr{db}
      , sqlBaseSelect{
          "SELECT " + std::string{AC::FullSelectColList} +
          " FROM " + std::string{AC::TabName}
        } {}

    //-------------------------------------------------------------------------------------------------

    ListOrError allObj() const noexcept {
      auto stmt = safeStmt(sqlBaseSelect);
      if (!stmt) return DbOperationError::Other;
      return stmt2ObjectList(*stmt);
    }

    //-------------------------------------------------------------------------------------------------

    template<typename ...Args>
    OptionalObjectOrError singleObjectByColumnValue(Args ... whereArgs) const noexcept {
      auto stmt = stmtWithWhere(sqlBaseSelect, 1, std::forward<Args>(whereArgs)...);
      if (!stmt) return DbOperationError::Other;
      return stmt2SingleObject(*stmt);
    }

    //-------------------------------------------------------------------------------------------------

    template<typename ...Args>
    ListOrError objectsByColumnValue(Args ... whereArgs) const noexcept {
      auto stmt = stmtWithWhere(sqlBaseSelect, 0, std::forward<Args>(whereArgs)...);
      if (!stmt) return DbOperationError::Other;
      return stmt2ObjectList(*stmt);
    }


    //-------------------------------------------------------------------------------------------------

  protected:
    SqliteOverlay::SqliteDatabase* dbPtr;
    const std::string sqlBaseSelect;

    std::string colNameFromEnum(Col col) const {
      return std::string{AC::ColDefs[static_cast<int>(col)].name};
    }

    //-------------------------------------------------------------------------------------------------

    std::optional<SqliteOverlay::SqlStatement> safeStmt(const std::string& sql) const noexcept {
      try {
        return dbPtr->prepStatement(sql);
      }
      catch (...) {
        return std::nullopt;
      }
    }

    //-------------------------------------------------------------------------------------------------

    /** \pre The parameters have been bound to the statement, but step()
     *  has not yet been called
     */
    ListOrError stmt2ObjectList(SqliteOverlay::SqlStatement& stmt) const noexcept {
      try {
        ObjList result;
        while (stmt.dataStep()) {
          result.push_back(AC::fromSelectStmt(stmt));
        }
        return result;
      }
      catch (SqliteOverlay::BusyException&) {
        return DbOperationError::Busy;
      }
      catch (...) {
        return DbOperationError::Other;
      }
    }

    //-------------------------------------------------------------------------------------------------

    /** \pre The parameters have been bound to the statement, but step()
     *  has not yet been called
     */
    OptionalObjectOrError stmt2SingleObject(SqliteOverlay::SqlStatement& stmt) const noexcept {
      try {
        if (stmt.dataStep()) {
          return OptOpject{AC::fromSelectStmt(stmt)};
        } else {
          return OptOpject{std::nullopt};
        }
      }
      catch (SqliteOverlay::BusyException&) {
        return DbOperationError::Busy;
      }
      catch (...) {
        return DbOperationError::Other;
      }
    }

    //-------------------------------------------------------------------------------------------------

    // Laufende Iteration mit  "Col = Val" und Nachfolgespalte
    template<class T, typename ...Args>
    void recursiveWhereBuilder_langStep(std::string& w, int nextParaIdx, Col col, T&& val, Col nextCol, Args ... args) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      static_assert (!std::is_same_v<T, ColumnValueComparisonOp>, "fehler");
      w += colNameFromEnum(col) + "=?" + std::to_string(nextParaIdx) + " AND ";

      recursiveWhereBuilder_langStep(w, nextParaIdx+1, nextCol, std::forward<Args>(args)...);
    }

    // Laufende Iteration mit  "Col NOT NULL" und Nachfolgespalte
    template<typename ...Args>
    void recursiveWhereBuilder_langStep(std::string& w, int nextParaIdx, Col col, ColumnValueComparisonOp op, Col nextCol, Args ... args) const {
      assert((op == ColumnValueComparisonOp::NotNull) || (op == ColumnValueComparisonOp::Null));

      w += colNameFromEnum(col) + std::string{ComparisonOp2String[static_cast<int>(op)]};
      w += " AND ";
      recursiveWhereBuilder_langStep(w, nextParaIdx, nextCol, std::forward<Args>(args)...);
    }

    // Laufende Iteration mit  "Col op Val" und Nachfolgespalte
    template<class T, typename ...Args>
    void recursiveWhereBuilder_langStep(std::string& w, int nextParaIdx, Col col, ColumnValueComparisonOp op, T&& val, Col nextCol, Args ... args) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      static_assert (!std::is_same_v<T, ColumnValueComparisonOp>, "fehler");

      assert((op != ColumnValueComparisonOp::NotNull) && (op != ColumnValueComparisonOp::Null));

      w += colNameFromEnum(col) + std::string{ComparisonOp2String[static_cast<int>(op)]};
      w += "?" + std::to_string(nextParaIdx) + " AND ";
      recursiveWhereBuilder_langStep(w, nextParaIdx+1, nextCol, std::forward<Args>(args)...);
    }

    // Abschluss mit "Col = Val"
    template<class T>
    void recursiveWhereBuilder_langStep(std::string& w, int nextParaIdx, Col col, T&& val) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      w += colNameFromEnum(col) + "=?" + std::to_string(nextParaIdx);
    }

    // Abschluss mit "Col NOT NULL"
    void recursiveWhereBuilder_langStep(std::string& w, int nextParaIdx, Col col, ColumnValueComparisonOp op) const {
      w += colNameFromEnum(col) + std::string{ComparisonOp2String[static_cast<int>(op)]};
    }

    // Abschluss mit "Col op Val"
    template<class T>
    void recursiveWhereBuilder_langStep(std::string& w, int nextParaIdx, Col col, ColumnValueComparisonOp op, T&& val) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      static_assert (!std::is_same_v<T, ColumnValueComparisonOp>, "fehler");
      w += colNameFromEnum(col) + std::string{ComparisonOp2String[static_cast<int>(op)]};
      w += "?" + std::to_string(nextParaIdx);
    }

      //-------------------------------------------------------------------------------------------------

    // Laufende Iteration mit  "Col = Val" und Nachfolgespalte
    template<class T, typename ...Args>
    void recursiveWhereBuilder_bindStep(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, T&& val, Col nextCol, Args ... args) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      static_assert (!std::is_same_v<T, ColumnValueComparisonOp>, "fehler");

      stmt.bind(nextParaIdx, std::forward<T>(val));
      recursiveWhereBuilder_bindStep(stmt, nextParaIdx+1, nextCol, std::forward<Args>(args)...);
    }

    // Laufende Iteration mit  "Col NOT NULL" und Nachfolgespalte
    template<typename ...Args>
    void recursiveWhereBuilder_bindStep(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, ColumnValueComparisonOp op, Col nextCol, Args ... args) const {
      assert((op == ColumnValueComparisonOp::NotNull) || (op == ColumnValueComparisonOp::Null));

      recursiveWhereBuilder_bindStep(stmt, nextParaIdx, nextCol, std::forward<Args>(args)...);
    }

    // Laufende Iteration mit  "Col op Val" und Nachfolgespalte
    template<class T, typename ...Args>
    void recursiveWhereBuilder_bindStep(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, ColumnValueComparisonOp op, T&& val, Col nextCol, Args ... args) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      static_assert (!std::is_same_v<T, ColumnValueComparisonOp>, "fehler");

      assert((op != ColumnValueComparisonOp::NotNull) && (op != ColumnValueComparisonOp::Null));

      stmt.bind(nextParaIdx, std::forward<T>(val));
      recursiveWhereBuilder_bindStep(stmt, nextParaIdx+1, nextCol, std::forward<Args>(args)...);
    }

    // Abschluss mit "Col = Val"
    template<class T>
    void recursiveWhereBuilder_bindStep(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, T&& val) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      stmt.bind(nextParaIdx, std::forward<T>(val));
    }

    // Abschluss mit "Col NOT NULL"
    void recursiveWhereBuilder_bindStep(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, ColumnValueComparisonOp op) const {
      assert((op == ColumnValueComparisonOp::NotNull) || (op == ColumnValueComparisonOp::Null));
    }

    // Abschluss mit "Col op Val"
    template<class T>
    void recursiveWhereBuilder_bindStep(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, ColumnValueComparisonOp op, T&& val) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      static_assert (!std::is_same_v<T, ColumnValueComparisonOp>, "fehler");

      assert((op != ColumnValueComparisonOp::NotNull) && (op != ColumnValueComparisonOp::Null));

      stmt.bind(nextParaIdx, std::forward<T>(val));
    }

    //-------------------------------------------------------------------------------------------------

    template<typename ...Args>
    std::optional<SqliteOverlay::SqlStatement> stmtWithWhere(const std::string& baseSql, int limit, Args ... whereArgs) const {
      std::string sql = baseSql + " WHERE ";
      recursiveWhereBuilder_langStep(sql, 1, std::forward<Args>(whereArgs)...);
      if (limit > 0) {
        sql += " LIMIT " + std::to_string(limit);
      }
      std::cout << sql << std::endl;
      try {
        auto stmt = dbPtr->prepStatement(sql);
        recursiveWhereBuilder_bindStep(stmt, 1, std::forward<Args>(whereArgs)...);
        std::cout << stmt.getExpandedSQL() << "\n" << std::endl;
        return stmt;
      }
      catch (...) {
        return std::nullopt;
      }
    }

    //-------------------------------------------------------------------------------------------------

  };

  template<TableAdapterClass AC>
  class GenericTable : public GenericView<AC> {
  public:
    using IdType = typename AC::IdType;
    using Parent = GenericView<AC>;

    using OptionalObjectOrError = typename Parent::OptionalObjectOrError;


    explicit GenericTable(SqliteOverlay::SqliteDatabase* db) noexcept
      : GenericView<AC>(db) {}

    OptionalObjectOrError singleObjectById(IdType id) const noexcept {
      const std::string sql = Parent::sqlBaseSelect + " WHERE rowid = ?1";
      auto stmt = Parent::safeStmt(sql);
      if (!stmt) return DbOperationError::Other;

      if constexpr (std::is_same_v<IdType, int>) {
        stmt->bind(1, id);
      } else {
        stmt->bind(1, id.get());
      }

      return Parent::stmt2SingleObject(*stmt);
    }
  };
}
