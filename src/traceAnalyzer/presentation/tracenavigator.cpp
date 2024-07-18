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
 *    Iron Prando da Silva
 */

#include "tracenavigator.h"
#include "businessObjects/commentmodel.h"
#include "vector"
#include <QInputDialog>
#include <QLineEdit>

using namespace std;

TraceNavigator::TraceNavigator(QString path, CommentModel* commentModel, QObject* parent) :
    QObject(parent),
    traceFile(path, true),
    commentModel(commentModel),
    changesToCommitExist(false)
{
    getCommentsFromDB();

    QObject::connect(commentModel,
                     &CommentModel::gotoCommentTriggered,
                     this,
                     [=](const QModelIndex& index)
                     { navigateToTime(commentModel->getTimeFromIndex(index)); });

    QObject::connect(
        commentModel, &CommentModel::dataChanged, this, &TraceNavigator::traceFileModified);
    QObject::connect(
        commentModel, &CommentModel::rowsRemoved, this, &TraceNavigator::traceFileModified);

    Transaction::setNumTransactions(GeneralTraceInfo().numberOfTransactions);
}

/* Navigation
 *
 *
 */

void TraceNavigator::navigateToTime(traceTime time)
{
    if (time < 0)
        time = 0;
    else if (time > traceFile.getGeneralInfo().span.End())
        time = traceFile.getGeneralInfo().span.End();
    else
    {
        currentTraceTime = time;
        Q_EMIT currentTraceTimeChanged();
    }
}

void TraceNavigator::navigateToTransaction(ID id)
{
    navigateToTime(traceFile.getTransactionByID(id)->span.Begin());
}

/* DB
 *
 */

void TraceNavigator::commitChangesToDB()
{
    traceFile.updateComments(commentModel->getComments());
    changesToCommitExist = false;
}

void TraceNavigator::getCommentsFromDB()
{
    for (const auto& comment : traceFile.getComments())
        commentModel->addComment(comment.time, comment.text);
}

void TraceNavigator::refreshData()
{
    traceFile.refreshData();
    clearSelectedTransactions();
    navigateToTime(currentTraceTime);
}

/* Transaction Selection
 *
 *
 */

void TraceNavigator::addSelectedTransactions(const vector<shared_ptr<Transaction>>& transactions)
{
    for (const auto &transaction : transactions)
    {
        selectedTransactions.push_back(transaction);
    }
    Q_EMIT selectedTransactionsChanged();
}

void TraceNavigator::addSelectedTransaction(const shared_ptr<Transaction>& transaction)
{
    selectedTransactions.push_back(transaction);
    Q_EMIT selectedTransactionsChanged();
}

void TraceNavigator::addSelectedTransaction(ID id)
{
    shared_ptr<Transaction> transaction = TraceFile().getTransactionByID(id);
    selectedTransactions.push_back(transaction);
    Q_EMIT selectedTransactionsChanged();
}

void TraceNavigator::selectTransaction(ID id)
{
    clearSelectedTransactions();
    addSelectedTransaction(id);
    navigateToTransaction(id);
}

void TraceNavigator::selectTransaction(const shared_ptr<Transaction>& transaction)
{
    selectTransaction(transaction->id);
}

void TraceNavigator::selectNextTransaction()
{
    if (selectedTransactions.empty() ||
        selectedTransactions.front()->id == traceFile.getGeneralInfo().numberOfTransactions)
        selectFirstTransaction();
    else
        selectTransaction(selectedTransactions.front()->id + 1);
}

void TraceNavigator::selectPreviousTransaction()
{
    if (selectedTransactions.empty() || selectedTransactions.front()->id == 1)
        selectLastTransaction();
    else
        selectTransaction(selectedTransactions.front()->id - 1);
}

void TraceNavigator::selectFirstTransaction()
{
    selectTransaction(1);
}

void TraceNavigator::selectLastTransaction()
{
    selectTransaction(traceFile.getGeneralInfo().numberOfTransactions);
}

void TraceNavigator::selectNextRefresh(traceTime time)
{
    shared_ptr<Transaction> nextRefresh;

    nextRefresh = traceFile.getNextRefresh(time);

    if (nextRefresh)
        selectTransaction(nextRefresh);
}

void TraceNavigator::selectNextActivate(traceTime time)
{
    shared_ptr<Transaction> nextActivate;

    nextActivate = traceFile.getNextActivate(time);

    if (nextActivate)
        selectTransaction(nextActivate);
}

void TraceNavigator::selectNextPrecharge(traceTime time)
{
    shared_ptr<Transaction> nextPrecharge;

    nextPrecharge = traceFile.getNextPrecharge(time);

    if (nextPrecharge)
        selectTransaction(nextPrecharge);
}

void TraceNavigator::selectNextCommand(traceTime time)
{
    shared_ptr<Transaction> nextCommand;

    nextCommand = traceFile.getNextCommand(time);

    if (nextCommand)
        selectTransaction(nextCommand);
}

// void TraceNavigator::selectNextActb()
// {
//     shared_ptr<Transaction> nextActb;
//
//     if (!SelectedTransactions().empty())
//         nextActb = traceFile.getNextActb(SelectedTransactions().front()->id);
//     else
//         nextActb = traceFile.getNextActb(0);
//
//     if (nextActb)
//         selectTransaction(nextActb);
// }
//
// void TraceNavigator::selectNextPreb()
// {
//     shared_ptr<Transaction> nextPreb;
//
//     if (!SelectedTransactions().empty())
//         nextPreb = traceFile.getNextPreb(
//                        SelectedTransactions().front()->id);
//     else
//         nextPreb = traceFile.getNextPreb(0);
//
//     if (nextPreb)
//         selectTransaction(nextPreb);
// }
//
// void TraceNavigator::selectNextRefb()
// {
//     shared_ptr<Transaction> n;
//
//     if (!SelectedTransactions().empty())
//         n = traceFile.getNextRefb(SelectedTransactions().front()->id);
//     else
//         n = traceFile.getNextRefb(0);
//
//     if (n)
//         selectTransaction(n);
// }

bool TraceNavigator::transactionIsSelected(const shared_ptr<Transaction>& transaction) const
{
    return transactionIsSelected(transaction->id);
}

bool TraceNavigator::transactionIsSelected(ID id) const
{
    for (const auto& transaction : selectedTransactions)
    {
        if (transaction->id == id)
            return true;
    }
    return false;
}

void TraceNavigator::clearSelectedTransactions()
{
    if (hasSelectedTransactions())
    {
        selectedTransactions.clear();
        Q_EMIT selectedTransactionsChanged();
    }
}

bool TraceNavigator::hasSelectedTransactions()
{
    return !selectedTransactions.empty();
}

Timespan TraceNavigator::getSpanCoveredBySelectedTransaction()
{
    if (!hasSelectedTransactions())
        return Timespan(0, 0);

    traceTime begin = SelectedTransactions().at(0)->span.Begin();
    traceTime end = SelectedTransactions().at(0)->span.End();

    for (const auto& transaction : selectedTransactions)
    {
        if (transaction->span.End() > end)
            end = transaction->span.End();
    }

    return Timespan(begin, end);
}

const CommentModel* TraceNavigator::getCommentModel() const
{
    return commentModel;
}

bool TraceNavigator::existChangesToCommit() const
{
    return changesToCommitExist;
}

void TraceNavigator::traceFileModified()
{
    changesToCommitExist = true;
}
