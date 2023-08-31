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
 *    Derek Christ
 *    Lukas Steiner
 *    Derek Christ
 */

#include "TlmRecorder.h"

#include "DRAMSys/common/DebugManager.h"

#include <fstream>
#include <sqlite3.h>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

TlmRecorder::TlmRecorder(const std::string& name,
                         const Configuration& config,
                         const std::string& dbName,
                         std::string mcconfig,
                         std::string memspec,
                         std::string traces) :
    name(name),
    config(config),
    memSpec(*config.memSpec),
    currentDataBuffer(&recordingDataBuffer.at(0)),
    storageDataBuffer(&recordingDataBuffer.at(1)),
    simulationTimeCoveredByRecording(SC_ZERO_TIME)
{
    currentDataBuffer->reserve(transactionCommitRate);
    storageDataBuffer->reserve(transactionCommitRate);

    openDB(dbName);
    char* sErrMsg = nullptr;
    sqlite3_exec(db, "PRAGMA main.page_size = 4096", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA main.cache_size=10000", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA main.locking_mode=EXCLUSIVE", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA main.synchronous=OFF", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA journal_mode = OFF", nullptr, nullptr, &sErrMsg);

    executeInitialSqlCommand();
    prepareSqlStatements();

    insertGeneralInfo(mcconfig, memspec, traces);
    insertCommandLengths();

    PRINTDEBUGMESSAGE(name, "Starting new database transaction");
}

void TlmRecorder::finalize()
{
    if (db != nullptr)
        closeConnection();
    sqlite3_finalize(insertTransactionStatement);
    sqlite3_finalize(insertRangeStatement);
    sqlite3_finalize(updateRangeStatement);
    sqlite3_finalize(insertPhaseStatement);
    sqlite3_finalize(updatePhaseStatement);
    sqlite3_finalize(insertGeneralInfoStatement);
    sqlite3_finalize(insertCommandLengthsStatement);
    sqlite3_finalize(insertDebugMessageStatement);
    sqlite3_finalize(insertPowerStatement);
    sqlite3_finalize(insertBufferDepthStatement);
    sqlite3_finalize(insertBandwidthStatement);
}

void TlmRecorder::recordPower(double timeInSeconds, double averagePower)
{
    sqlite3_bind_double(insertPowerStatement, 1, timeInSeconds);
    sqlite3_bind_double(insertPowerStatement, 2, averagePower);
    executeSqlStatement(insertPowerStatement);
}

void TlmRecorder::recordBufferDepth(double timeInSeconds,
                                    const std::vector<double>& averageBufferDepth)
{
    for (size_t index = 0; index < averageBufferDepth.size(); index++)
    {
        sqlite3_bind_double(insertBufferDepthStatement, 1, timeInSeconds);
        sqlite3_bind_int(insertBufferDepthStatement, 2, static_cast<int>(index));
        sqlite3_bind_double(insertBufferDepthStatement, 3, averageBufferDepth[index]);
        executeSqlStatement(insertBufferDepthStatement);
    }
}

void TlmRecorder::recordBandwidth(double timeInSeconds, double averageBandwidth)
{
    sqlite3_bind_double(insertBandwidthStatement, 1, timeInSeconds);
    sqlite3_bind_double(insertBandwidthStatement, 2, averageBandwidth);
    executeSqlStatement(insertBandwidthStatement);
}

