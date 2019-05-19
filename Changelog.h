#ifndef SQLITE_OVERLAY_CHANGELOG_H
#define SQLITE_OVERLAY_CHANGELOG_H

#include <vector>
#include <mutex>

#include <sqlite3.h>

namespace SqliteOverlay
{
  /** \brief An enum that maps between SQLite constants for row modifications
   * to enum values.
   */
  enum class RowChangeAction
  {
    Insert = SQLITE_INSERT,
    Update = SQLITE_UPDATE,
    Delete = SQLITE_DELETE
  };

  //----------------------------------------------------------------------------

  /** \brief A struct that encapsulates change information that SQLite
   * optionally provides via a callback function whenever the database
   * is modified.
   */
  struct ChangeLogEntry
  {
    RowChangeAction action;  ///< the kind of change (insert, update, delete)
    std::string dbName;   ///< the name of the affected database, most likely "main"
    std::string tabName;   ///< the name of the affected table
    size_t rowId;   ///< the rowid of the affected row

    /** \brief Convenience ctor from basic data types
     */
    ChangeLogEntry(
        int a,   ///< the numeric change ID as defined in sqlite3.h
        const std::string& dn,   ///< the affected database name
        const std::string& tn,   ///< the affected table name
        size_t id   ///< the affected table row
        )
      :action{static_cast<RowChangeAction>(a)}, dbName{dn}, tabName{tn}, rowId{id} {}

    /** \brief Convenience ctor from (almost all) basic data types
     */
    ChangeLogEntry(
        RowChangeAction a,   ///< the kind of modification
        const std::string& dn,   ///< the affected database name
        const std::string& tn,   ///< the affected table name
        size_t id   ///< the affected table row
        )
      :action{a}, dbName{dn}, tabName{tn}, rowId{id} {}

    //
    // special members
    //
/*
    ChangeLogEntry() = default;
    ChangeLogEntry(const ChangeLogEntry& other) = default;
    ChangeLogEntry(ChangeLogEntry&& other) = default;
    ChangeLogEntry& operator=(const ChangeLogEntry& other) = default;
    ChangeLogEntry& operator=(ChangeLogEntry&& other) = default;
    */
  };
  using ChangeLogList = std::vector<ChangeLogEntry>;

  //----------------------------------------------------------------------------

  /** \brief A struct that is used by the built-in `changeLogCallback`
   * to get access to the internal change log.
   */
  struct ChangeLogCallbackContext
  {
    std::mutex* logMutex;   ///< pointer to a mutex that must be acquired before accessing the changelog
    ChangeLogList* logPtr;   ///< pointer to the actual changelog
  };

  //----------------------------------------------------------------------------

  /** \brief A built-in callback function for storing all database modifications
   * in a changelog.
   *
   * The function signature is defined by SQLite
   */
  void changeLogCallback(
      void* customPtr,   ///< a custom pointer that can be set during callback setup
      int modType,   ///< the kind of modification as per sqlite3.h
      char const * _dbName,   ///< the affected database (e.g., "main")
      char const* _tabName,   ///< the affected table
      sqlite3_int64 id   /// the affected row
      );

  //----------------------------------------------------------------------------

}

#endif
