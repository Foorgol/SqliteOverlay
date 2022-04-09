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

  template<class ValueType>
  using DbErrorOr = Sloppy::ResultOrError<ValueType, DbOperationError>;

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

    using DbObj = typename AC::DbObj;
    using ObjList = std::vector<DbObj>;
    using OptOpject = std::optional<DbObj>;
    using SingleObjectOrError = Sloppy::ResultOrError<DbObj, DbOperationError>;
    using OptionalObjectOrError = Sloppy::ResultOrError<std::optional<DbObj>, DbOperationError>;
    using ListOrError = Sloppy::ResultOrError<ObjList, DbOperationError>;

    explicit GenericView(SqliteOverlay::SqliteDatabase* db) noexcept
      : dbPtr{db}
      , sqlBaseSelect{
          "SELECT " + std::string{AC::FullSelectColList} +
          " FROM " + std::string{AC::TabName}
        }
      , sqlCountAll{"SELECT COUNT(*) FROM " + std::string{AC::TabName}}
      , sqlCountWhere{sqlCountAll + " WHERE "}
    {

    }

    //-------------------------------------------------------------------------------------------------

    ListOrError allObj() const noexcept {
      auto stmt = safeStmt(sqlBaseSelect);
      if (!stmt) return DbOperationError::Other;
      return stmt2ObjectList(*stmt);
    }

    //-------------------------------------------------------------------------------------------------

    DbErrorOr<int> objCount() const noexcept {
      try {
        auto stmt = dbPtr->prepStatement(sqlCountAll);
        if (!stmt.dataStep()) return DbOperationError::Other;
        return stmt.get<int>(0);
      }
      catch (SqliteOverlay::BusyException&) {
        return DbOperationError::Busy;
      }
      catch (...) {
        return DbOperationError::Other;
      }
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
    const std::string sqlCountAll;
    const std::string sqlCountWhere;

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
    using Col = typename Parent::Col;

    using OptionalObjectOrError = typename Parent::OptionalObjectOrError;
    using DbObj = typename Parent::DbObj;

    //---------------------------------------------------------------

    explicit GenericTable(SqliteOverlay::SqliteDatabase* db) noexcept
      : GenericView<AC>(db)
      , sqlBaseDelete{"DELETE FROM " + std::string{AC::TabName}}
      , sqlBaseDeleteById{sqlBaseDelete + " WHERE " + std::string{AC::ColDefs[0].name} + "=?1"}
      , sqlBaseUpdate{"UPDATE " + std::string{AC::TabName} + " SET "}
    {
      sqlBaseInsert =
          "INSERT INTO " + std::string{AC::TabName} +
          " (" + std::string{AC::FullSelectColList} + ") VALUES (";

      for (int i = 1; i <= AC::nCols; ++i) {
        sqlBaseInsert += "?" + std::to_string(i);
        if (i != AC::nCols) sqlBaseInsert += ",";
      }

      sqlBaseInsert += ")";

      std::cout << sqlBaseDeleteById << std::endl;
    }

    //---------------------------------------------------------------

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

    //---------------------------------------------------------------

    DbErrorOr<IdType> insert(const DbObj& obj) const noexcept {
      try {
        auto stmt = this->dbPtr->prepStatement(sqlBaseInsert);

        // let the adapter class bind the object values to
        // the statement and then overwrite the ID value with
        // NULL so that we get an SQLite-defined ID
        AC::bindToStmt(obj, stmt);
        stmt.bindNull(1);
        std::cout << stmt.getExpandedSQL() << std::endl;

        stmt.step();  // always suceeds; might throw, though

        return IdType{this->dbPtr->getLastInsertId()};
      }
      catch (SqliteOverlay::BusyException&) {
        return DbOperationError::Busy;
      }
      catch (SqliteOverlay::ConstraintFailedException&) {
        return DbOperationError::Constraint;
      }
      catch (...) {
        return DbOperationError::Other;
      }
    }

    //---------------------------------------------------------------

    DbErrorOr<int> del(const IdType& id) const noexcept {
      try {
        auto stmt = this->dbPtr->prepStatement(sqlBaseDeleteById);

        if constexpr (std::is_same_v<IdType, int>) {
          stmt.bind(1, id);
        } else {
          stmt.bind(1, id.get());   // for NamedTypes
        }

        stmt.step();  // always suceeds; might throw, though

        return this->dbPtr->getRowsAffected();
      }
      catch (SqliteOverlay::BusyException&) {
        return DbOperationError::Busy;
      }
      catch (SqliteOverlay::ConstraintFailedException&) {
        return DbOperationError::Constraint;
      }
      catch (...) {
        return DbOperationError::Other;
      }
    }

    //---------------------------------------------------------------

    DbErrorOr<int> del(const DbObj& obj) const noexcept {
      return del(obj.id);
    }

    //---------------------------------------------------------------

    template<typename ...Args>
    DbErrorOr<int> del(Args ... whereArgs) const noexcept {
      auto stmt = this->stmtWithWhere(sqlBaseDelete, 0, std::forward<Args>(whereArgs)...);
      if (!stmt) return DbOperationError::Other;

      try {
        stmt->step();  // always suceeds; might throw, though
        return this->dbPtr->getRowsAffected();
      }
      catch (SqliteOverlay::BusyException&) {
        return DbOperationError::Busy;
      }
      catch (SqliteOverlay::ConstraintFailedException&) {
        return DbOperationError::Constraint;
      }
      catch (...) {
        return DbOperationError::Other;
      }
    }

    //---------------------------------------------------------------

    template<typename ...Args>
    DbErrorOr<int> updateSingle(const IdType& id, Args ... columnValuePairs) {
      std::string sql = sqlBaseUpdate;
      recursiveUpdateBuilder_langStep(sql, 1, std::forward<Args>(columnValuePairs)...);
      sql += " WHERE " + std::string{AC::ColDefs[0].name} + "=";
      if constexpr(std::is_same_v<IdType, int>) {
        sql += std::to_string(id);
      } else {
      sql += std::to_string(id.get());
      }

      try {
        auto stmt = this->dbPtr->prepStatement(sql);
        recursiveValueBinder(stmt, 1, std::forward<Args>(columnValuePairs)...);
        std::cout << stmt.getExpandedSQL() << std::endl;

        stmt.step();  // always suceeds; might throw, though
        return this->dbPtr->getRowsAffected();
      }
      catch (SqliteOverlay::BusyException&) {
        return DbOperationError::Busy;
      }
      catch (SqliteOverlay::ConstraintFailedException&) {
        return DbOperationError::Constraint;
      }
      catch (...) {
        return DbOperationError::Other;
      }
    }

  protected:

    // Laufende Iteration mit  "Col = Val" und Nachfolgespalte
    template<class T, typename ...Args>
    void recursiveUpdateBuilder_langStep(std::string& s, int nextParaIdx, Col col, T&& val, Args ... args) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      s += Parent::colNameFromEnum(col) + "=?" + std::to_string(nextParaIdx) + ",";
      recursiveUpdateBuilder_langStep(s, nextParaIdx + 1, std::forward<Args>(args)...);
    }

    // Abschluss mit "Col = Val"
    template<class T>
    void recursiveUpdateBuilder_langStep(std::string& s, int nextParaIdx, Col col, T&& val) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");
      s += Parent::colNameFromEnum(col) + "=?" + std::to_string(nextParaIdx);
    }

    // Laufende Iteration mit  "Col = Val" und Nachfolgespalte
    template<class T, typename ...Args>
    void recursiveValueBinder(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, T&& val, Args ... args) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");

      stmt.bind(nextParaIdx, std::forward<T>(val));
      recursiveValueBinder(stmt, nextParaIdx + 1, std::forward<Args>(args)...);
    }

    // Abschluss mit "Col = Val"
    template<class T>
    void recursiveValueBinder(SqliteOverlay::SqlStatement& stmt, int nextParaIdx, Col col, T&& val) const {
      static_assert (!std::is_same_v<T, Col>, "fehler");

      stmt.bind(nextParaIdx, std::forward<T>(val));
    }

  private:
    const std::string sqlBaseDelete;
    const std::string sqlBaseDeleteById;
    const std::string sqlBaseUpdate;
    std::string sqlBaseInsert;
  };
}
