/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
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
 *    Eder F. Zulian
 *    Lukas Steiner
 *    Derek Christ
 */

#ifndef TLMRECORDER_H
#define TLMRECORDER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <thread>

#include <systemc>
#include <tlm>
#include "sqlite3.h"
#include "dramExtensions.h"
#include "utils.h"
#include "../configuration/Configuration.h"

class TlmRecorder
{
public:
    TlmRecorder(const std::string& name, const Configuration& config, const std::string& dbName);
    TlmRecorder(const TlmRecorder&) = delete;
    TlmRecorder(TlmRecorder&&) = default;

    void recordMcConfig(std::string _mcconfig)
    {
        mcconfig = std::move(_mcconfig);
    }

    void recordMemspec(std::string _memspec)
    {
        memspec = std::move(_memspec);
    }

    void recordTraceNames(std::string _traces)
    {
        traces = std::move(_traces);
    }

    void recordPhase(tlm::tlm_generic_payload &trans, const tlm::tlm_phase &phase, const sc_core::sc_time &time);
    void recordPower(double timeInSeconds, double averagePower);
    void recordBufferDepth(double timeInSeconds, const std::vector<double> &averageBufferDepth);
    void recordBandwidth(double timeInSeconds, double averageBandwidth);
    void recordDebugMessage(const std::string &message, const sc_core::sc_time &time);
    void updateDataStrobe(const sc_core::sc_time &begin, const sc_core::sc_time &end,
                          tlm::tlm_generic_payload &trans);
    void finalize();

private:
    const Configuration& config;

    struct Transaction
    {
        Transaction() = default;
        explicit Transaction(uint64_t id) : id(id) {}

        uint64_t id = 0;
        uint64_t address = 0;
        unsigned int burstLength = 0;
        char cmd = 'X';
        DramExtension dramExtension;
        sc_core::sc_time timeOfGeneration;
        TimeInterval timeOnDataStrobe;

        struct Phase
        {
            Phase(std::string name, const sc_core::sc_time& begin): name(std::move(name)),
                    interval(begin, sc_core::SC_ZERO_TIME) {}
            std::string name;
            TimeInterval interval;
        };
        std::vector<Phase> recordedPhases;
    };

    std::string name;

    std::string mcconfig, memspec, traces;

    void prepareSqlStatements();
    void executeInitialSqlCommand();
    static void executeSqlStatement(sqlite3_stmt *statement);

    void openDB(const std::string &dbName);
    void closeConnection();

    void introduceTransactionSystem(tlm::tlm_generic_payload &trans);
    void removeTransactionFromSystem(tlm::tlm_generic_payload &trans);

    void terminateRemainingTransactions();
    void commitRecordedDataToDB();
    void insertGeneralInfo();
    void insertCommandLengths();
    void insertTransactionInDB(Transaction &recordingData);
    void insertRangeInDB(uint64_t id, const sc_core::sc_time &begin, const sc_core::sc_time &end);
    void insertPhaseInDB(const std::string &phaseName, const sc_core::sc_time &begin, const sc_core::sc_time &end,
                         uint64_t transactionID);
    void insertDebugMessageInDB(const std::string &message, const sc_core::sc_time &time);

    static constexpr unsigned transactionCommitRate = 8192;
    std::array<std::vector<Transaction>, 2> recordingDataBuffer;
    std::vector<Transaction> *currentDataBuffer;
    std::vector<Transaction> *storageDataBuffer;
    std::thread storageThread;

    std::unordered_map<tlm::tlm_generic_payload *, Transaction> currentTransactionsInSystem;

    uint64_t totalNumTransactions;
    sc_core::sc_time simulationTimeCoveredByRecording;

    sqlite3 *db = nullptr;
    sqlite3_stmt *insertTransactionStatement = nullptr, *insertRangeStatement = nullptr,
            *updateRangeStatement = nullptr, *insertPhaseStatement = nullptr, *updatePhaseStatement = nullptr,
            *insertGeneralInfoStatement = nullptr, *insertCommandLengthsStatement = nullptr,
            *insertDebugMessageStatement = nullptr, *updateDataStrobeStatement = nullptr,
            *insertPowerStatement = nullptr, *insertBufferDepthStatement = nullptr, *insertBandwidthStatement = nullptr;
    std::string insertTransactionString, insertRangeString, updateRangeString, insertPhaseString,
            updatePhaseString, insertGeneralInfoString, insertCommandLengthsString,
            insertDebugMessageString, updateDataStrobeString, insertPowerString,
            insertBufferDepthString, insertBandwidthString;

