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
#include <iostream>

// define symbol printqueries if all queries should be printed to the console
// #define printqueries

TraceDB::TraceDB(const QString& path, bool openExisting)
{
    this->pathToDB = path;

    database = QSqlDatabase::database(path);
    if (database.isValid() && database.isOpen())
    {
        // Close the database connection if it exists and was not closed yet.
        database.close();
        QSqlDatabase::removeDatabase(path);
    }

    database = QSqlDatabase::addDatabase("QSQLITE", path);
    database.setDatabaseName(path);
    if (!database.open())
    {
        qDebug() << database.lastError().text();
    }
    if (!openExisting)
        dropAndCreateTables();
    prepareQueries();
    generalInfo = getGeneralInfoFromDB();
    commandLengths = getCommandLengthsFromDB();
}

void TraceDB::prepareQueries()
{
    selectTransactionsByTimespan = QSqlQuery(database);
    if (!selectTransactionsByTimespan.prepare(queryTexts.selectTransactionsByTimespan))
        qDebug() << database.lastError().text();

    selectTransactionById = QSqlQuery(database);
    if (!selectTransactionById.prepare(queryTexts.selectTransactionById))
        qDebug() << database.lastError().text();

    selectDebugMessagesByTimespan = QSqlQuery(database);
    if (!selectDebugMessagesByTimespan.prepare(
            "SELECT time, Message FROM DebugMessages WHERE :begin <= time AND time <= :end "))
        qDebug() << database.lastError().text();

    selectDebugMessagesByTimespanWithLimit = QSqlQuery(database);
    if (!selectDebugMessagesByTimespanWithLimit.prepare(
            "SELECT time, Message FROM DebugMessages WHERE :begin <= time AND time <= :end LIMIT "
            ":limit"))
        qDebug() << database.lastError().text();

    checkDependenciesExist = QSqlQuery(database);
    if (!checkDependenciesExist.prepare(queryTexts.checkDependenciesExist))
        qDebug() << database.lastError().text();

    selectDependenciesByTimespan = QSqlQuery(database);
    if (!selectDependenciesByTimespan.prepare(queryTexts.selectDependenciesByTimespan))
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

void TraceDB::updateFileDescription(const QString& description)
{
    QSqlQuery query(database);
    query.prepare("UPDATE GeneralInfo SET Description=:description");
    query.bindValue(":description", description);
    executeQuery(query);
}

void TraceDB::refreshData()
{
    prepareQueries();
    generalInfo = getGeneralInfoFromDB();
}

// QueryText must select the fields
// TransactionID, Ranges.begin, Ranges.end, Address, TThread, TChannel, TBank, TRow, TColumn,
// Phases.ID AS PhaseID, PhaseName, PhaseBegin, PhaseEnd
std::vector<std::shared_ptr<Transaction>>
TraceDB::getTransactionsWithCustomQuery(const QString& queryText)
{
    QSqlQuery query(database);
    query.prepare(queryText);
    executeQuery(query);
    return parseTransactionsFromQuery(query);
}

std::vector<std::shared_ptr<Transaction>>
TraceDB::getTransactionsInTimespan(const Timespan& span, bool updateVisiblePhases)
{
    selectTransactionsByTimespan.bindValue(":begin", span.Begin());
    selectTransactionsByTimespan.bindValue(":end", span.End());
    executeQuery(selectTransactionsByTimespan);
    return parseTransactionsFromQuery(selectTransactionsByTimespan, updateVisiblePhases);
}

bool TraceDB::checkDependencyTableExists()
{
    executeQuery(checkDependenciesExist);

    bool exists = checkDependenciesExist.next() && checkDependenciesExist.value(0).toInt() == 1;

    checkDependenciesExist.finish();

    return exists;
}

#ifdef EXTENSION_ENABLED
void TraceDB::updateDependenciesInTimespan(const Timespan& span)
{
    if (checkDependencyTableExists())
    {
        selectDependenciesByTimespan.bindValue(":begin", span.Begin());
        selectDependenciesByTimespan.bindValue(":end", span.End());
        executeQuery(selectDependenciesByTimespan);
        mUpdateDependenciesFromQuery(selectDependenciesByTimespan);
    }
}
#endif

// TODO Remove exception
std::shared_ptr<Transaction> TraceDB::getTransactionByID(ID id)
{
    selectTransactionById.bindValue(":id", id);
    executeQuery(selectTransactionById);
    auto result = parseTransactionsFromQuery(selectTransactionById);
    if (!result.empty())
        return result[0];
    else
        throw sqlException(
            ("Transaction with ID " + QString::number(id) + " not in DB").toStdString(),
            this->pathToDB.toStdString());
}

std::shared_ptr<Transaction> TraceDB::getNextActivate(traceTime time)
{
    QSqlQuery query(database);
    QString queryText =
        queryTexts.queryHead +
        "WHERE PhaseBegin > :traceTime AND PhaseName = 'ACT' ORDER BY PhaseBegin ASC LIMIT 1";

    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

std::shared_ptr<Transaction> TraceDB::getNextPrecharge(traceTime time)
{
    QSqlQuery query(database);
    QString queryText = queryTexts.queryHead +
                        "WHERE PhaseBegin > :traceTime AND PhaseName "
                        "IN ('PRE','PREPB','PREA','PREAB','PRESB') ORDER BY PhaseBegin ASC LIMIT 1";

    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

// shared_ptr<Transaction> TraceDB::getNextActb(ID currentTransactionId)
// {
//     QSqlQuery query(database);
//     QString queryText = queryTexts.queryHead +
//                         "WHERE TransactionID > :currentID AND PhaseName = 'ACTB' LIMIT 1";
//
//     query.prepare(queryText);
//     query.bindValue(":currentID", currentTransactionId);
//     executeQuery(query);
//     return parseTransactionFromQuery(query);
// }

// shared_ptr<Transaction> TraceDB::getNextPreb(ID currentTransactionId)
// {
//     QSqlQuery query(database);
//     QString queryText = queryTexts.queryHead +
//                         "WHERE TransactionID > :currentID AND PhaseName = 'PREB' LIMIT 1";
//
//     query.prepare(queryText);
//     query.bindValue(":currentID", currentTransactionId);
//     executeQuery(query);
//     return parseTransactionFromQuery(query);
// }

std::shared_ptr<Transaction> TraceDB::getNextRefresh(traceTime time)
{
    QSqlQuery query(database);
    QString queryText = queryTexts.queryHead +
                        "WHERE PhaseBegin > :traceTime AND PhaseName "
                        "IN ('REFAB','REFA','REFB','REFPB','REFP2B','REFSB','SREF','SREFB') ORDER "
                        "BY PhaseBegin ASC LIMIT 1";
    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

std::shared_ptr<Transaction> TraceDB::getNextCommand(traceTime time)
{
    QSqlQuery query(database);
    QString queryText =
        queryTexts.queryHead + "WHERE PhaseBegin > :traceTime ORDER BY PhaseBegin ASC LIMIT 1";
    query.prepare(queryText);
    query.bindValue(":traceTime", time);
    executeQuery(query);
    return parseTransactionFromQuery(query);
}

// shared_ptr<Transaction> TraceDB::getNextRefb(ID currentTransactionId)
// {
//     QSqlQuery query(database);
//     QString queryText = queryTexts.queryHead +
//                         "WHERE TransactionID > :currentID AND PhaseName = 'REFB' LIMIT 1";
//
//     query.prepare(queryText);
//     query.bindValue(":currentID", currentTransactionId);
//     executeQuery(query);
//     return parseTransactionFromQuery(query);
// }

ID TraceDB::getTransactionIDFromPhaseID(ID phaseID)
{
    QSqlQuery query(database);
    query.prepare("SELECT Transact FROM Phases WHERE ID=:id");
    query.bindValue(":id", phaseID);
    executeQuery(query);

    if (query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        throw sqlException("Phase with ID " + std::to_string(phaseID) + " not in db",
                           this->pathToDB.toStdString());
    }
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
    parameter = getParameterFromTable("Traces", "GeneralInfo");
    QString traces = parameter.isValid() ? "Traces: " + parameter.toString() : "Traces: empty";
    parameter = getParameterFromTable("Memspec", "GeneralInfo");
    QString memspec = parameter.isValid() ? "Memspec: " + parameter.toString() : "Memspec: empty";
    parameter = getParameterFromTable("MCconfig", "GeneralInfo");
    QString mcconfig =
        parameter.isValid() ? "MCconfig: " + parameter.toString() : "MCconfig: empty";
    parameter = getParameterFromTable("WindowSize", "GeneralInfo");
    uint64_t windowSize = parameter.isValid() ? parameter.toULongLong() : 0;
    parameter = getParameterFromTable("RefreshMaxPostponed", "GeneralInfo");
    unsigned refreshMaxPostponed = parameter.isValid() ? parameter.toUInt() : 0;
    parameter = getParameterFromTable("RefreshMaxPulledin", "GeneralInfo");
    unsigned refreshMaxPulledin = parameter.isValid() ? parameter.toUInt() : 0;
    parameter = getParameterFromTable("ControllerThread", "GeneralInfo");
    unsigned controllerThread = parameter.isValid() ? parameter.toUInt() : UINT_MAX;
    parameter = getParameterFromTable("MaxBufferDepth", "GeneralInfo");
    unsigned maxBufferDepth = parameter.isValid() ? parameter.toUInt() : 8;
    parameter = getParameterFromTable("Per2BankOffset", "GeneralInfo");
    unsigned per2BankOffset = parameter.isValid() ? parameter.toUInt() : 1;
    parameter = getParameterFromTable("RowColumnCommandBus", "GeneralInfo");
    bool rowColumnCommandBus = parameter.isValid() && parameter.toBool();
    parameter = getParameterFromTable("PseudoChannelMode", "GeneralInfo");
    bool pseudoChannelMode = parameter.isValid() && parameter.toBool();

    uint64_t numberOfPhases = getNumberOfPhases();
    uint64_t numberOfTransactions = getNumberOfTransactions();
    auto traceEnd = static_cast<traceTime>(getTraceLength());

    QString description = (traces + "\n");
    description += mcconfig + "\n";
    description += memspec + "\n";
    description += "Number of Transactions: " + QString::number(numberOfTransactions) + "\n";
    description += "Clock period: " + QString::number(clkPeriod) + " " + unitOfTime + "\n";
    description += "Length of trace: " + prettyFormatTime(traceEnd) + "\n";
    description += "Window size: " + QString::number(windowSize) + "\n";

    return {numberOfTransactions,
            numberOfPhases,
            Timespan(0, traceEnd),
            numberOfRanks,
            numberOfBankGroups,
            numberOfBanks,
            description,
            unitOfTime,
            clkPeriod,
            windowSize,
            refreshMaxPostponed,
            refreshMaxPulledin,
            controllerThread,
            maxBufferDepth,
            per2BankOffset,
            rowColumnCommandBus,
            pseudoChannelMode};
}

CommandLengths TraceDB::getCommandLengthsFromDB()
{
    const std::string table = "CommandLengths";

    auto getLengthFromDb = [=, &table](const std::string& command) -> QVariant
    {
        QSqlQuery query(
            ("SELECT Length FROM " + table + " WHERE Command = \"" + command + "\"").c_str(),
            database);

        if (query.first())
            return query.value(0);
        else
            return {};
    };

    auto getCommandLength = [=, &table](const std::string& command) -> double
    {
        QVariant length = getLengthFromDb(command);

        if (length.isValid())
            return length.toDouble();
        else
        {
            qDebug() << "CommandLength for" << command.c_str() << "not present in table"
                     << table.c_str() << ". Defaulting to 1.";
            return 1;
        }
    };

    double NOP = getCommandLength("NOP");
    double RD = getCommandLength("RD");
    double WR = getCommandLength("WR");
    double MWR = getCommandLength("MWR");
    double RDA = getCommandLength("RDA");
    double WRA = getCommandLength("WRA");
    double MWRA = getCommandLength("MWRA");
    double ACT = getCommandLength("ACT");

    double PREPB = getCommandLength("PREPB");
    double REFPB = getCommandLength("REFPB");

    double RFMPB = getCommandLength("RFMPB");
    double REFP2B = getCommandLength("REFP2B");
    double RFMP2B = getCommandLength("RFMP2B");
    double PRESB = getCommandLength("PRESB");
    double REFSB = getCommandLength("REFSB");
    double RFMSB = getCommandLength("RFMSB");

    double PREAB = getCommandLength("PREAB");
    double REFAB = getCommandLength("REFAB");

    double RFMAB = getCommandLength("RFMAB");
    double PDEA = getCommandLength("PDEA");
    double PDXA = getCommandLength("PDXA");
    double PDEP = getCommandLength("PDEP");
    double PDXP = getCommandLength("PDXP");
    double SREFEN = getCommandLength("SREFEN");
    double SREFEX = getCommandLength("SREFEX");

    return {NOP,   RD,    WR,     MWR,    RDA,   WRA,    MWRA,  ACT,   PREPB,
            REFPB, RFMPB, REFP2B, RFMP2B, PRESB, REFSB,  RFMSB, PREAB, REFAB,
            RFMAB, PDEA,  PDXA,   PDEP,   PDXP,  SREFEN, SREFEX};
}

QVariant TraceDB::getParameterFromTable(const std::string& parameter, const std::string& table)
{
    QSqlQuery query(("SELECT " + parameter + " FROM " + table).c_str(), database);
    if (query.first())
        return query.value(0);
    else
    {
        qDebug() << "Parameter " << parameter.c_str() << " not present in table " << table.c_str();
        return {};
    }
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

uint64_t TraceDB::getNumberOfPhases()
{
    QSqlQuery query(database);
    query.prepare("SELECT COUNT(ID) FROM Phases");
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

std::vector<CommentModel::Comment> TraceDB::getDebugMessagesInTimespan(const Timespan& span)
{
    selectDebugMessagesByTimespan.bindValue(":begin", span.Begin());
    selectDebugMessagesByTimespan.bindValue(":end", span.End());
    executeQuery(selectDebugMessagesByTimespan);

    return parseCommentsFromQuery(selectDebugMessagesByTimespan);
}

std::vector<CommentModel::Comment> TraceDB::getDebugMessagesInTimespan(const Timespan& span,
                                                                       unsigned int limit = 50)
{
    selectDebugMessagesByTimespanWithLimit.bindValue(":begin", span.Begin());
    selectDebugMessagesByTimespanWithLimit.bindValue(":end", span.End());
    selectDebugMessagesByTimespanWithLimit.bindValue(":limit", limit);
    executeQuery(selectDebugMessagesByTimespanWithLimit);
    return parseCommentsFromQuery(selectDebugMessagesByTimespanWithLimit);
}

#ifdef EXTENSION_ENABLED
DependencyInfos TraceDB::getDependencyInfos(DependencyInfos::Type infoType)
{
    DependencyInfos dummy;
    if (!checkDependencyTableExists())
    {
        return dummy;
    }

    switch (infoType)
    {
    case DependencyInfos::Type::DependencyType:
    {
        selectDependencyTypePercentages = QSqlQuery(database);
        if (!selectDependencyTypePercentages.prepare(queryTexts.selectDependencyTypePercentages))
            qDebug() << database.lastError().text();

        executeQuery(selectDependencyTypePercentages);
        return parseDependencyInfos(selectDependencyTypePercentages, infoType);
    }

    case DependencyInfos::Type::TimeDependency:
    {
        selectTimeDependencyPercentages = QSqlQuery(database);
        if (!selectTimeDependencyPercentages.prepare(queryTexts.selectTimeDependencyPercentages))
            qDebug() << database.lastError().text();

        executeQuery(selectTimeDependencyPercentages);
        return parseDependencyInfos(selectTimeDependencyPercentages, infoType);
    }

    case DependencyInfos::Type::DelayedPhase:
    {
        selectDelayedPhasePercentages = QSqlQuery(database);
        if (!selectDelayedPhasePercentages.prepare(queryTexts.selectDelayedPhasePercentages))
            qDebug() << database.lastError().text();

        executeQuery(selectDelayedPhasePercentages);
        return parseDependencyInfos(selectDelayedPhasePercentages, infoType);
    }

    case DependencyInfos::Type::DependencyPhase:
    {
        selectDependencyPhasePercentages = QSqlQuery(database);
        if (!selectDependencyPhasePercentages.prepare(queryTexts.selectDependencyPhasePercentages))
            qDebug() << database.lastError().text();

        executeQuery(selectDependencyPhasePercentages);
        return parseDependencyInfos(selectDependencyPhasePercentages, infoType);
    }
    }

    return dummy;
}
#endif

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
    else
        return {};
}

std::vector<std::shared_ptr<Transaction>>
TraceDB::parseTransactionsFromQuery(QSqlQuery& query, bool updateVisiblePhases)
{
    if (updateVisiblePhases)
    {
        _visiblePhases.clear();
    }

    std::vector<std::shared_ptr<Transaction>> result;

    bool firstIteration = true;
    ID currentID = 0;
    int i = -1;

    while (query.next())
    {
        ID id = query.value(0).toInt();

        if (currentID != id || firstIteration)
        {
            ++i;
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

        unsigned int phaseID = query.value(8).toInt();
        QString phaseName = query.value(9).toString();
        Timespan span(query.value(10).toLongLong(), query.value(11).toLongLong());
        Timespan spanOnDataStrobe(query.value(12).toLongLong(), query.value(13).toLongLong());
        unsigned int rank = query.value(14).toUInt();
        unsigned int bankGroup = query.value(15).toUInt();
        unsigned int bank = query.value(16).toUInt();
        unsigned int row = query.value(17).toUInt();
        unsigned int column = query.value(18).toUInt();
        unsigned int burstLength = query.value(19).toUInt();
        auto phase = PhaseFactory::createPhase(phaseID,
                                               phaseName,
                                               span,
                                               spanOnDataStrobe,
                                               rank,
                                               bankGroup,
                                               bank,
                                               row,
                                               column,
                                               burstLength,
                                               result.at(result.size() - 1),
                                               *this);
        result.at(result.size() - 1)->addPhase(phase);

        if (updateVisiblePhases)
        {
            _visiblePhases[phaseID] = phase;
        }
    }
    return result;
}

#ifdef EXTENSION_ENABLED
void TraceDB::mUpdateDependenciesFromQuery(QSqlQuery& query)
{
    DependencyType type;
    while (query.next())
    {
        ID delayedID = query.value(0).toInt();
        ID dependencyID = query.value(4).toInt();

        QString dependencyTypeStr = query.value(2).toString();
        if (dependencyTypeStr == "bank")
        {
            type = DependencyType::IntraBank;
        }
        else if (dependencyTypeStr == "rank")
        {
            type = DependencyType::IntraRank;
        }
        else if (dependencyTypeStr == "interRank")
        {
            type = DependencyType::InterRank;
        }

        QString timeDependencyStr = query.value(3).toString();

        if (_visiblePhases.count(delayedID) > 0)
        {

            if (_visiblePhases.count(dependencyID) > 0)
            {

                _visiblePhases[delayedID]->addDependency(std::make_shared<PhaseDependency>(
                    PhaseDependency(type, timeDependencyStr, _visiblePhases[dependencyID])));
            }
            else
            {

                _visiblePhases[delayedID]->addDependency(
                    std::make_shared<PhaseDependency>(PhaseDependency(type, timeDependencyStr)));
            }
        }
        else
        {
            // TODO delayed phase not visible?
        }
    }
}
#endif

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

#ifdef EXTENSION_ENABLED
DependencyInfos TraceDB::parseDependencyInfos(QSqlQuery& query,
                                              const DependencyInfos::Type infoType)
{
    DependencyInfos infos(infoType);

    while (query.next())
    {
        infos.addInfo({query.value(0).toString(), query.value(1).toFloat()});
    }

    query.finish();

    return infos;
}
#endif

void TraceDB::executeQuery(QSqlQuery query)
{

    // query.exec returns bool indicating if the query was sucessfull
    if (query.exec())
    {
#ifdef printqueries
        cout << queryToString(query).toStdString() << endl;
#endif
    }

    else
    {
        query.finish();
        throw sqlException(
            ("Query:\n " + queryToString(query) + "\n failed. Error: \n" + query.lastError().text())
                .toStdString(),
            this->pathToDB.toStdString());
    }
}

QString TraceDB::queryToString(const QSqlQuery& query)
{
    QString str = query.lastQuery();
    QMapIterator<QString, QVariant> it(query.boundValues());
    while (it.hasNext())
    {
        it.next();
        str.replace(it.key(), it.value().toString());
    }
    return str;
}

void TraceDB::dropAndCreateTables()
{
    executeScriptFile("common/static/createTraceDB.sql");
}

void TraceDB::executeScriptFile(const QString& fileName)
{
    QSqlQuery query(database);
    QFile scriptFile(fileName);

    if (scriptFile.open(QIODevice::ReadOnly))
    {
        // The SQLite driver executes only a single (the first) query in the QSqlQuery
        // if the script contains more queries, it needs to be splitted.
        QStringList scriptQueries = QTextStream(&scriptFile).readAll().split(';');

        for (QString& queryTxt : scriptQueries)
        {
            if (queryTxt.trimmed().isEmpty())
            {
                continue;
            }
            if (!query.exec(queryTxt))
            {
                throw sqlException("Querry failed:" + query.lastError().text().toStdString(),
                                   this->pathToDB.toStdString());
            }
            query.finish();
        }
    }
}
