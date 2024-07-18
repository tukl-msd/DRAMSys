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
 *    Eder F. Zulian
 *    Lukas Steiner
 *    Derek Christ
 */

#ifndef TLMRECORDER_H
#define TLMRECORDER_H

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/common/utils.h"
#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/controller/McConfig.h"
#include "DRAMSys/simulation/SimConfig.h"

#include <string>
#include <systemc>
#include <thread>
#include <tlm>
#include <unordered_map>
#include <utility>
#include <vector>

class sqlite3;
class sqlite3_stmt;

namespace DRAMSys
{

class TlmRecorder
{
public:
    TlmRecorder(const std::string& name,
                const SimConfig& simConfig,
                const McConfig& mcConfig,
                const MemSpec& memSpec,
                const std::string& dbName,
                const std::string& mcconfig,
                const std::string& memspec,
                const std::string& traces);
    TlmRecorder(const TlmRecorder&) = delete;
    TlmRecorder(TlmRecorder&&) = default;
    TlmRecorder& operator=(const TlmRecorder&) = delete;
    TlmRecorder& operator=(TlmRecorder&&) = delete;
    ~TlmRecorder() = default;

    void recordPhase(tlm::tlm_generic_payload& trans,
                     const tlm::tlm_phase& phase,
                     const sc_core::sc_time& delay);
    void recordPower(double timeInSeconds, double averagePower);
    void recordBufferDepth(double timeInSeconds, const std::vector<double>& averageBufferDepth);
    void recordBandwidth(double timeInSeconds, double averageBandwidth);
    void recordDebugMessage(const std::string& message, const sc_core::sc_time& time);
    void finalize();

private:
    std::string name;
    const SimConfig& simConfig;
    const McConfig& mcConfig;
    const MemSpec& memSpec;

    struct Transaction
    {
        Transaction(uint64_t id,
                    uint64_t address,
                    unsigned int dataLength,
                    char cmd,
                    const sc_core::sc_time& timeOfGeneration,
                    Thread thread,
                    Channel channel) :
            id(id),
            address(address),
            dataLength(dataLength),
            cmd(cmd),
            timeOfGeneration(timeOfGeneration),
            thread(thread),
            channel(channel)
        {
        }

        uint64_t id = 0;
        uint64_t address = 0;
        unsigned int dataLength = 0;
        char cmd = 'X';
        sc_core::sc_time timeOfGeneration;
        Thread thread;
        Channel channel;

        struct Phase
        {
            // for BEGIN_REQ and BEGIN_RESP
            Phase(std::string name, const sc_core::sc_time& begin) :
                name(std::move(name)),
                interval(begin, sc_core::SC_ZERO_TIME)
            {
            }
            Phase(std::string name,
                  TimeInterval interval,
                  TimeInterval intervalOnDataStrobe,
                  Rank rank,
                  BankGroup bankGroup,
                  Bank bank,
                  Row row,
                  Column column,
                  unsigned int burstLength) :
                name(std::move(name)),
                interval(std::move(interval)),
                intervalOnDataStrobe(std::move(intervalOnDataStrobe)),
                rank(rank),
                bankGroup(bankGroup),
                bank(bank),
                row(row),
                column(column),
                burstLength(burstLength)
            {
            }
            std::string name;
            TimeInterval interval;
            TimeInterval intervalOnDataStrobe = {sc_core::SC_ZERO_TIME, sc_core::SC_ZERO_TIME};
            Rank rank = Rank(0);
            BankGroup bankGroup = BankGroup(0);
            Bank bank = Bank(0);
            Row row = Row(0);
            Column column = Column(0);
            unsigned int burstLength = 0;
        };
        std::vector<Phase> recordedPhases;
    };

    void prepareSqlStatements();
    void executeInitialSqlCommand();
    static void executeSqlStatement(sqlite3_stmt* statement);

    void openDB(const std::string& dbName);
    void closeConnection();

    void introduceTransactionToSystem(tlm::tlm_generic_payload& trans);
    void removeTransactionFromSystem(tlm::tlm_generic_payload& trans);

    void terminateRemainingTransactions();
    void commitRecordedDataToDB();
    void insertGeneralInfo(const std::string& mcConfigString,
                           const std::string& memSpecString,
                           const std::string& traces);
    void insertCommandLengths();
    void insertTransactionInDB(const Transaction& recordingData);
    void insertRangeInDB(uint64_t id, const sc_core::sc_time& begin, const sc_core::sc_time& end);
    void insertPhaseInDB(const Transaction::Phase& phase, uint64_t transactionID);
    void insertDebugMessageInDB(const std::string& message, const sc_core::sc_time& time);

