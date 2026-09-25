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

#include "data/tracedb.h"
#include "businessObjects/phases/phasefactory.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlRecord>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <filesystem>
#include <unordered_map>
#include <utility>

TraceDB::TraceDB(QString path) : pathToDB(std::move(path))
{
    if (!std::filesystem::exists(pathToDB.toStdString()))
        throw std::runtime_error("Database file does not exist!");

    database = QSqlDatabase::database(pathToDB);
    if (database.isValid() && database.isOpen())
    {
        // Close the database connection if it exists and was not closed yet.
        database.close();
        QSqlDatabase::removeDatabase(pathToDB);
    }

    database = QSqlDatabase::addDatabase("QSQLITE", pathToDB);
    database.setDatabaseName(pathToDB);
    if (!database.open())
    {
        qDebug() << database.lastError().text();
    }

    prepareQueries();
    prepareTables();
    generalInfo = getGeneralInfoFromDB();
    commandLengths = getCommandLengthsFromDB();
}

void TraceDB::prepareQueries()
{
    selectTransactionsByTimespan = QSqlQuery(database);
    if (!selectTransactionsByTimespan.prepare(transactionQueryHeadByTimespan))
        qDebug() << database.lastError().text();

    selectTransactionById = QSqlQuery(database);
    if (!selectTransactionById.prepare(transactionQueryHead + " WHERE t.id = :id GROUP BY t.id"))
        qDebug() << database.lastError().text();

    commandPhasesReal = commandPhaseQuerySelect + " WHERE cs.transaction_id IN ";
    commandPhasesInternal =
        commandPhaseQuerySelect + " WHERE cs.transaction_id IS NULL AND cs.id IN ";
}

void TraceDB::prepareTables()
{
    QSqlQuery query(database);
    query.prepare(R"(
        CREATE TABLE IF NOT EXISTS Comments(
                Time INTEGER,
                Text TEXT
        );)");
    executeQuery(query);

    query.prepare(R"(
CREATE INDEX IF NOT EXISTS idx_phases_lookup ON phases(id, stage, type);
)");
    executeQuery(query);

    query.prepare(R"(
CREATE INDEX IF NOT EXISTS idx_nbt_txn_signal ON nb_transports(transaction_id, signal_id, phase_id, timestamp);
)");
    executeQuery(query);

    // Covering index for the time-range scan used by the main transaction query.
    query.prepare(R"(
CREATE INDEX IF NOT EXISTS idx_nbt_ts ON nb_transports(timestamp, transaction_id, phase_id, signal_id);
)");
    executeQuery(query);

    // Lookups per transaction ID in the strobe/state tables.
    query.prepare(R"(
CREATE INDEX IF NOT EXISTS idx_cs_txn ON command_strobes(transaction_id);
)");
    executeQuery(query);

    query.prepare(R"(
CREATE INDEX IF NOT EXISTS idx_ds_cs ON data_strobes(command_strobe_id);
)");
    executeQuery(query);

    query.prepare(R"(
CREATE INDEX IF NOT EXISTS idx_state_strobe ON dram_states(command_strobe_id);
)");
    executeQuery(query);
}

std::vector<unsigned> TraceDB::getDistinctChannels()
{
    // A channel is present for every bus recorded in the signals table.
    QSqlQuery query(database);
    query.prepare("SELECT DISTINCT channel FROM signals ORDER BY channel");
    executeQuery(query);

    std::vector<unsigned> channels;
    while (query.next())
        channels.push_back(query.value(0).toUInt());
    return channels;
}

ID TraceDB::getAdjacentTransactionID(unsigned channel, ID id, bool forward)
{
    // A transaction's channel is the channel of the TLM bus it was seen on,
    // so the channel is resolved via the nb_transports/signal tables.
    QSqlQuery query(database);
    query.prepare(QString("SELECT DISTINCT t.id FROM transactions AS t"
                          " JOIN nb_transports AS n ON n.transaction_id = t.id"
                          " JOIN signals AS s ON s.id = n.signal_id"
                          " WHERE s.channel = :channel AND t.id %1 :id"
                          " ORDER BY t.id %2"
                          " LIMIT 1")
                      .arg(forward ? ">" : "<")
                      .arg(forward ? "ASC" : "DESC"));
    query.bindValue(":channel", channel);
    query.bindValue(":id", id);
    executeQuery(query);

    if (query.next())
        return query.value(0).toUInt();
    return 0;
}