    std::string initialCommand =
        "DROP TABLE IF EXISTS Phases;                                                                              \n"
        "DROP TABLE IF EXISTS GeneralInfo;                                                                         \n"
        "DROP TABLE IF EXISTS CommandLengths;                                                                      \n"
        "DROP TABLE IF EXISTS Comments;                                                                            \n"
        "DROP TABLE IF EXISTS ranges;                                                                              \n"
        "DROP TABLE IF EXISTS Transactions;                                                                        \n"
        "DROP TABLE IF EXISTS DebugMessages;                                                                       \n"
        "DROP TABLE IF EXISTS Power;                                                                               \n"
        "DROP TABLE IF EXISTS BufferDepth;                                                                         \n"
        "DROP TABLE IF EXISTS Bandwidth;                                                                           \n"
        "                                                                                                          \n"
        "CREATE TABLE Phases(                                                                                      \n"
        "        ID INTEGER PRIMARY KEY,                                                                           \n"
        "        PhaseName TEXT,                                                                                   \n"
        "        PhaseBegin INTEGER,                                                                               \n"
        "        PhaseEnd INTEGER,                                                                                 \n"
        "        Transact INTEGER                                                                                  \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE GeneralInfo(                                                                                 \n"
        "        NumberOfTransactions INTEGER,                                                                     \n"
        "        TraceEnd INTEGER,                                                                                 \n"
        "        NumberOfRanks INTEGER,                                                                            \n"
        "        NumberOfBankgroups INTEGER,                                                                       \n"
        "        NumberOfBanks INTEGER,                                                                            \n"
        "        clk INTEGER,                                                                                      \n"
        "        UnitOfTime TEXT,                                                                                  \n"
        "        MCconfig TEXT,                                                                                    \n"
        "        Memspec TEXT,                                                                                     \n"
        "        Traces TEXT,                                                                                      \n"
        "        WindowSize INTEGER,                                                                               \n"
        "        RefreshMaxPostponed INTEGER,                                                                      \n"
        "        RefreshMaxPulledin INTEGER,                                                                       \n"
        "        ControllerThread INTEGER,                                                                         \n"
        "        MaxBufferDepth INTEGER,                                                                           \n"
        "        Per2BankOffset INTEGER,                                                                           \n"
        "        RowColumnCommandBus BOOL,                                                                         \n"
        "        PseudoChannelMode BOOL                                                                            \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE CommandLengths(                                                                              \n"
        "        Command TEXT,                                                                                     \n"
        "        Length INTEGER                                                                                    \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE Power(                                                                                       \n"
        "        time DOUBLE,                                                                                      \n"
        "        AveragePower DOUBLE                                                                               \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE BufferDepth(                                                                                 \n"
        "    Time DOUBLE,                                                                                          \n"
        "    BufferNumber INTEGER,                                                                                 \n"
        "    AverageBufferDepth DOUBLE                                                                             \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE Bandwidth(                                                                                   \n"
        "    Time DOUBLE,                                                                                          \n"
        "    AverageBandwidth DOUBLE                                                                               \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE Comments(                                                                                    \n"
        "        Time INTEGER,                                                                                     \n"
        "        Text TEXT                                                                                         \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE DebugMessages(                                                                               \n"
        "        Time INTEGER,                                                                                     \n"
        "        Message TEXT                                                                                      \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "-- use SQLITE R* TREE Module to make queries on timespans effecient (see http://www.sqlite.org/rtree.html)\n"
        "CREATE VIRTUAL TABLE ranges USING rtree(                                                                  \n"
        "   id,                                                                                                    \n"
        "   begin, end                                                                                             \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE TABLE Transactions(                                                                                \n"
        "        ID INTEGER,                                                                                       \n"
        "        Range INTEGER,                                                                                    \n"
        "        Address INTEGER,                                                                                  \n"
        "        Burstlength INTEGER,                                                                              \n"
        "        TThread INTEGER,                                                                                  \n"
        "        TChannel INTEGER,                                                                                 \n"
        "        TRank INTEGER,                                                                                    \n"
        "        TBankgroup INTEGER,                                                                               \n"
        "        TBank INTEGER,                                                                                    \n"
        "        TRow INTEGER,                                                                                     \n"
        "        TColumn INTEGER,                                                                                  \n"
        "        DataStrobeBegin INTEGER,                                                                          \n"
        "        DataStrobeEnd INTEGER,                                                                            \n"
        "        TimeOfGeneration INTEGER,                                                                         \n"
        "        Command TEXT                                                                                      \n"
        ");                                                                                                        \n"
        "                                                                                                          \n"
        "CREATE INDEX ranges_index ON Transactions(Range);                                                         \n"
        "CREATE INDEX \"phasesTransactions\" ON \"Phases\" (\"Transact\" ASC);                                     \n"
        "CREATE INDEX \"messageTimes\" ON \"DebugMessages\" (\"Time\" ASC);                                        \n";
};

#endif // TLMRECORDER_H
