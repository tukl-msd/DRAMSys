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

    std::shared_ptr<Transaction> getTransactionByID(ID id);
    std::vector<unsigned> getDistinctChannels();
    unsigned getCurrentChannel() const { return currentChannel; }
    void setCurrentChannel(unsigned channel) { currentChannel = channel; }

    // Returns the transaction ID directly after (forward == true) or before
    // (forward == false) the given ID on the given channel.
    // Returns 0 if there is no such transaction.
    ID getAdjacentTransactionID(unsigned channel, ID id, bool forward);

    std::vector<CommentModel::Comment> getComments();

    QSqlDatabase getDatabase() const;

private:
    unsigned currentChannel = 0;

    QString pathToDB;
    QSqlDatabase database;
    GeneralInfo generalInfo;
    std::unordered_map<std::string, double> commandLengths;

    QSqlQuery selectTransactionsByTimespan;
    QSqlQuery selectTransactionById;

    QString transactionQueryHead = R"(
SELECT
    t.id,
	t.address,
    t.data_length,
	n.signal_id,
    t.thread,
    t.channel,
	t.tlm_command,

    MAX(n.timestamp) FILTER (WHERE p.stage='REQ' AND p.type='BEGIN') AS req_begin,
    MAX(n.timestamp) FILTER (WHERE p.stage='REQ' AND p.type='END') AS req_end,
    MAX(n.timestamp) FILTER (WHERE p.stage='RESP' AND p.type='BEGIN') AS resp_begin,
    MAX(n.timestamp) FILTER (WHERE p.stage='RESP' AND p.type='END') AS resp_end

FROM nb_transports AS n
JOIN phases AS p
    ON n.phase_id = p.id
JOIN transactions as t
    ON n.transaction_id = t.id
)";

    // The caller appends a list of transaction IDs after "IN", e.g. "(1, 2, 3)".
    QString phaseQuery = R"(
SELECT
    b.transaction_id,
	pb.stage AS PhaseName,
    b.timestamp AS PhaseBegin,
    e.timestamp AS PhaseEnd
FROM nb_transports AS b
JOIN nb_transports AS e
    ON e.transaction_id = b.transaction_id
JOIN phases AS pb
    ON b.phase_id = pb.id
JOIN phases AS pe
    ON e.phase_id = pe.id
   AND pe.stage = pb.stage
   AND pe.type = 'END'
WHERE
    pb.type = 'BEGIN'
AND
    b.transaction_id IN
)";

    // The caller appends a list of transaction IDs after "IN", e.g. "(1, 2, 3)".
    QString commandPhaseQuery = R"(
SELECT
    cs.transaction_id,
	c.name AS PhaseName,
	state.begin as DramStateBegin,
	state.end as DramStateEnd,
	ds.begin AS DataStrobeBegin,
	ds.end AS DataStrobeEnd,
	cs.rank,
	cs.bank_group,
	cs.bank,
	cs.row,
	cs.column,
	ds.burst_length
FROM command_strobes AS cs
JOIN commands AS c
    ON cs.command_id == c.id
JOIN data_strobes as ds
    ON ds.command_strobe_id == cs.id
JOIN dram_states as state
    ON cs.id == state.command_strobe_id
WHERE cs.transaction_id IN
)";

    void prepareQueries();
    void prepareTables();
    void executeQuery(QSqlQuery query);
    std::shared_ptr<Transaction> parseTransactionFromQuery(QSqlQuery& query);
    std::vector<std::shared_ptr<Transaction>> parseTransactionsFromQuery(QSqlQuery& query);
    static std::vector<CommentModel::Comment> parseCommentsFromQuery(QSqlQuery& query);

    uint64_t getTraceLength();
    uint64_t getNumberOfTransactions();
    GeneralInfo getGeneralInfoFromDB();
    std::unordered_map<std::string, double> getCommandLengthsFromDB();
    QVariant getParameterFromGeneralInfo(const std::string& parameter);
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