void TraceDB::updateComments(const std::vector<CommentModel::Comment>& comments)
{
    QSqlQuery query(database);
    query.prepare("DELETE FROM Comments");
    executeQuery(query);
    query.prepare("insert into Comments values(:time,:text)");

    for (const auto& comment : comments)
    {
        query.bindValue(":time", comment.time);
        query.bindValue(":text", comment.text);
        executeQuery(query);
    }
}

void TraceDB::refreshData()
{
    prepareQueries();
    generalInfo = getGeneralInfoFromDB();
}

std::vector<std::shared_ptr<Transaction>> TraceDB::getTransactionsInTimespan(const Timespan& span)
{
    selectTransactionsByTimespan.bindValue(":begin", span.Begin());
    selectTransactionsByTimespan.bindValue(":end", span.End());
    selectTransactionsByTimespan.bindValue(":channel", currentChannel);
    executeQuery(selectTransactionsByTimespan);
    return parseTransactionsFromQuery(selectTransactionsByTimespan);
}

// TODO Remove exception
std::shared_ptr<Transaction> TraceDB::getTransactionByID(ID id)
{
    selectTransactionById.bindValue(":id", id);
    executeQuery(selectTransactionById);
    auto result = parseTransactionsFromQuery(selectTransactionById);
    if (!result.empty())
        return result[0];

    throw sqlException(("Transaction with ID " + QString::number(id) + " not in DB").toStdString(),
                       this->pathToDB.toStdString());
}

GeneralInfo TraceDB::getGeneralInfoFromDB()
{
    QVariant parameter;

    parameter = getParameterFromGeneralInfo("NumberOfRanks");
    unsigned numberOfRanks = parameter.isValid() ? parameter.toUInt() : 1;

    parameter = getParameterFromGeneralInfo("NumberOfBankgroups");
    unsigned numberOfBankGroups = parameter.isValid() ? parameter.toUInt() : 4;

    parameter = getParameterFromGeneralInfo("NumberOfBanks");
    unsigned numberOfBanks = parameter.isValid() ? parameter.toUInt() : 16;

    parameter = getParameterFromGeneralInfo("Clk");
    uint64_t clkPeriod = parameter.isValid() ? parameter.toULongLong() : 1072;

    parameter = getParameterFromGeneralInfo("UnitOfTime");
    QString unitOfTime = parameter.isValid() ? parameter.toString() : "PS";

    parameter = getParameterFromGeneralInfo("Per2BankOffset");
    unsigned per2BankOffset = parameter.isValid() ? parameter.toUInt() : 1;

    parameter = getParameterFromGeneralInfo("RowColumnCommandBus");
    bool rowColumnCommandBus = parameter.isValid() && parameter.toBool();

    parameter = getParameterFromGeneralInfo("PseudoChannelMode");
    bool pseudoChannelMode = parameter.isValid() && parameter.toBool();

    uint64_t numberOfTransactions = getNumberOfTransactions();
    auto traceEnd = static_cast<traceTime>(getTraceLength());

    return {numberOfTransactions,
            Timespan(0, traceEnd),
            numberOfRanks,
            numberOfBankGroups,
            numberOfBanks,
            unitOfTime,
            clkPeriod,
            per2BankOffset,
            rowColumnCommandBus,
            pseudoChannelMode};
}

