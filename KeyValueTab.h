/*
 *    This is SqliteOverlay, a database abstraction layer on top of SQLite.
 *    Copyright (C) 2015  Volker Knollmann
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>                                     // for int64_t
#include <functional>                                   // for reference_wra...
#include <memory>                                       // for unique_ptr
#include <optional>                                     // for optional
#include <string>                                       // for string
#include <vector>                                       // for vector

#include <Sloppy/ConfigFileParser/ConstraintChecker.h>  // for ValueConstraint
#include <Sloppy/DateTime/DateAndTime.h>                // for WallClockTime...
#include <Sloppy/json.hpp>                              // for json

#include "DbTab.h"                                      // for DbTab
#include "SqlStatement.h"                               // for SqlStatement

namespace SqliteOverlay
{
  class SqliteDatabase;

  /** \brief A convience object that treats a given table as a key-value-store
   * and eases creating and retrieval of keys and values.
   *
   * It uses fixed, predefined column names "K" and "V".
   *
   * It supports optional caching of common database queries. So if you expect
   * a lot of queries, you should consider enabling the statement cache. But
   * remember that an enabled cache results in persistent, pending, unfinalized
   * statements that trigger errors when trying to close the underlying database.
   *
   * Caching is DISABLED by default.
   */
  class KeyValueTab
  {
  public:    
    static constexpr char KEY_COL_NAME[] = "K";
    static constexpr char VAL_COL_NAME[] = "V";

    /** \brief Creates a new Key-Value-Table instance from an existing
     * database table.
     *
     * \throws NoSuchTable if the table doesn't exist
     * \throws std::invalid_argument if the table doesn't have the required key/value columns
     */
    KeyValueTab (
        const SqliteDatabase& _db,   ///< the database that contains the table
        const std::string& _tabName   ///< the table name
        );

    /** Empty dtor */
    ~KeyValueTab() {}

    /** No copy construction because we own resources (prepared SQL statements) */
    KeyValueTab(const KeyValueTab& other) = delete;

    /** Standard move ctor */
    KeyValueTab(KeyValueTab&& other) = default;

    /** No copy assignment because we own resources (prepared SQL statements) */
    KeyValueTab& operator=(const KeyValueTab& other) = delete;

    /** Standard move assignment */
    KeyValueTab& operator=(KeyValueTab&& other) = default;

    /** \brief Assigns a value to a key; creates the key if it doesn't exist yet
     */
    template<typename T>
    void set(
        const std::string& key,   ///< the key's name
        const T& val   ///< the value to be assigned to the key
        )
    {
      if (hasKey(key))
      {
        prepValueUpdateStmt();
        valUpdateStatement->bind(1, val);
        valUpdateStatement->bind(2, key);
        valUpdateStatement->step();

        // delete the statement object if the user
        // has disabled the statement caching
        if (!cacheStatements) valUpdateStatement.reset();
      } else {
        prepValueInsertStmt();
        valInsertStatement->bind(1, key);
        valInsertStatement->bind(2, val);
        valInsertStatement->step();

        // delete the statement object if the user
        // has disabled the statement caching
        if (!cacheStatements) valInsertStatement.reset();
      }
    }

    /** \brief Retrieves a value from the table
     *
     * \throws NoDataException if the key doesn't exist
     *
     */
    template<typename T>
    T get(
        const std::string& key   ///< the key's name
        )
    {
      prepValueSelectStmt();
      valSelectStatement->bind(1, key);
      valSelectStatement->step();
      const auto result = valSelectStatement->get<T>(0);

      // delete the statement object if the user
      // has disabled the statement caching
      if (!cacheStatements) valSelectStatement.reset();

      return result;
    }

    /** \brief Retrieves a value from the table
     *
     * If the key doesn't exist, the output reference is empty. Otherwise
     * it contains the value for the provided key.
     *
     */
    template<typename T>
    std::optional<T> get2(
        const std::string& key   ///< the key's name
        )
    {
      prepValueSelectStmt();
      valSelectStatement->bind(1, key);
      valSelectStatement->step();

      const std::optional<T> result = (valSelectStatement->hasData()) ? valSelectStatement->get<T>(0) : std::optional<T>{};

      // delete the statement object if the user
      // has disabled the statement caching
      if (!cacheStatements) valSelectStatement.reset();

      return result;
    }

    /** \returns the value of a key as a string
     *
     * \throws NoDataException if the key doesn't exist
     */
    std::string operator[](const std::string& key) {
      return get<std::string>(key);
    }

    // boolean queries
    bool hasKey(const std::string& key) const;

    /** \brief Checks whether a key/value-pair satisfies a given constraint.
     *
     * Optionally, this method generated a human-readable error message for
     * displaying it to the user, e.g., on the console. The pointed-to string
     * will only be modified if the requested constraint is not met.
     *
     * \throws std::invalid_argument if the provided key name is empty
     *
     * \returns `true` if the key and its value satisfy the requested constraint.
     */
    bool checkConstraint(
        const std::string& keyName,    ///< the name of the key
        Sloppy::ValueConstraint c,     ///< the constraint to check
        std::string* errMsg = nullptr  ///< an optional pointer to a string for returning a human-readable error message
        );

    /** \returns the number of entries (which is: the number of keys) in the table
     */
    int size() const { return tab.length(); }

    /** \brief Removes an entry from the table
     *
     * Unless we throw an exception, it is guaranteed that no entry
     * with the given exists when we return from this call. Means:
     * we don't throw if the key didn't exist in the first place...
     */
    void remove(const std::string& key);

    /** \returns a list of all keys in the table
     */
    std::vector<std::string> allKeys() const;

    /** \brief Drops all stored statements and other links to the database so that
     * the database is no longer "BUSY" due to unfinished prepared statements.
     *
     * You can be safely continue to use the object after calling this function
     * at the (marginal?) cost that some query statements have to be re-created.
     *
     * \note Releasing the database does not automatically disable statement
     * caching. It only drops the currently pending, cached statements, that's all.
     * If caching is enabled and you continue to use the object, new statements
     * will be created and cached.
     */
    void releaseDatabase();

    /** \brief Enables or disables caching of common query statements; if caching
     * is enabled, the object keeps some prepared, unfinished SQL statements open
     * for faster look-ups.
     *
     * Disabling the caching implies the release of any pending statements just
     * like a call to `releaseDatabase` would do.
     *
     * \warning Enabling the statement caching prevents the closing of the
     * associated database connection. The reason is that dabase connections
     * fail with BUSY if there pending unfinished connections.
     *
     * \warning Since we internally store a reference to the database connection
     * you shouldn't use this object anymore after closing the associated database
     * connection. Keep that in mind if you use a rather long-living instance
     * of a KeyValueTab, e.g. in a GUI-widget.
     */
    void enableStatementCache(
        bool useCaching   ///< `true`: caching enabled, `false`: caching disabled
        );

  protected:
    /** \brief Either resets or creates/prepares a statement for a value select operation
     * and stores it in `valSelectStatement`.
     */
    void prepValueSelectStmt();

    /** \brief Either resets or creates/prepares a statement for a value update operation
     * and stores it in `valUpdateStatement`.
     */
    void prepValueUpdateStmt();

    /** \brief Either resets or creates/prepares a statement for a value insert operation
     * and stores it in `valInsertStatement`.
     */
    void prepValueInsertStmt();

  private:
    std::reference_wrapper<const SqliteDatabase> db;
    std::string tabName;
    DbTab tab;
    bool cacheStatements{false};
    std::unique_ptr<SqlStatement> valSelectStatement;
    std::unique_ptr<SqlStatement> valUpdateStatement;
    std::unique_ptr<SqlStatement> valInsertStatement;
  };  

  //----------------------------------------------------------------------------

}

