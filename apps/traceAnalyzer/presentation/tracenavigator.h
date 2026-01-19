/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 *    Derek Christ
 */

#ifndef TRACENAVIGATOR_H
#define TRACENAVIGATOR_H
#include "businessObjects/generalinfo.h"
#include "businessObjects/transaction.h"
#include "data/tracedb.h"
#include "memory"
#include <QObject>
#include <vector>

class CommentModel;

/*  Class to navigate through a tracefile
 *
 *
 */

class TraceNavigator : public QObject
{
    Q_OBJECT

public:
    TraceNavigator(QString path, CommentModel* commentModel, QObject* parent = 0);

    traceTime CurrentTraceTime() const { return currentTraceTime; }
    TraceDB& TraceFile() { return traceFile; }
    const GeneralInfo& GeneralTraceInfo() { return traceFile.getGeneralInfo(); }

    void navigateToTime(traceTime time);
    void navigateToTransaction(ID id);

    /* Transaction selection
     * (selecting a single transactions also navigates to that transaction)
     */
    void selectTransaction(ID id);
    void selectTransaction(const std::shared_ptr<Transaction>& transaction);
    void selectNextTransaction();
    void selectPreviousTransaction();
    void selectLastTransaction();
    void selectFirstTransaction();
    void selectNextRefresh(traceTime time);
    void selectNextActivate(traceTime time);
    void selectNextPrecharge(traceTime time);
    void selectNextCommand(traceTime time);
    //     void selectNextActb();
    //     void selectNextPreb();
    //     void selectNextRefb();

    void addSelectedTransactions(const std::vector<std::shared_ptr<Transaction>>& transactions);
    const std::vector<std::shared_ptr<Transaction>>& SelectedTransactions()
    {
        return selectedTransactions;
    }
    void addSelectedTransaction(const std::shared_ptr<Transaction>& Transaction);
    void addSelectedTransaction(ID id);
    void clearSelectedTransactions();
    bool hasSelectedTransactions();
    Timespan getSpanCoveredBySelectedTransaction();

    bool transactionIsSelected(ID id) const;
    bool transactionIsSelected(const std::shared_ptr<Transaction>& Transaction) const;

    void commitChangesToDB();
    void refreshData();

    const CommentModel* getCommentModel() const;

    bool existChangesToCommit() const;

Q_SIGNALS:
    void currentTraceTimeChanged();
    void selectedTransactionsChanged();

public Q_SLOTS:
    void traceFileModified();

private:
    TraceDB traceFile;

    // represents the current position in the tracefile
    // components drawing the tracefile center around that time
    traceTime currentTraceTime = 0;
    std::vector<std::shared_ptr<Transaction>> selectedTransactions;

    CommentModel* commentModel;

    void getCommentsFromDB();
    bool changesToCommitExist;
};

#endif // TRACENAVIGATOR_H
