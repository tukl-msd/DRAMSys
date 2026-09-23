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
    generalInfo = getGeneralInfoFromDB();
    commandLengths = getCommandLengthsFromDB();
}

void TraceDB::prepareQueries()
{
    selectTransactionsByTimespan = QSqlQuery(database);
    if (!selectTransactionsByTimespan.prepare(
            queryHead + " WHERE Ranges.end >= :begin AND Ranges.begin <= :end"))
        qDebug() << database.lastError().text();

    selectTransactionById = QSqlQuery(database);
    if (!selectTransactionById.prepare(queryHead + " WHERE Transactions.ID = :id"))
        qDebug() << database.lastError().text();
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

std::shared_ptr<Transaction> TraceDB::getNextActivate(traceTime time)
{
    QSqlQuery query(database);
    QString queryText =
        queryHead +
        "WHERE PhaseBegin > :traceTime AND PhaseName = 'ACT' ORDER BY PhaseBegin ASC LIMIT 1";

    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

std::shared_ptr<Transaction> TraceDB::getNextPrecharge(traceTime time)
{
    QSqlQuery query(database);
    QString queryText = queryHead +
                        "WHERE PhaseBegin > :traceTime AND PhaseName "
                        "IN ('PRE','PREPB','PREA','PREAB','PRESB') ORDER BY PhaseBegin ASC LIMIT 1";

    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

std::shared_ptr<Transaction> TraceDB::getNextRefresh(traceTime time)
{
    QSqlQuery query(database);
    QString queryText =
        queryHead +
        "WHERE PhaseBegin > :traceTime AND PhaseName "
        "IN ('REFAB','REFA','REFB','REFPB','REFP2B','REFDB','REFSB','SREF','SREFB') ORDER BY "
        "PhaseBegin ASC LIMIT 1";
    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

std::shared_ptr<Transaction> TraceDB::getNextCommand(traceTime time)
{
    QSqlQuery query(database);
    QString queryText = queryHead + "WHERE PhaseBegin > :traceTime ORDER BY PhaseBegin ASC LIMIT 1";
    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

GeneralInfo TraceDB::getGeneralInfoFromDB()
{
    QVariant parameter;

    parameter = getParameterFromTable("NumberOfRanks", "GeneralInfo");
    unsigned numberOfRanks = parameter.isValid() ? parameter.toUInt() : 1;

    parameter = getParameterFromTable("NumberOfBankgroups", "GeneralInfo");
    unsigned numberOfBankGroups = parameter.isValid() ? parameter.toUInt() : numberOfRanks;

    parameter = getParameterFromTable("NumberOfBanks", "GeneralInfo");
    unsigned numberOfBanks = parameter.isValid() ? parameter.toUInt() : numberOfBankGroups;

    parameter = getParameterFromTable("Clk", "GeneralInfo");
    uint64_t clkPeriod = parameter.isValid() ? parameter.toULongLong() : 1000;

    parameter = getParameterFromTable("UnitOfTime", "GeneralInfo");
    QString unitOfTime = parameter.isValid() ? parameter.toString() : "PS";

    parameter = getParameterFromTable("Per2BankOffset", "GeneralInfo");
    unsigned per2BankOffset = parameter.isValid() ? parameter.toUInt() : 1;

    parameter = getParameterFromTable("RowColumnCommandBus", "GeneralInfo");
    bool rowColumnCommandBus = parameter.isValid() && parameter.toBool();

    parameter = getParameterFromTable("PseudoChannelMode", "GeneralInfo");
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
        QSqlQuery query(
            ("SELECT Length FROM CommandLengths WHERE Command = \"" + command + "\"").c_str(),
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

QVariant TraceDB::getParameterFromTable(const std::string& parameter, const std::string& table)
{
    QSqlQuery query(("SELECT " + parameter + " FROM " + table).c_str(), database);
    if (query.first())
        return query.value(0);

    qDebug() << "Parameter " << parameter.c_str() << " not present in table " << table.c_str();
    return {};
}

uint64_t TraceDB::getTraceLength()
{
    QSqlQuery query(database);
    query.prepare("SELECT MAX(PhaseEnd) FROM Phases");
    executeQuery(query);

    query.next();
    return query.value(0).toULongLong();
}

uint64_t TraceDB::getNumberOfTransactions()
{
    QSqlQuery query(database);
    query.prepare("SELECT COUNT(ID) FROM Transactions");
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

std::shared_ptr<Transaction> TraceDB::parseTransactionFromQuery(QSqlQuery& query)
{
    auto result = parseTransactionsFromQuery(query);
    if (!result.empty())
        return result[0];

    return {};
}

std::vector<std::shared_ptr<Transaction>> TraceDB::parseTransactionsFromQuery(QSqlQuery& query)
{
    std::vector<std::shared_ptr<Transaction>> result;

    bool firstIteration = true;
    ID currentID = 0;

    while (query.next())
    {
        ID id = query.value(0).toInt();

        if (currentID != id || firstIteration)
        {
            firstIteration = false;
            currentID = id;
            Timespan span(query.value(1).toLongLong(), query.value(2).toLongLong());
            uint64_t address = query.value(3).toULongLong();
            unsigned int dataLength = query.value(4).toUInt();
            unsigned int thread = query.value(5).toUInt();
            unsigned int channel = query.value(6).toUInt();
            QString command = query.value(7).toString();
            result.push_back(std::make_shared<Transaction>(id,
                                                           std::move(command),
                                                           address,
                                                           dataLength,
                                                           thread,
                                                           channel,
                                                           span,
                                                           generalInfo.clkPeriod));
        }

        QString phaseName = query.value(9).toString();
        Timespan span(query.value(10).toLongLong(), query.value(11).toLongLong());
        Timespan spanOnDataStrobe(query.value(12).toLongLong(), query.value(13).toLongLong());
        unsigned int rank = query.value(14).toUInt();
        unsigned int bankGroup = query.value(15).toUInt();
        unsigned int bank = query.value(16).toUInt();
        unsigned int row = query.value(17).toUInt();
        unsigned int column = query.value(18).toUInt();
        unsigned int burstLength = query.value(19).toUInt();
        unsigned int dualBank = query.value(20).toUInt();
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
                                               result.at(result.size() - 1),
                                               *this);
        result.at(result.size() - 1)->addPhase(phase);
    }
    return result;
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
