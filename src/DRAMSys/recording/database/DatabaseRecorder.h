/*
 * Copyright (c) 2026, Julius-Maximilians-Universität Würzburg
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
 *    Derek Christ
 */

#pragma once

#include "BandwidthTable.h"
#include "CommandStrobeTable.h"
#include "CommandTable.h"
#include "DataStrobeTable.h"
#include "DramStateTable.h"
#include "GeneralInfoTable.h"
#include "NBTransportTable.h"
#include "PhaseTable.h"
#include "PowerTable.h"
#include "SignalTable.h"
#include "TransactionTable.h"

#include "DRAMSys/recording/TlmMonitor.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <variant>

class sqlite3;
class sqlite3_stmt;

namespace DRAMSys::Recording
{

class Database;

/**
 * Decouples the simulator from the SQLite database by buffering records in a FIFO queue that is
 * drained by a dedicated writer thread.
 *
 * The producer blocks while the queue is full (backpressure), so the
 * simulation throughput is limited by the database throughput instead of
 * the records falling behind indefinitely.
 *
 * All database access happens on the writer thread.
 *
 */
class DatabaseRecorder
{
public:
    explicit DatabaseRecorder(Database& database);
    DatabaseRecorder(const DatabaseRecorder&) = delete;
    DatabaseRecorder(DatabaseRecorder&&) = delete;
    DatabaseRecorder& operator=(const DatabaseRecorder&) = delete;
    DatabaseRecorder& operator=(DatabaseRecorder&&) = delete;
    ~DatabaseRecorder();

    uint64_t record(GeneralInfoRecord const& record);
    uint64_t record(TlmATRecord const& record);
    uint64_t record(TlmLTRecord const& record);
    uint64_t record(TlmDBGRecord const& record);
    uint64_t record(TransactionRecord const& record);
    uint64_t record(PhaseRecord const& record);
    uint64_t record(SignalRecord const& record);
    uint64_t record(CommandRecord const& record);
    uint64_t record(CommandStrobeRecord const& record);
    uint64_t record(DataStrobeRecord const& record);
    uint64_t record(DramStateRecord const& record);
    uint64_t record(BandwidthRecord const& record);
    uint64_t record(PowerRecord const& record);

private:
    using AsyncRecord = std::variant<GeneralInfoRecord,
                                     TlmATRecord,
                                     TransactionRecord,
                                     PhaseRecord,
                                     SignalRecord,
                                     CommandRecord,
                                     CommandStrobeRecord,
                                     DataStrobeRecord,
                                     DramStateRecord,
                                     BandwidthRecord,
                                     PowerRecord>;

    /// Backpressure: the producer blocks while at least this many records are pending.
    static constexpr std::size_t MAX_QUEUE_ENTRIES = 1024;

    /// The number of records the writer persists per SQLite transaction to amortize commit costs.
    static constexpr std::size_t COMMIT_BATCH_SIZE = 512;

    /// The window the writer keeps open to fill a batch before committing a partial one.
    static constexpr std::chrono::milliseconds BATCH_ACQUIRE_TIME{1};

    /// Enqueues a record, blocking while the queue is full (backpressure).
    template <typename Record>
    void enqueue(Record const& record)
    {
        {
            std::unique_lock lock(m);
            cvNotFull.wait(lock,
                           [&] { return queue.size() < MAX_QUEUE_ENTRIES || stopWriterThread; });
            queue.emplace_back(record);
        }
        cvNotEmpty.notify_one();
    }

    void consumerThread();
    void persist(AsyncRecord& record);

    // Record IDs are assigned on the producer thread, so a record can reference the
    // ID of an earlier recorded record before that record has been persisted.
    uint64_t currentTlmAtRecordId = 1;
    uint64_t transactionRecordId = 1;
    uint64_t commandStrobeRecordId = 1;
    uint64_t dataStrobeRecordId = 1;
    uint64_t dramStateRecordId = 1;
    uint64_t currentSignalRecordId = 1;

    GeneralInfoTable generalInfoTable;
    BandwidthTable bandwidthTable;
    PowerTable powerTable;
    TransactionTable transactionTable;
    NBTransportTable nbTransportTable;
    DataStrobeTable dataStrobeTable;
    DramStateTable dramStateTable;
    PhaseTable phaseTable;
    SignalTable signalTable;
    CommandTable commandTable;
    CommandStrobeTable commandStrobeTable;

    Database& database;

    // Synchronization between the single producer and the writer thread
    bool stopWriterThread = false;
    std::mutex m;
    std::condition_variable cvNotEmpty;
    std::condition_variable cvNotFull;
    std::deque<AsyncRecord> queue;

    // Declared last, so the writer thread is started only after all other
    // members have been initialized.
    std::thread writerThread;
};

} // namespace DRAMSys::Recording