void TlmRecorder::recordPhase(tlm_generic_payload& trans,
                              const tlm_phase& phase,
                              const sc_time& delay)
{
    const sc_time& currentTime = sc_time_stamp();

    if (phase == BEGIN_REQ)
    {
        introduceTransactionToSystem(trans);
        std::string phaseName = getPhaseName(phase).substr(6);
        currentTransactionsInSystem.at(&trans).recordedPhases.emplace_back(phaseName,
                                                                           currentTime + delay);
    }
    if (phase == BEGIN_RESP)
    {
        std::string phaseName = getPhaseName(phase).substr(6);
        currentTransactionsInSystem.at(&trans).recordedPhases.emplace_back(phaseName,
                                                                           currentTime + delay);
    }
    else if (phase == END_REQ)
    {
        // BEGIN_REQ is always the first phase of a normal transaction
        currentTransactionsInSystem.at(&trans).recordedPhases.front().interval.end =
            currentTime + delay;
    }
    else if (phase == END_RESP)
    {
        // BEGIN_RESP is always the last phase of a normal transaction at this point
        currentTransactionsInSystem.at(&trans).recordedPhases.back().interval.end =
            currentTime + delay;
        removeTransactionFromSystem(trans);
    }
    else if (isFixedCommandPhase(phase))
    {
        tlm_generic_payload* keyTrans = nullptr;
        if (ChildExtension::isChildTrans(trans))
        {
            keyTrans = &ChildExtension::getParentTrans(trans);
        }
        else
        {
            if (currentTransactionsInSystem.find(&trans) == currentTransactionsInSystem.end())
                introduceTransactionToSystem(trans);
            keyTrans = &trans;
        }

        std::string phaseName = getPhaseName(phase).substr(6); // remove "BEGIN_"
        const ControllerExtension& extension = ControllerExtension::getExtension(trans);
        TimeInterval intervalOnDataStrobe;
        if (phaseHasDataStrobe(phase))
        {
            intervalOnDataStrobe = memSpec.getIntervalOnDataStrobe(Command(phase), trans);
            intervalOnDataStrobe.start = currentTime + intervalOnDataStrobe.start;
            intervalOnDataStrobe.end = currentTime + intervalOnDataStrobe.end;
        }

        currentTransactionsInSystem.at(keyTrans).recordedPhases.emplace_back(
            std::move(phaseName),
            std::move(TimeInterval(currentTime + delay,
                                   currentTime + delay +
                                       memSpec.getExecutionTime(Command(phase), trans))),
            std::move(intervalOnDataStrobe),
            extension.getRank(),
            extension.getBankGroup(),
            extension.getBank(),
            extension.getRow(),
            extension.getColumn(),
            extension.getBurstLength());

        if (isRefreshCommandPhase(phase))
            removeTransactionFromSystem(trans);
    }
    else if (isPowerDownEntryPhase(phase))
    {
        introduceTransactionToSystem(trans);
        std::string phaseName = getPhaseName(phase).substr(6); // remove "BEGIN_"
        const ControllerExtension& extension = ControllerExtension::getExtension(trans);
        currentTransactionsInSystem.at(&trans).recordedPhases.emplace_back(
            std::move(phaseName),
            std::move(TimeInterval(currentTime + delay, SC_ZERO_TIME)),
            std::move(TimeInterval(SC_ZERO_TIME, SC_ZERO_TIME)),
            extension.getRank(),
            extension.getBankGroup(),
            extension.getBank(),
            extension.getRow(),
            extension.getColumn(),
            extension.getBurstLength());
    }
    else if (isPowerDownExitPhase(phase))
    {
        currentTransactionsInSystem.at(&trans).recordedPhases.back().interval.end =
            currentTime + delay + memSpec.getCommandLength(Command(phase));
        removeTransactionFromSystem(trans);
    }

    simulationTimeCoveredByRecording = currentTime + delay;
}

void TlmRecorder::recordDebugMessage(const std::string& message, const sc_time& time)
{
    insertDebugMessageInDB(message, time);
}

// ------------- internal -----------------------

void TlmRecorder::introduceTransactionToSystem(tlm_generic_payload& trans)
{
    totalNumTransactions++;

    char commandChar = 0;
    tlm_command command = trans.get_command();
    if (command == TLM_READ_COMMAND)
        commandChar = 'R';
    else if (command == TLM_WRITE_COMMAND)
        commandChar = 'W';
    else
        commandChar = 'X';

    const ArbiterExtension& extension = ArbiterExtension::getExtension(trans);

    currentTransactionsInSystem.insert({&trans,
                                        Transaction(totalNumTransactions,
                                                    trans.get_address(),
                                                    trans.get_data_length(),
                                                    commandChar,
                                                    extension.getTimeOfGeneration(),
                                                    extension.getThread(),
                                                    extension.getChannel())});

    PRINTDEBUGMESSAGE(name,
                      "New transaction #" + std::to_string(totalNumTransactions) +
                          " generation time " +
                          currentTransactionsInSystem.at(&trans).timeOfGeneration.to_string());
}

