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

#ifndef SQLITE_OVERLAY_TRANSACTION_H
#define	SQLITE_OVERLAY_TRANSACTION_H

#include <memory>

#include "SqliteDatabase.h"

namespace SqliteOverlay
{

  /** \brief A class that wraps a database transaction into a C++ object with the benefit
   * that the transaction is automatically either commited or rolled back when the object's
   * dtor is called.
   */
  class Transaction
  {
  public:
    /** \brief The standard ctor for a new transaction; if it completes, the new transaction is active.
     *
     * See [here](https://www.sqlite.org/lang_transaction.html) for the different transaction types.
     *
     * \throws std::invalid_argument if the provided database pointer is a `nullptr`
     *
     * \throws BusyException if the DB was busy and the required lock could not be acquired
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    Transaction(
        const SqliteDatabase* _db,   ///< pointer to the database on which to create the new transaction
        TransactionType tt=TransactionType::Immediate,   ///< the type of transaction, see [here](https://www.sqlite.org/lang_transaction.html)
        TransactionDtorAction _dtorAct = TransactionDtorAction::Rollback   ///< what to do when the dtor is called
        );

    /** \brief Disabled copy ctor */
    Transaction(const Transaction& other) = delete;

    /** \brief Disabled copy assignment */
    Transaction& operator=(const Transaction& other) = delete;

    /** \brief Standard move ctor */
    Transaction(Transaction&& other);

    /** \brief Move assignment
     *
     * Test case: implicitly in Transaction tests
     *
     */
    Transaction& operator=(Transaction&& other);

    /** \brief Dtor; commits or rolls back the transaction if commit / rollback hasn't been called before
     *
     * If the transaction has already been finished (committed / rolled back), nothing happens in the dtor.
     *
     * \warning If a potential commit or rollback fails (because the DB was busy, for instance) then
     * an exception would occur in the dtor that is not handled by the dtor. Essentially, this means
     * that the programm is terminated immediately (dtor should never throw exception). So be sure to
     * properly call `commit()` or `rollback()` before the dtor and handle possible errors gracefully.
     *
     * Test case: yes
     *
     */
    ~Transaction();

    /** \returns `true` if the transaction hasn't been committed / rolled back yet
     *
     * Test case: yes
     *
     */
    bool isActive() const;

    /** \brief Commits a transaction and releases the lock on the database
     *
     * \throws BusyException if the DB was busy and the transaction could not be committed
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    void commit();

    /** \brief Undoes all changes of a transaction and releases the lock on the database
     *
     * \throws BusyException if the DB was busy and the transaction could not be rolled back
     *
     * \throws GenericSqliteException incl. error code if anything else goes wrong
     *
     * Test case: yes
     *
     */
    void rollback();

    /** \returns `true` if this is a nested transaction (a "sub-transaction") and `false` if it's not
     *
     * Test case: yes
     *
     */
    bool isNested() const;

  private:
    const SqliteDatabase* db;
    TransactionDtorAction dtorAct;
    string savepointName;
    bool isFinished;
  };

}
#endif	/* SQLITE_OVERLAY_TRANSACTION_H */