std::unordered_map<std::string, double> TraceDB::getCommandLengthsFromDB()
{
    auto getLengthFromDb = [this](const std::string& command) -> QVariant
    {
        QSqlQuery query(("SELECT length FROM commands WHERE name = \"" + command + "\"").c_str(),
                        database);

        if (query.first())
            return query.value(0);

        return {};
    };

    auto getCommandLength = [=](const std::string& command) -> double
    {
        QVariant length = getLengthFromDb(command);

        if (length.isValid())
            return length.toDouble();

        qDebug() << "CommandLength for" << command.c_str()
                 << "not present in database. Defaulting to 1.";
        return 1;
    };

    std::unordered_map<std::string, double> commandLengths;
    commandLengths["NOP"] = getCommandLength("NOP");
    commandLengths["RD"] = getCommandLength("RD");
    commandLengths["WR"] = getCommandLength("WR");
    commandLengths["MWR"] = getCommandLength("MWR");
    commandLengths["RDA"] = getCommandLength("RDA");
    commandLengths["WRA"] = getCommandLength("WRA");
    commandLengths["MWRA"] = getCommandLength("MWRA");
    commandLengths["ACT"] = getCommandLength("ACT");
    commandLengths["PREPB"] = getCommandLength("PREPB");
    commandLengths["REFPB"] = getCommandLength("REFPB");
    commandLengths["RFMPB"] = getCommandLength("RFMPB");
    commandLengths["REFP2B"] = getCommandLength("REFP2B");
    commandLengths["RFMP2B"] = getCommandLength("RFMP2B");
    commandLengths["REFDB"] = getCommandLength("REFDB");
    commandLengths["RFMDB"] = getCommandLength("RFMDB");
    commandLengths["PRESB"] = getCommandLength("PRESB");
    commandLengths["REFSB"] = getCommandLength("REFSB");
    commandLengths["RFMSB"] = getCommandLength("RFMSB");
    commandLengths["PREAB"] = getCommandLength("PREAB");
    commandLengths["REFAB"] = getCommandLength("REFAB");
    commandLengths["RFMAB"] = getCommandLength("RFMAB");
    commandLengths["PDEA"] = getCommandLength("PDEA");
    commandLengths["PDXA"] = getCommandLength("PDXA");
    commandLengths["PDEP"] = getCommandLength("PDEP");
    commandLengths["PDXP"] = getCommandLength("PDXP");
    commandLengths["SREFEN"] = getCommandLength("SREFEN");
    commandLengths["SREFEX"] = getCommandLength("SREFEX");
    return commandLengths;
}

QVariant TraceDB::getParameterFromGeneralInfo(const std::string& parameter)
{
    QSqlQuery query(("SELECT value FROM general_info WHERE key = \"" + parameter + "\"").c_str(),
                    database);
    if (query.first())
        return query.value(0);

    qDebug() << "Parameter " << parameter.c_str() << " not present in generalInfo table";
    return {};
}

uint64_t TraceDB::getTraceLength()
{
    QSqlQuery query(database);
    query.prepare("SELECT MAX(timestamp) FROM nb_transports");
    executeQuery(query);

    query.next();
    return query.value(0).toULongLong();
}

uint64_t TraceDB::getNumberOfTransactions()
{
    QSqlQuery query(database);
    query.prepare("SELECT COUNT(ID) FROM transactions");
    executeQuery(query);

    query.next();
    return query.value(0).toULongLong();
}

std::vector<CommentModel::Comment> TraceDB::getComments()
{
    QSqlQuery query(database);
    query.prepare("SELECT Time,Text From Comments");
    executeQuery(query);
    return parseCommentsFromQuery(query);
}

QSqlDatabase TraceDB::getDatabase() const
{
    return database;
}

/*  Helpers
 *
 *
 *
 */

QString joinIds(const std::unordered_map<ID, std::shared_ptr<Transaction>>& transactions)
{
    QStringList ids;
    for (const auto& pair : transactions)
        ids << QString::number(pair.first);
    return ids.join(", ");
}