void TlmRecorder::removeTransactionFromSystem(tlm_generic_payload& trans)
{
    assert(currentTransactionsInSystem.count(&trans) != 0);

    PRINTDEBUGMESSAGE(
        name, "Removing transaction #" + std::to_string(currentTransactionsInSystem.at(&trans).id));

    Transaction& recordingData = currentTransactionsInSystem.at(&trans);
    currentDataBuffer->push_back(recordingData);
    currentTransactionsInSystem.erase(&trans);

    if (currentDataBuffer->size() == transactionCommitRate)
    {
        if (storageThread.joinable())
            storageThread.join();

        std::swap(currentDataBuffer, storageDataBuffer);

        storageThread = std::thread(&TlmRecorder::commitRecordedDataToDB, this);
        currentDataBuffer->clear();
    }
}

void TlmRecorder::terminateRemainingTransactions()
{
    while (!currentTransactionsInSystem.empty())
    {
        auto transaction =
            std::min_element(currentTransactionsInSystem.begin(),
                             currentTransactionsInSystem.end(),
                             [](decltype(currentTransactionsInSystem)::value_type& l,
                                decltype(currentTransactionsInSystem)::value_type& r) -> bool
                             { return l.second.id < r.second.id; });
        if (transaction->second.cmd == 'X')
        {
            std::string beginPhase = transaction->second.recordedPhases.front().name;
            if (beginPhase == "PDNA")
                recordPhase(*(transaction->first), END_PDNA, SC_ZERO_TIME);
            else if (beginPhase == "PDNP")
                recordPhase(*(transaction->first), END_PDNP, SC_ZERO_TIME);
            else if (beginPhase == "SREF")
                recordPhase(*(transaction->first), END_SREF, SC_ZERO_TIME);
            else
                removeTransactionFromSystem(*transaction->first);
        }
        else
        {
            std::string beginPhase = transaction->second.recordedPhases.back().name;

            if (beginPhase == "RESP")
                recordPhase(*(transaction->first), END_RESP, SC_ZERO_TIME);
            else
            {
                // Do not terminate transaction as it is not ready to be completed.
                currentTransactionsInSystem.erase(transaction);

                // Decrement totalNumTransactions as this transaction will not be recorded in the
                // database.
                totalNumTransactions--;
            }
        }
    }
}

