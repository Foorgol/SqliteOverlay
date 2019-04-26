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

#ifndef SQLITE_OVERLAY_KEYVALUETAB_H
#define	SQLITE_OVERLAY_KEYVALUETAB_H

#include <Sloppy/ConfigFileParser/ConstraintChecker.h>
#include <Sloppy/json_fwd.hpp>

#include "SqliteDatabase.h"
#include "DbTab.h"
#include "ClausesAndQueries.h"
#include "SqlStatement.h"

namespace SqliteOverlay
{
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
        const string& _tabName   ///< the table name
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
        const string& key,   ///< the key's name
        const T& val   ///< the value to be assigned to the key
        )
    {
      if (hasKey(key))
      {
        valUpdateStatement.reset(true);
        valUpdateStatement.bind(1, val);
        valUpdateStatement.bind(2, key);
        valUpdateStatement.step();
      } else {
        valInsertStatement.reset(true);
        valInsertStatement.bind(1, key);
        valInsertStatement.bind(2, val);
        valInsertStatement.step();
      }
    }

    /** \brief Retrieves a value from the table and assigns it to
     * a provided output reference.
     *
     * Works with "normal" types (e.g., `int`).
     *
     * \throws NoDataException if the key doesn't exist
     *
     */
    template<typename T>
    void get(
        const string& key,   ///< the key's name
        T& outVal   ///< reference to which the key's value will be assigned
        )
    {
      valSelectStatement.reset(true);
      valSelectStatement.bind(1, key);
      valSelectStatement.step();
      valSelectStatement.get(0, outVal);
    }

    /** \brief Retrieves a value from the table and assigns it to
     * a provided output reference.
     *
     * Works with "optional" types (e.g., `optional<int>`). if the
     * key doesn't exist, the output reference is empty. Otherwise
     * it contains the value for the provided key.
     *
     */
    template<typename T>
    void get(
        const string& key,   ///< the key's name
        optional<T>& outVal   ///< reference to which the key's value will be assigned
        )
    {
      valSelectStatement.reset(true);
      valSelectStatement.bind(1, key);
      valSelectStatement.step();
      if (!valSelectStatement.hasData())
      {
        outVal.reset();
        return;
      }
      valSelectStatement.get(0, outVal);
    }

    /** \returns the value of a key as a string
     *
     * \throws NoDataException if the key doesn't exist
     */
    string operator[](const string& key);

    /** \returns the value of a key as an integer
     *
     * \throws NoDataException if the key doesn't exist
     */
    int getInt(const string& key);

    /** \returns the value of a key as a long
     *
     * \throws NoDataException if the key doesn't exist
     */
    long getLong(const string& key);

    /** \returns the value of a key as a double
     *
     * \throws NoDataException if the key doesn't exist
     */
    double getDouble(const string& key);

    /** \returns the value of a key as a bool
     *
     * \note We simply convert to int and compare with 0. If the value is
     * 0, we return `false` and in all other cases `true`.
     *
     * \throws NoDataException if the key doesn't exist
     */
    double getBool(const string& key);

    /** \returns the value of a key as a UTCTimestamp
     *
     * \throws NoDataException if the key doesn't exist
     */
    UTCTimestamp getUTCTimestamp(const string& key);

    /** \returns the value of a key as a JSON object
     *
     * \throws NoDataException if the key doesn't exist
     *
     * \throws nlohmann::json::parse_error if the key didn't contain valid JSON data
     *
     */
    nlohmann::json getJson(const string& key);

    /** \returns the value of a key as an optional string that
     * is empty ("empty optional", not "empty string"!) if the key doesn't exist
     */
    optional<string> getString2(const string& key);

    /** \returns the value of a key as an optional integer that
     * is empty if the key doesn't exist
     */
    optional<int> getInt2(const string& key);

    /** \returns the value of a key as an optional long that
     * is empty if the key doesn't exist
     */
    optional<long> getLong2(const string& key);

    /** \returns the value of a key as an optional double that
     * is empty if the key doesn't exist
     */
    optional<double> getDouble2(const string& key);

    /** \returns the value of a key as an optional bool that
     * is empty if the key doesn't exist
     *
     * \note We simply convert to int and compare with 0. If the value is
     * 0, we return `false` and in all other cases `true`.
     */
    optional<bool> getBool2(const string& key);

    /** \returns the value of a key as an optional UTCTimestamp that
     * is empty if the key doesn't exist
     */
    optional<UTCTimestamp> getUTCTimestamp2(const string& key);

    /** \returns the value of a key as an optional JSON object that
     * is empty if the key doesn't exist
     */
    optional<nlohmann::json> getJson2(const string& key);

    // boolean queries
    bool hasKey(const string& key) const;

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
        const string& keyName,    ///< the name of the key
        Sloppy::ValueConstraint c,     ///< the constraint to check
        string* errMsg = nullptr  ///< an optional pointer to a string for returning a human-readable error message
        );

    /** \returns the number of entries (which is: the number of keys) in the table
     */
    size_t size() const { return tab.length(); };

    /** \brief Removes an entry from the table
     *
     * Unless we throw an exception, it is guaranteed that no entry
     * with the given exists when we return from this call. Means:
     * we don't throw if the key didn't exist in the first place...
     */
    void remove(const string& key);

    /** \returns a list of all keys in the table
     */
    vector<string> allKeys() const;

  private:
    reference_wrapper<const SqliteDatabase> db;
    string tabName;
    DbTab tab;
    SqlStatement valSelectStatement;
    SqlStatement valUpdateStatement;
    SqlStatement valInsertStatement;
  };  

  //----------------------------------------------------------------------------

}
#endif	/* KEYVALUETAB_H */

