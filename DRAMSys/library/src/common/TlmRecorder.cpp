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
 *    Derek Christ
 *    Lukas Steiner
 *    Derek Christ
 */

#include <fstream>

#include "TlmRecorder.h"
#include "DebugManager.h"
#include "../controller/Command.h"
#include "../configuration/Configuration.h"

using namespace sc_core;
using namespace tlm;

TlmRecorder::TlmRecorder(const std::string& name, const Configuration& config, const std::string& dbName) :
        name(name), config(config), totalNumTransactions(0), simulationTimeCoveredByRecording(SC_ZERO_TIME)
{
    currentDataBuffer = &recordingDataBuffer[0];
    storageDataBuffer = &recordingDataBuffer[1];

    currentDataBuffer->reserve(transactionCommitRate);
    storageDataBuffer->reserve(transactionCommitRate);

    openDB(dbName);
    char *sErrMsg;
    sqlite3_exec(db, "PRAGMA main.page_size = 4096", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA main.cache_size=10000", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA main.locking_mode=EXCLUSIVE", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA main.synchronous=OFF", nullptr, nullptr, &sErrMsg);
    sqlite3_exec(db, "PRAGMA journal_mode = OFF", nullptr, nullptr, &sErrMsg);

    executeInitialSqlCommand();
    prepareSqlStatements();

    PRINTDEBUGMESSAGE(name, "Starting new database transaction");
}

void TlmRecorder::finalize()
{
    if (db)
        closeConnection();
    sqlite3_finalize(insertTransactionStatement);
    sqlite3_finalize(insertRangeStatement);
    sqlite3_finalize(updateRangeStatement);
    sqlite3_finalize(insertPhaseStatement);
    sqlite3_finalize(updatePhaseStatement);
    sqlite3_finalize(insertGeneralInfoStatement);
    sqlite3_finalize(insertCommandLengthsStatement);
    sqlite3_finalize(insertDebugMessageStatement);
    sqlite3_finalize(updateDataStrobeStatement);
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

void TlmRecorder::recordBufferDepth(double timeInSeconds, const std::vector<double> &averageBufferDepth)
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

void TlmRecorder::recordPhase(tlm_generic_payload &trans,
                              const tlm_phase &phase, const sc_time &time)
{
    if (currentTransactionsInSystem.find(&trans) == currentTransactionsInSystem.end())
        introduceTransactionSystem(trans);

    if (phase == END_REQ || phase == END_RESP || phase >= END_PDNA)
    {
        assert(getPhaseName(phase).substr(4) == currentTransactionsInSystem[&trans].recordedPhases.back().name);
        currentTransactionsInSystem[&trans].recordedPhases.back().interval.end = time;
    }
    else
    {
        std::string phaseName = getPhaseName(phase).substr(6); // remove "BEGIN_"
        currentTransactionsInSystem[&trans].recordedPhases.emplace_back(phaseName, time);
    }

    if (currentTransactionsInSystem[&trans].cmd == 'X')
    {
        if (phase == END_REFAB
                || phase == END_RFMAB
                || phase == END_REFPB
                || phase == END_RFMPB
                || phase == END_REFP2B
                || phase == END_RFMP2B
                || phase == END_REFSB
                || phase == END_RFMSB
                || phase == END_PDNA
                || phase == END_PDNP
                || phase == END_SREF)
            removeTransactionFromSystem(trans);
    }
    else
    {
        if (phase == END_RESP)
            removeTransactionFromSystem(trans);
    }

    simulationTimeCoveredByRecording = time;
}


void TlmRecorder::updateDataStrobe(const sc_time &begin, const sc_time &end,
                                   tlm_generic_payload &trans)
{
    assert(currentTransactionsInSystem.count(&trans) != 0);
    currentTransactionsInSystem[&trans].timeOnDataStrobe.start = begin;
    currentTransactionsInSystem[&trans].timeOnDataStrobe.end = end;
}


void TlmRecorder::recordDebugMessage(const std::string &message, const sc_time &time)
{
    insertDebugMessageInDB(message, time);
}


// ------------- internal -----------------------

void TlmRecorder::introduceTransactionSystem(tlm_generic_payload &trans)
{
    totalNumTransactions++;
    currentTransactionsInSystem[&trans].id = totalNumTransactions;
    tlm_command command = trans.get_command();
    if (command == TLM_READ_COMMAND)
        currentTransactionsInSystem[&trans].cmd = 'R';
    else if (command == TLM_WRITE_COMMAND)
        currentTransactionsInSystem[&trans].cmd = 'W';
    else
        currentTransactionsInSystem[&trans].cmd = 'X';
    currentTransactionsInSystem[&trans].address = trans.get_address();
    currentTransactionsInSystem[&trans].burstLength = DramExtension::getBurstLength(trans);
    currentTransactionsInSystem[&trans].dramExtension = DramExtension::getExtension(trans);
    currentTransactionsInSystem[&trans].timeOfGeneration = GenerationExtension::getTimeOfGeneration(trans);

    PRINTDEBUGMESSAGE(name, "New transaction #" + std::to_string(totalNumTransactions) + " generation time " +
                      currentTransactionsInSystem[&trans].timeOfGeneration.to_string());
}

void TlmRecorder::removeTransactionFromSystem(tlm_generic_payload &trans)
{
    assert(currentTransactionsInSystem.count(&trans) != 0);

    PRINTDEBUGMESSAGE(name, "Removing transaction #" +
                      std::to_string(currentTransactionsInSystem[&trans].id));

    Transaction &recordingData = currentTransactionsInSystem[&trans];
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
        auto transaction = std::min_element(currentTransactionsInSystem.begin(),
                currentTransactionsInSystem.end(), [](decltype(currentTransactionsInSystem)::value_type& l,
                decltype(currentTransactionsInSystem)::value_type& r) -> bool {return l.second.id < r.second.id;});
        if (transaction->second.cmd == 'X')
        {
            std::string beginPhase = transaction->second.recordedPhases.front().name;
            if (beginPhase == "PDNA")
                recordPhase(*(transaction->first), END_PDNA, sc_time_stamp());
            else if (beginPhase == "PDNP")
                recordPhase(*(transaction->first), END_PDNP, sc_time_stamp());
            else if (beginPhase == "SREF")
                recordPhase(*(transaction->first), END_SREF, sc_time_stamp());
            else
                removeTransactionFromSystem(*transaction->first);
        }
        else
            recordPhase(*(transaction->first), END_RESP, sc_time_stamp());
    }
}

void TlmRecorder::commitRecordedDataToDB()
{
    sqlite3_exec(db, "BEGIN;", nullptr, nullptr, nullptr);
    for (Transaction &recordingData : *storageDataBuffer)
    {
        assert(!recordingData.recordedPhases.empty());
        insertTransactionInDB(recordingData);
        for (Transaction::Phase &phaseData : recordingData.recordedPhases)
        {
            insertPhaseInDB(phaseData.name, phaseData.interval.start,
                            phaseData.interval.end, recordingData.id);
        }

        sc_time rangeBegin = recordingData.recordedPhases.front().interval.start;
        sc_time rangeEnd = rangeBegin;
        for (auto &it : recordingData.recordedPhases)
        {
            rangeEnd = std::max(rangeEnd, it.interval.end);
        }
        insertRangeInDB(recordingData.id, rangeBegin, rangeEnd);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

void TlmRecorder::openDB(const std::string &dbName)
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
            "INSERT INTO Transactions VALUES (:id,:rangeID,:address,:burstlength,:thread,:channel,:rank,"
            ":bankgroup,:bank,:row,:column,:dataStrobeBegin,:dataStrobeEnd, :timeOfGeneration,:command)";

    insertRangeString = "INSERT INTO Ranges VALUES (:id,:begin,:end)";

    updateRangeString = "UPDATE Ranges SET  End = :end WHERE ID = :id";

    updateDataStrobeString =
            "UPDATE Transactions SET DataStrobeBegin = :begin, DataStrobeEnd = :end WHERE ID = :id";

    insertPhaseString =
            "INSERT INTO Phases (PhaseName,PhaseBegin,PhaseEnd,Transact) VALUES (:name,:begin,:end,:transaction)";

    updatePhaseString =
            "UPDATE Phases SET PhaseEnd = :end WHERE Transact = :trans AND PhaseName = :name";

    insertGeneralInfoString =
        "INSERT INTO GeneralInfo VALUES"
        "(:numberOfTransactions, :end, :numberOfRanks, :numberOfBankGroups, :numberOfBanks, :clk, :unitOfTime, "
        ":mcconfig, :memspec, :traces, :windowSize, :refreshMaxPostponed, :refreshMaxPulledin, :controllerThread, "
        ":maxBufferDepth, :per2BankOffset, :rowColumnCommandBus, :pseudoChannelMode)";

    insertCommandLengthsString = "INSERT INTO CommandLengths VALUES"
                                 "(:command, :length)";

    insertDebugMessageString =
            "INSERT INTO DebugMessages (Time,Message) Values (:time,:message)";

    insertPowerString = "INSERT INTO Power VALUES (:time,:averagePower)";
    insertBufferDepthString = "INSERT INTO BufferDepth VALUES (:time,:bufferNumber,:averageBufferDepth)";
    insertBandwidthString = "INSERT INTO Bandwidth VALUES (:time,:averageBandwidth)";

    sqlite3_prepare_v2(db, insertTransactionString.c_str(), -1, &insertTransactionStatement, nullptr);
    sqlite3_prepare_v2(db, insertRangeString.c_str(), -1, &insertRangeStatement, nullptr);
    sqlite3_prepare_v2(db, updateRangeString.c_str(), -1, &updateRangeStatement, nullptr);
    sqlite3_prepare_v2(db, insertPhaseString.c_str(), -1, &insertPhaseStatement, nullptr);
    sqlite3_prepare_v2(db, updatePhaseString.c_str(), -1, &updatePhaseStatement, nullptr);
    sqlite3_prepare_v2(db, updateDataStrobeString.c_str(), -1, &updateDataStrobeStatement, nullptr);
    sqlite3_prepare_v2(db, insertGeneralInfoString.c_str(), -1, &insertGeneralInfoStatement, nullptr);
    sqlite3_prepare_v2(db, insertCommandLengthsString.c_str(), -1, &insertCommandLengthsStatement, nullptr);
    sqlite3_prepare_v2(db, insertDebugMessageString.c_str(), -1, &insertDebugMessageStatement, nullptr);
    sqlite3_prepare_v2(db, insertPowerString.c_str(), -1, &insertPowerStatement, nullptr);
    sqlite3_prepare_v2(db, insertBufferDepthString.c_str(), -1, &insertBufferDepthStatement, nullptr);
    sqlite3_prepare_v2(db, insertBandwidthString.c_str(), -1, &insertBandwidthStatement, nullptr);
}

void TlmRecorder::insertDebugMessageInDB(const std::string &message, const sc_time &time)
{
    sqlite3_bind_int64(insertDebugMessageStatement, 1, static_cast<int64_t>(time.value()));
    sqlite3_bind_text(insertDebugMessageStatement, 2, message.c_str(), static_cast<int>(message.length()), nullptr);
    executeSqlStatement(insertDebugMessageStatement);
}

void TlmRecorder::insertGeneralInfo()
{
    sqlite3_bind_int64(insertGeneralInfoStatement, 1, static_cast<int64_t>(totalNumTransactions));
    sqlite3_bind_int64(insertGeneralInfoStatement, 2, static_cast<int64_t>(simulationTimeCoveredByRecording.value()));
    sqlite3_bind_int(insertGeneralInfoStatement, 3, static_cast<int>(config.memSpec->ranksPerChannel));
    sqlite3_bind_int(insertGeneralInfoStatement, 4, static_cast<int>(config.memSpec->bankGroupsPerChannel));
    sqlite3_bind_int(insertGeneralInfoStatement, 5, static_cast<int>(config.memSpec->banksPerChannel));
    sqlite3_bind_int64(insertGeneralInfoStatement, 6, static_cast<int64_t>(config.memSpec->tCK.value()));
    sqlite3_bind_text(insertGeneralInfoStatement, 7, "PS", 2, nullptr);

    sqlite3_bind_text(insertGeneralInfoStatement, 8, mcconfig.c_str(), static_cast<int>(mcconfig.length()), nullptr);
    sqlite3_bind_text(insertGeneralInfoStatement, 9, memspec.c_str(), static_cast<int>(memspec.length()), nullptr);
    sqlite3_bind_text(insertGeneralInfoStatement, 10, traces.c_str(), static_cast<int>(traces.length()), nullptr);
    if (config.enableWindowing)
        sqlite3_bind_int64(insertGeneralInfoStatement, 11, static_cast<int64_t>((config.memSpec->tCK
                * config.windowSize).value()));
    else
        sqlite3_bind_int64(insertGeneralInfoStatement, 11, 0);
    sqlite3_bind_int(insertGeneralInfoStatement, 12, static_cast<int>(config.refreshMaxPostponed));
    sqlite3_bind_int(insertGeneralInfoStatement, 13, static_cast<int>(config.refreshMaxPulledin));
    sqlite3_bind_int64(insertGeneralInfoStatement, 14, static_cast<int64_t>(UINT64_MAX));
    sqlite3_bind_int(insertGeneralInfoStatement, 15, static_cast<int>(config.requestBufferSize));
    sqlite3_bind_int(insertGeneralInfoStatement, 16, static_cast<int>(config.memSpec->getPer2BankOffset()));

    const MemSpec& memSpec = *config.memSpec;
    const auto memoryType = memSpec.memoryType;
    bool rowColumnCommandBus = [memoryType]() -> bool {
        if (memoryType == MemSpec::MemoryType::HBM2)
            return true;
        else
            return false;
    }();

    bool pseudoChannelMode = [&memSpec, memoryType]() -> bool {
        if (memoryType != MemSpec::MemoryType::HBM2)
            return false;

        if (memSpec.pseudoChannelsPerChannel != 1)
            return true;
        else
            return false;
    }();

    sqlite3_bind_int(insertGeneralInfoStatement, 17, static_cast<int>(rowColumnCommandBus));
    sqlite3_bind_int(insertGeneralInfoStatement, 18, static_cast<int>(pseudoChannelMode));
    executeSqlStatement(insertGeneralInfoStatement);
}

void TlmRecorder::insertCommandLengths()
{
    const MemSpec& memSpec = *config.memSpec;

    auto insertCommandLength = [this, &memSpec](Command command) {
        auto commandName = command.toString();

        sqlite3_bind_text(insertCommandLengthsStatement, 1, commandName.c_str(), commandName.length(), nullptr);
        sqlite3_bind_int(insertCommandLengthsStatement, 2,
                         static_cast<int>(lround(memSpec.getCommandLength(command) / memSpec.tCK)));
        executeSqlStatement(insertCommandLengthsStatement);
    };

    for (unsigned int command = 0; command < Command::END_ENUM; ++command)
        insertCommandLength(static_cast<Command::Type>(command));
}

void TlmRecorder::insertTransactionInDB(Transaction &recordingData)
{
    sqlite3_bind_int(insertTransactionStatement, 1, static_cast<int>(recordingData.id));
    sqlite3_bind_int(insertTransactionStatement, 2, static_cast<int>(recordingData.id));
    sqlite3_bind_int64(insertTransactionStatement, 3, static_cast<int64_t>(recordingData.address));
    sqlite3_bind_int(insertTransactionStatement, 4, static_cast<int>(recordingData.burstLength));
    sqlite3_bind_int(insertTransactionStatement, 5,
                     static_cast<int>(recordingData.dramExtension.getThread().ID()));
    sqlite3_bind_int(insertTransactionStatement, 6,
                     static_cast<int>(recordingData.dramExtension.getChannel().ID()));
    sqlite3_bind_int(insertTransactionStatement, 7,
                     static_cast<int>(recordingData.dramExtension.getRank().ID()));
    sqlite3_bind_int(insertTransactionStatement, 8,
                     static_cast<int>(recordingData.dramExtension.getBankGroup().ID()));
    sqlite3_bind_int(insertTransactionStatement, 9,
                     static_cast<int>(recordingData.dramExtension.getBank().ID()));
    sqlite3_bind_int(insertTransactionStatement, 10,
                     static_cast<int>(recordingData.dramExtension.getRow().ID()));
    sqlite3_bind_int(insertTransactionStatement, 11,
                     static_cast<int>(recordingData.dramExtension.getColumn().ID()));
    sqlite3_bind_int64(insertTransactionStatement, 12,
                       static_cast<int64_t>(recordingData.timeOnDataStrobe.start.value()));
    sqlite3_bind_int64(insertTransactionStatement, 13,
                       static_cast<int64_t>(recordingData.timeOnDataStrobe.end.value()));
    sqlite3_bind_int64(insertTransactionStatement, 14,
                       static_cast<int64_t>(recordingData.timeOfGeneration.value()));
    sqlite3_bind_text(insertTransactionStatement, 15,
                      &recordingData.cmd, 1, nullptr);

    executeSqlStatement(insertTransactionStatement);
}

void TlmRecorder::insertRangeInDB(uint64_t id, const sc_time &begin,
                                  const sc_time &end)
{
    sqlite3_bind_int64(insertRangeStatement, 1, static_cast<int64_t>(id));
    sqlite3_bind_int64(insertRangeStatement, 2, static_cast<int64_t>(begin.value()));
    sqlite3_bind_int64(insertRangeStatement, 3, static_cast<int64_t>(end.value()));
    executeSqlStatement(insertRangeStatement);
}

void TlmRecorder::insertPhaseInDB(const std::string &phaseName, const sc_time &begin,
                                  const sc_time &end,
                                  uint64_t transactionID)
{
    sqlite3_bind_text(insertPhaseStatement, 1, phaseName.c_str(),
                      static_cast<int>(phaseName.length()), nullptr);
    sqlite3_bind_int64(insertPhaseStatement, 2, static_cast<int64_t>(begin.value()));
    sqlite3_bind_int64(insertPhaseStatement, 3, static_cast<int64_t>(end.value()));
    sqlite3_bind_int64(insertPhaseStatement, 4, static_cast<int64_t>(transactionID));
    executeSqlStatement(insertPhaseStatement);
}


void TlmRecorder::executeSqlStatement(sqlite3_stmt *statement)
{
    int errorCode = sqlite3_step(statement);
    if (errorCode != SQLITE_DONE)
        SC_REPORT_FATAL("Error in TraceRecorder",
                        (std::string("Could not execute statement. Error code: ") + std::to_string(errorCode)).c_str());

    sqlite3_reset(statement);
}

void TlmRecorder::executeInitialSqlCommand()
{
    PRINTDEBUGMESSAGE(name, "Creating database by running provided sql script");

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, initialCommand.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
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
    insertGeneralInfo();
    insertCommandLengths();
    PRINTDEBUGMESSAGE(name, "Number of transactions written to DB: "
                      + std::to_string(totalNumTransactions));
    PRINTDEBUGMESSAGE(name, "tlmPhaseRecorder:\tEnd Recording");
    sqlite3_close(db);
    db = nullptr;
}