void TlmRecorder::commitRecordedDataToDB()
{
    sqlite3_exec(db, "BEGIN;", nullptr, nullptr, nullptr);
    for (const Transaction& transaction : *storageDataBuffer)
    {
        assert(!transaction.recordedPhases.empty());
        insertTransactionInDB(transaction);
        for (const Transaction::Phase& phase : transaction.recordedPhases)
        {
            insertPhaseInDB(phase, transaction.id);
        }

        sc_time rangeBegin = transaction.recordedPhases.front().interval.start;
        sc_time rangeEnd = rangeBegin;
        for (const Transaction::Phase& phase : transaction.recordedPhases)
        {
            rangeEnd = std::max(rangeEnd, phase.interval.end);
        }
        insertRangeInDB(transaction.id, rangeBegin, rangeEnd);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

void TlmRecorder::openDB(const std::string& dbName)
{
    std::ifstream f(dbName.c_str());
    if (f.good())
    {
        if (remove(dbName.c_str()) != 0)
        {
            SC_REPORT_FATAL("TlmRecorder", "Error deleting file");
        }
    }

    if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK)
    {
        SC_REPORT_FATAL("Error in TraceRecorder", "Error cannot open database");
        sqlite3_close(db);
    }
}

void TlmRecorder::prepareSqlStatements()
{
    insertTransactionString =
        "INSERT INTO Transactions VALUES (:id,:rangeID,:address,:dataLength,:thread,:channel,"
        ":timeOfGeneration,:command)";

    insertRangeString = "INSERT INTO Ranges VALUES (:id,:begin,:end)";

    updateRangeString = "UPDATE Ranges SET  End = :end WHERE ID = :id";

    insertPhaseString =
        "INSERT INTO Phases "
        "(PhaseName,PhaseBegin,PhaseEnd,DataStrobeBegin,DataStrobeEnd,Rank,BankGroup,Bank,"
        "Row,Column,BurstLength,Transact) VALUES "
        "(:name,:begin,:end,:strobeBegin,:strobeEnd,:rank,:bankGroup,:bank,"
        ":row,:column,:burstLength,:transaction)";

    updatePhaseString =
        "UPDATE Phases SET PhaseEnd = :end WHERE Transact = :trans AND PhaseName = :name";

    insertGeneralInfoString =
        "INSERT INTO GeneralInfo VALUES"
        "(:numberOfRanks, :numberOfBankGroups, :numberOfBanks, :clk, :unitOfTime, "
        ":mcconfig, :memspec, :traces, :windowSize, :refreshMaxPostponed, :refreshMaxPulledin, "
        ":controllerThread, "
        ":maxBufferDepth, :per2BankOffset, :rowColumnCommandBus, :pseudoChannelMode)";

    insertCommandLengthsString = "INSERT INTO CommandLengths VALUES"
                                 "(:command, :length)";

    insertDebugMessageString = "INSERT INTO DebugMessages (Time,Message) Values (:time,:message)";

    insertPowerString = "INSERT INTO Power VALUES (:time,:averagePower)";
    insertBufferDepthString =
        "INSERT INTO BufferDepth VALUES (:time,:bufferNumber,:averageBufferDepth)";
    insertBandwidthString = "INSERT INTO Bandwidth VALUES (:time,:averageBandwidth)";

    sqlite3_prepare_v2(
        db, insertTransactionString.c_str(), -1, &insertTransactionStatement, nullptr);
    sqlite3_prepare_v2(db, insertRangeString.c_str(), -1, &insertRangeStatement, nullptr);
    sqlite3_prepare_v2(db, updateRangeString.c_str(), -1, &updateRangeStatement, nullptr);
    sqlite3_prepare_v2(db, insertPhaseString.c_str(), -1, &insertPhaseStatement, nullptr);
    sqlite3_prepare_v2(db, updatePhaseString.c_str(), -1, &updatePhaseStatement, nullptr);
    sqlite3_prepare_v2(
        db, insertGeneralInfoString.c_str(), -1, &insertGeneralInfoStatement, nullptr);
    sqlite3_prepare_v2(
        db, insertCommandLengthsString.c_str(), -1, &insertCommandLengthsStatement, nullptr);
    sqlite3_prepare_v2(
        db, insertDebugMessageString.c_str(), -1, &insertDebugMessageStatement, nullptr);
    sqlite3_prepare_v2(db, insertPowerString.c_str(), -1, &insertPowerStatement, nullptr);
    sqlite3_prepare_v2(
        db, insertBufferDepthString.c_str(), -1, &insertBufferDepthStatement, nullptr);
    sqlite3_prepare_v2(db, insertBandwidthString.c_str(), -1, &insertBandwidthStatement, nullptr);
}

void TlmRecorder::insertDebugMessageInDB(const std::string& message, const sc_time& time)
{
    sqlite3_bind_int64(insertDebugMessageStatement, 1, static_cast<int64_t>(time.value()));
    sqlite3_bind_text(insertDebugMessageStatement,
                      2,
                      message.c_str(),
                      static_cast<int>(message.length()),
                      nullptr);
    executeSqlStatement(insertDebugMessageStatement);
}

void TlmRecorder::insertGeneralInfo(std::string mcconfig, std::string memspec, std::string traces)
{
    sqlite3_bind_int(
        insertGeneralInfoStatement, 1, static_cast<int>(config.memSpec->ranksPerChannel));
    sqlite3_bind_int(
        insertGeneralInfoStatement, 2, static_cast<int>(config.memSpec->bankGroupsPerChannel));
    sqlite3_bind_int(
        insertGeneralInfoStatement, 3, static_cast<int>(config.memSpec->banksPerChannel));
    sqlite3_bind_int64(
        insertGeneralInfoStatement, 4, static_cast<int64_t>(config.memSpec->tCK.value()));
    sqlite3_bind_text(insertGeneralInfoStatement, 5, "PS", 2, nullptr);

    sqlite3_bind_text(insertGeneralInfoStatement,
                      6,
                      mcconfig.c_str(),
                      static_cast<int>(mcconfig.length()),
                      nullptr);
    sqlite3_bind_text(insertGeneralInfoStatement,
                      7,
                      memspec.c_str(),
                      static_cast<int>(memspec.length()),
                      nullptr);
    sqlite3_bind_text(
        insertGeneralInfoStatement, 8, traces.c_str(), static_cast<int>(traces.length()), nullptr);
    if (config.enableWindowing)
        sqlite3_bind_int64(insertGeneralInfoStatement,
                           9,
                           static_cast<int64_t>((config.memSpec->tCK * config.windowSize).value()));
    else
        sqlite3_bind_int64(insertGeneralInfoStatement, 9, 0);
    sqlite3_bind_int(insertGeneralInfoStatement, 10, static_cast<int>(config.refreshMaxPostponed));
    sqlite3_bind_int(insertGeneralInfoStatement, 11, static_cast<int>(config.refreshMaxPulledin));
    sqlite3_bind_int(insertGeneralInfoStatement, 12, static_cast<int>(UINT_MAX));
    sqlite3_bind_int(insertGeneralInfoStatement, 13, static_cast<int>(config.requestBufferSize));
    sqlite3_bind_int(
        insertGeneralInfoStatement, 14, static_cast<int>(config.memSpec->getPer2BankOffset()));

    const MemSpec& memSpec = *config.memSpec;
    const auto memoryType = memSpec.memoryType;

    bool rowColumnCommandBus =
        (memoryType == MemSpec::MemoryType::HBM2) || (memoryType == MemSpec::MemoryType::HBM3);

    bool pseudoChannelMode = [&memSpec, memoryType]() -> bool
    {
        if (memoryType != MemSpec::MemoryType::HBM2 && memoryType != MemSpec::MemoryType::HBM3)
            return false;

        return memSpec.pseudoChannelsPerChannel != 1;
    }();

    sqlite3_bind_int(insertGeneralInfoStatement, 15, static_cast<int>(rowColumnCommandBus));
    sqlite3_bind_int(insertGeneralInfoStatement, 16, static_cast<int>(pseudoChannelMode));
    executeSqlStatement(insertGeneralInfoStatement);
}

void TlmRecorder::insertCommandLengths()
{
    const MemSpec& _memSpec = *config.memSpec;

    auto insertCommandLength = [this, &_memSpec](Command command)
    {
        auto commandName = command.toString();

        sqlite3_bind_text(
            insertCommandLengthsStatement, 1, commandName.c_str(), commandName.length(), nullptr);
        sqlite3_bind_double(
            insertCommandLengthsStatement, 2, _memSpec.getCommandLengthInCycles(command));
        executeSqlStatement(insertCommandLengthsStatement);
    };

    for (unsigned int command = 0; command < Command::END_ENUM; ++command)
        insertCommandLength(static_cast<Command::Type>(command));
}

void TlmRecorder::insertTransactionInDB(const Transaction& recordingData)
{
    sqlite3_bind_int(insertTransactionStatement, 1, static_cast<int>(recordingData.id));
    sqlite3_bind_int(insertTransactionStatement, 2, static_cast<int>(recordingData.id));
    sqlite3_bind_int64(insertTransactionStatement, 3, static_cast<int64_t>(recordingData.address));
    sqlite3_bind_int(insertTransactionStatement, 4, static_cast<int>(recordingData.dataLength));
    sqlite3_bind_int(insertTransactionStatement, 5, static_cast<int>(recordingData.thread));
    sqlite3_bind_int(insertTransactionStatement, 6, static_cast<int>(recordingData.channel));
    sqlite3_bind_int64(insertTransactionStatement,
                       7,
                       static_cast<int64_t>(recordingData.timeOfGeneration.value()));
    sqlite3_bind_text(insertTransactionStatement, 8, &recordingData.cmd, 1, nullptr);

    executeSqlStatement(insertTransactionStatement);
}

void TlmRecorder::insertRangeInDB(uint64_t id, const sc_time& begin, const sc_time& end)
{
    sqlite3_bind_int64(insertRangeStatement, 1, static_cast<int64_t>(id));
    sqlite3_bind_int64(insertRangeStatement, 2, static_cast<int64_t>(begin.value()));
    sqlite3_bind_int64(insertRangeStatement, 3, static_cast<int64_t>(end.value()));
    executeSqlStatement(insertRangeStatement);
}

void TlmRecorder::insertPhaseInDB(const Transaction::Phase& phase, uint64_t transactionID)
{
    sqlite3_bind_text(insertPhaseStatement,
                      1,
                      phase.name.c_str(),
                      static_cast<int>(phase.name.length()),
                      nullptr);
    sqlite3_bind_int64(insertPhaseStatement, 2, static_cast<int64_t>(phase.interval.start.value()));
    sqlite3_bind_int64(insertPhaseStatement, 3, static_cast<int64_t>(phase.interval.end.value()));
    sqlite3_bind_int64(
        insertPhaseStatement, 4, static_cast<int64_t>(phase.intervalOnDataStrobe.start.value()));
    sqlite3_bind_int64(
        insertPhaseStatement, 5, static_cast<int64_t>(phase.intervalOnDataStrobe.end.value()));
    sqlite3_bind_int(insertPhaseStatement, 6, static_cast<int>(phase.rank));
    sqlite3_bind_int(insertPhaseStatement, 7, static_cast<int>(phase.bankGroup));
    sqlite3_bind_int(insertPhaseStatement, 8, static_cast<int>(phase.bank));
    sqlite3_bind_int(insertPhaseStatement, 9, static_cast<int>(phase.row));
    sqlite3_bind_int(insertPhaseStatement, 10, static_cast<int>(phase.column));
    sqlite3_bind_int(insertPhaseStatement, 11, static_cast<int>(phase.burstLength));
    sqlite3_bind_int64(insertPhaseStatement, 12, static_cast<int64_t>(transactionID));
    executeSqlStatement(insertPhaseStatement);
}

void TlmRecorder::executeSqlStatement(sqlite3_stmt* statement)
{
    int errorCode = sqlite3_step(statement);
    if (errorCode != SQLITE_DONE)
        SC_REPORT_FATAL(
            "Error in TraceRecorder",
            (std::string("Could not execute statement. Error code: ") + std::to_string(errorCode))
                .c_str());

    sqlite3_reset(statement);
}

void TlmRecorder::executeInitialSqlCommand()
{
    PRINTDEBUGMESSAGE(name, "Creating database by running provided sql script");

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, initialCommand.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        SC_REPORT_FATAL("SQLITE Error", errMsg);
        sqlite3_free(errMsg);
    }

    PRINTDEBUGMESSAGE(name, "Database created successfully");
}

void TlmRecorder::closeConnection()
{
    terminateRemainingTransactions();
    if (storageThread.joinable())
        storageThread.join();
    std::swap(currentDataBuffer, storageDataBuffer);
    commitRecordedDataToDB();
    PRINTDEBUGMESSAGE(
        name, "Number of transactions written to DB: " + std::to_string(totalNumTransactions));
    PRINTDEBUGMESSAGE(name, "tlmPhaseRecorder:\tEnd Recording");
    sqlite3_close(db);
    db = nullptr;
}

} // namespace DRAMSys