std::vector<std::shared_ptr<Transaction>> TraceDB::parseTransactionsFromQuery(QSqlQuery& query)
{
    std::vector<std::shared_ptr<Transaction>> result;

    // Internal commands (refresh, power-down, ...) are listed with their command_strobe
    // ID and flagged via the is_internal column; their phase rows are looked up by
    // that ID instead of a transaction ID.
    const int isInternalCol = query.record().indexOf("is_internal");
    std::unordered_map<ID, std::shared_ptr<Transaction>> realByID;
    std::unordered_map<ID, std::shared_ptr<Transaction>> internalByID;

    while (query.next())
    {
        ID id = query.value(0).toUInt();
        Timespan span(query.value(7).toLongLong(), query.value(10).toLongLong());
        uint64_t address = query.value(1).toULongLong();
        unsigned int dataLength = query.value(2).toUInt();
        unsigned int thread = query.value(4).toUInt();
        unsigned int channel = query.value(5).toUInt();
        QString command = (query.value(6).toUInt() == 1) ? "R" : "W"; // 1 == TLM_READ_COMMAND

        auto transaction = std::make_shared<Transaction>(id,
                                                         std::move(command),
                                                         address,
                                                         dataLength,
                                                         thread,
                                                         channel,
                                                         span,
                                                         generalInfo.clkPeriod);
        result.push_back(transaction);

        const bool isInternal = isInternalCol >= 0 && query.value(isInternalCol).toUInt() == 1;
        if (isInternal)
        {
            // Internal entries carry the high bit (see the list query). The phase
            // lookup uses the raw command_strobe ID, so key the map by that.
            internalByID[id & 0x7FFFFFFFu] = transaction;
        }
        else
        {
            realByID[id] = transaction;
        }
    }

    if (result.empty())
        return result;

    // REQ/RESP information: real transactions only (internal commands have none).
    if (!realByID.empty())
    {
        QSqlQuery phasesQuery(database);
        phasesQuery.prepare(phaseQuery + "(" + joinIds(realByID) + ")");
        executeQuery(phasesQuery);

        while (phasesQuery.next())
        {
            auto it = realByID.find(phasesQuery.value(0).toUInt());
            if (it == realByID.end())
                continue;

            auto& transaction = it->second;

            QString phaseName = phasesQuery.value(1).toString();
            Timespan span(phasesQuery.value(2).toLongLong(), phasesQuery.value(3).toLongLong());
            Timespan spanOnDataStrobe(0, 0);
            unsigned int rank = 0;
            unsigned int bankGroup = 0;
            unsigned int bank = 0;
            unsigned int row = 0;
            unsigned int column = 0;
            unsigned int burstLength = 0;
            unsigned int dualBank = 0;
            auto phase = PhaseFactory::createPhase(phaseName,
                                                   span,
                                                   spanOnDataStrobe,
                                                   rank,
                                                   bankGroup,
                                                   bank,
                                                   row,
                                                   column,
                                                   burstLength,
                                                   dualBank,
                                                   transaction,
                                                   *this);
            transaction->addPhase(phase);
        }
    }

    // Look up the command phase information of all entries with one batch query each
    // instead of one query *per transaction*.
    if (!realByID.empty())
    {
        QSqlQuery commandPhasesQuery(database);
        commandPhasesQuery.prepare(commandPhasesReal + "(" + joinIds(realByID) + ")");
        executeQuery(commandPhasesQuery);
        attachCommandPhases(commandPhasesQuery, realByID);
    }

    if (!internalByID.empty())
    {
        QSqlQuery commandPhasesQuery(database);
        commandPhasesQuery.prepare(commandPhasesInternal + "(" + joinIds(internalByID) + ")");
        executeQuery(commandPhasesQuery);
        attachCommandPhases(commandPhasesQuery, internalByID);
    }

    return result;
}

void TraceDB::attachCommandPhases(QSqlQuery& commandPhasesQuery,
                                  std::unordered_map<ID, std::shared_ptr<Transaction>>& transactionsByID)
{
    while (commandPhasesQuery.next())
    {
        auto it = transactionsByID.find(commandPhasesQuery.value(0).toUInt());
        if (it == transactionsByID.end())
            continue;

        auto& transaction = it->second;

        QString phaseName = commandPhasesQuery.value(1).toString();
        Timespan span(commandPhasesQuery.value(2).toLongLong(),
                      commandPhasesQuery.value(3).toLongLong());
        Timespan spanOnDataStrobe(commandPhasesQuery.value(4).toLongLong(),
                                  commandPhasesQuery.value(5).toLongLong());
        unsigned int rank = commandPhasesQuery.value(6).toUInt();
        unsigned int bankGroup = commandPhasesQuery.value(7).toUInt();
        unsigned int bank = commandPhasesQuery.value(8).toUInt();
        unsigned int dualBank = commandPhasesQuery.value(9).toUInt();
        unsigned int row = commandPhasesQuery.value(10).toUInt();
        unsigned int column = commandPhasesQuery.value(11).toUInt();
        unsigned int burstLength = commandPhasesQuery.value(12).toUInt();
        auto phase = PhaseFactory::createPhase(phaseName,
                                               span,
                                               spanOnDataStrobe,
                                               rank,
                                               bankGroup,
                                               bank,
                                               row,
                                               column,
                                               burstLength,
                                               dualBank,
                                               transaction,
                                               *this);
        transaction->addPhase(phase);
    }
}

std::vector<CommentModel::Comment> TraceDB::parseCommentsFromQuery(QSqlQuery& query)
{
    std::vector<CommentModel::Comment> result;
    while (query.next())
    {
        result.push_back(
            CommentModel::Comment{query.value(0).toLongLong(), query.value(1).toString()});
    }
    return result;
}

void TraceDB::executeQuery(QSqlQuery query)
{
    // query.exec returns bool indicating if the query was successful
    if (!query.exec())
    {
        query.finish();
        throw sqlException(("Query failed. Error: \n" + query.lastError().text()).toStdString(),
                           this->pathToDB.toStdString());
    }
}