    static constexpr unsigned transactionCommitRate = 8192;
    std::array<std::vector<Transaction>, 2> recordingDataBuffer;
    std::vector<Transaction>* currentDataBuffer;
    std::vector<Transaction>* storageDataBuffer;
    std::thread storageThread;

    std::unordered_map<tlm::tlm_generic_payload*, Transaction> currentTransactionsInSystem;

    uint64_t totalNumTransactions = 0;
    sc_core::sc_time simulationTimeCoveredByRecording;

    sqlite3* db = nullptr;
    sqlite3_stmt *insertTransactionStatement = nullptr, *insertRangeStatement = nullptr,
                 *updateRangeStatement = nullptr, *insertPhaseStatement = nullptr,
                 *updatePhaseStatement = nullptr, *insertGeneralInfoStatement = nullptr,
                 *insertCommandLengthsStatement = nullptr, *insertDebugMessageStatement = nullptr,
                 *insertPowerStatement = nullptr, *insertBufferDepthStatement = nullptr,
                 *insertBandwidthStatement = nullptr;
    std::string insertTransactionString, insertRangeString, updateRangeString, insertPhaseString,
        updatePhaseString, insertGeneralInfoString, insertCommandLengthsString,
        insertDebugMessageString, insertPowerString, insertBufferDepthString, insertBandwidthString;

    std::string initialCommand = R"(
        DROP TABLE IF EXISTS Phases;
        DROP TABLE IF EXISTS GeneralInfo;
        DROP TABLE IF EXISTS CommandLengths;
        DROP TABLE IF EXISTS Comments;
        DROP TABLE IF EXISTS ranges;
        DROP TABLE IF EXISTS Transactions;
        DROP TABLE IF EXISTS DebugMessages;
        DROP TABLE IF EXISTS Power;
        DROP TABLE IF EXISTS BufferDepth;
        DROP TABLE IF EXISTS Bandwidth;

        CREATE TABLE Phases(
                ID INTEGER PRIMARY KEY,
                PhaseName TEXT,
                PhaseBegin INTEGER,
                PhaseEnd INTEGER,
                DataStrobeBegin INTEGER,
                DataStrobeEnd INTEGER,
                Rank INTEGER,
                BankGroup INTEGER,
                Bank INTEGER,
                Row INTEGER,
                Column INTEGER,
                BurstLength INTEGER,
                Transact INTEGER
        );

        CREATE TABLE GeneralInfo(
                NumberOfRanks INTEGER,
                NumberOfBankgroups INTEGER,
                NumberOfBanks INTEGER,
                clk INTEGER,
                UnitOfTime TEXT,
                MCconfig TEXT,
                Memspec TEXT,
                Traces TEXT,
                WindowSize INTEGER,
                RefreshMaxPostponed INTEGER,
                RefreshMaxPulledin INTEGER,
                ControllerThread INTEGER,
                MaxBufferDepth INTEGER,
                Per2BankOffset INTEGER,
                RowColumnCommandBus BOOL,
                PseudoChannelMode BOOL
        );

        CREATE TABLE CommandLengths(
                Command TEXT,
                Length DOUBLE
        );

        CREATE TABLE Power(
                time DOUBLE,
                AveragePower DOUBLE
        );

        CREATE TABLE BufferDepth(
            Time DOUBLE,
            BufferNumber INTEGER,
            AverageBufferDepth DOUBLE
        );

        CREATE TABLE Bandwidth(
            Time DOUBLE,
            AverageBandwidth DOUBLE
        );

        CREATE TABLE Comments(
                Time INTEGER,
                Text TEXT
        );

        CREATE TABLE DebugMessages(
                Time INTEGER,
                Message TEXT
        );

        -- use SQLITE R* TREE Module to make queries on timespans effecient (see http://www.sqlite.org/rtree.html)
        CREATE VIRTUAL TABLE ranges USING rtree(
           id,
           begin, end
        );

        CREATE TABLE Transactions(
                ID INTEGER,
                Range INTEGER,
                Address INTEGER,
                DataLength INTEGER,
                Thread INTEGER,
                Channel INTEGER,
                TimeOfGeneration INTEGER,
                Command TEXT
        );

        CREATE INDEX ranges_index ON Transactions(Range);
        CREATE INDEX "phasesTransactions" ON "Phases" ("Transact" ASC);
        CREATE INDEX "messageTimes" ON "DebugMessages" ("Time" ASC);
    )";
};

} // namespace DRAMSys

#endif // TLMRECORDER_H
