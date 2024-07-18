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

#include "QueryTexts.h"

#ifdef EXTENSION_ENABLED
#include "businessObjects/phases/dependencyinfos.h"
#endif

#include "businessObjects/commandlengths.h"
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
#include <vector>

/*  TraceDB handles the connection to a SQLLite database containing trace data.
 *  A TraceDB object always holds an open connection to a valid database.
 */

class TraceDB : public QObject
{
    Q_OBJECT

public:
    TraceDB(const QString& path, bool openExisting);
    const QString& getPathToDB() const { return pathToDB; }

    void updateComments(const std::vector<CommentModel::Comment>& comments);
    void updateFileDescription(const QString& description);
    void refreshData();

#ifdef EXTENSION_ENABLED
    void updateDependenciesInTimespan(const Timespan& span);
#endif

    const GeneralInfo& getGeneralInfo() const { return generalInfo; }
    const CommandLengths& getCommandLengths() const { return commandLengths; }

    std::vector<std::shared_ptr<Transaction>>
    getTransactionsWithCustomQuery(const QString& queryText);
    std::vector<std::shared_ptr<Transaction>>
    getTransactionsInTimespan(const Timespan& span, bool updateVisiblePhases = false);
    std::shared_ptr<Transaction> getNextPrecharge(traceTime time);
    std::shared_ptr<Transaction> getNextActivate(traceTime time);
    std::shared_ptr<Transaction> getNextRefresh(traceTime time);
    std::shared_ptr<Transaction> getNextCommand(traceTime time);
    //     std::shared_ptr<Transaction> getNextPreb(ID currentTransactionId);
    //     std::shared_ptr<Transaction> getNextActb(ID currentTransactionId);
    //     std::shared_ptr<Transaction> getNextRefb(ID currentTransactionId);

    std::shared_ptr<Transaction> getTransactionByID(ID id);
    ID getTransactionIDFromPhaseID(ID phaseID);

    std::vector<CommentModel::Comment> getComments();
    std::vector<CommentModel::Comment> getDebugMessagesInTimespan(const Timespan& span);
    std::vector<CommentModel::Comment> getDebugMessagesInTimespan(const Timespan& span,
                                                                  unsigned int limit);

    bool checkDependencyTableExists();
#ifdef EXTENSION_ENABLED
    DependencyInfos getDependencyInfos(DependencyInfos::Type infoType);
#endif
    QSqlDatabase getDatabase() const;

private:
    QString pathToDB;
    QSqlDatabase database;
    GeneralInfo generalInfo;
    CommandLengths commandLengths;

    QSqlQuery insertPhaseQuery;
    QSqlQuery insertTransactionQuery;
    QSqlQuery selectTransactionsByTimespan;
    QSqlQuery selectTransactionById;
    QSqlQuery selectDebugMessagesByTimespan;
    QSqlQuery selectDebugMessagesByTimespanWithLimit;
    QSqlQuery checkDependenciesExist;
    QSqlQuery selectDependenciesByTimespan;
    QSqlQuery selectDependencyTypePercentages;
    QSqlQuery selectTimeDependencyPercentages;
    QSqlQuery selectDelayedPhasePercentages;
    QSqlQuery selectDependencyPhasePercentages;

    TransactionQueryTexts queryTexts;
    void prepareQueries();
    void executeQuery(QSqlQuery query);
    static QString queryToString(const QSqlQuery& query);
    std::shared_ptr<Transaction> parseTransactionFromQuery(QSqlQuery& query);
    std::vector<std::shared_ptr<Transaction>>
    parseTransactionsFromQuery(QSqlQuery& query, bool updateVisiblePhases = false);
    static std::vector<CommentModel::Comment> parseCommentsFromQuery(QSqlQuery& query);

#ifdef EXTENSION_ENABLED
    void mUpdateDependenciesFromQuery(QSqlQuery& query);
    static DependencyInfos parseDependencyInfos(QSqlQuery& query,
                                                const DependencyInfos::Type infoType);
#endif

    void executeScriptFile(const QString& fileName);
    void dropAndCreateTables();

    uint64_t getTraceLength();
    uint64_t getNumberOfTransactions();
    uint64_t getNumberOfPhases();
    GeneralInfo getGeneralInfoFromDB();
    CommandLengths getCommandLengthsFromDB();
    QVariant getParameterFromTable(const std::string& parameter, const std::string& table);

    std::map<unsigned int, std::shared_ptr<Phase>>
        _visiblePhases; // Updated at parseTransactionsFromQuery

    // At businessObjects/phasedependenciestracker.h
    friend class PhaseDependenciesTracker;
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
