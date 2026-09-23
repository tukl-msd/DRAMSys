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

#ifndef TRACEDB_H
#define TRACEDB_H

#include "businessObjects/commentmodel.h"
#include "businessObjects/generalinfo.h"
#include "businessObjects/phases/phasefactory.h"
#include "businessObjects/transaction.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QString>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

/*  TraceDB handles the connection to an SQLite database containing trace data.
 *  A TraceDB object always holds an open connection to a valid database.
 */

class TraceDB : public QObject
{
    Q_OBJECT

public:
    TraceDB(QString path);

    void updateComments(const std::vector<CommentModel::Comment>& comments);
    void refreshData();

    const GeneralInfo& getGeneralInfo() const { return generalInfo; }
    const std::unordered_map<std::string, double>& getCommandLengths() const
    {
        return commandLengths;
    }

    std::vector<std::shared_ptr<Transaction>>
    getTransactionsWithCustomQuery(const QString& queryText);
    std::vector<std::shared_ptr<Transaction>> getTransactionsInTimespan(const Timespan& span);
    std::shared_ptr<Transaction> getNextPrecharge(traceTime time);
    std::shared_ptr<Transaction> getNextActivate(traceTime time);
    std::shared_ptr<Transaction> getNextRefresh(traceTime time);
    std::shared_ptr<Transaction> getNextCommand(traceTime time);

    std::shared_ptr<Transaction> getTransactionByID(ID id);

    std::vector<CommentModel::Comment> getComments();

    QSqlDatabase getDatabase() const;

private:
    QString pathToDB;
    QSqlDatabase database;
    GeneralInfo generalInfo;
    std::unordered_map<std::string, double> commandLengths;

    QSqlQuery selectTransactionsByTimespan;
    QSqlQuery selectTransactionById;

    QString queryHead = R"(
SELECT Transactions.ID AS TransactionID,
       Ranges.begin,
       Ranges.end,
       Address,
       DataLength,
       Thread,
       Channel,
       Command,
       Phases.ID AS PhaseID,
       PhaseName,
       PhaseBegin,
       PhaseEnd,
       DataStrobeBegin,
       DataStrobeEnd,
       Rank,
       BankGroup,
       Bank,
       Row,
       Column,
       BurstLength,
       DualBank
FROM Transactions
INNER JOIN Phases ON Phases.Transact = Transactions.ID
INNER JOIN Ranges ON Transactions.Range = Ranges.ID
    )";

    void prepareQueries();
    void executeQuery(QSqlQuery query);
    std::shared_ptr<Transaction> parseTransactionFromQuery(QSqlQuery& query);
    std::vector<std::shared_ptr<Transaction>> parseTransactionsFromQuery(QSqlQuery& query);
    static std::vector<CommentModel::Comment> parseCommentsFromQuery(QSqlQuery& query);

    uint64_t getTraceLength();
    uint64_t getNumberOfTransactions();
    GeneralInfo getGeneralInfoFromDB();
    std::unordered_map<std::string, double> getCommandLengthsFromDB();
    QVariant getParameterFromTable(const std::string& parameter, const std::string& table);
};

class sqlException : public std::exception
{
private:
    std::string message;

public:
    sqlException(std::string message, std::string filename)
    {
        this->message = std::string("Error in file ") + filename + std::string(" ") + message;
    }
    const char* what() const noexcept override { return message.c_str(); }
};
#endif // TRACEDB_H
