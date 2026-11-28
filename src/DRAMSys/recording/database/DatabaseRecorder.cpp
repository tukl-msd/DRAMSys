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

#include "DatabaseRecorder.h"

#include "Database.h"

#include <type_traits>
#include <utility>
#include <vector>

namespace DRAMSys::Recording
{

DatabaseRecorder::DatabaseRecorder(Database& database) :
    generalInfoTable(database),
    bandwidthTable(database),
    powerTable(database),
    transactionTable(database),
    nbTransportTable(database),
    dataStrobeTable(database),
    dramStateTable(database),
    phaseTable(database),
    signalTable(database),
    commandTable(database),
    commandStrobeTable(database),
    database(database),
    writerThread(&DatabaseRecorder::consumerThread, this)
{
}

/**
 * Stops the writer thread and drains the queue.
 * The writer thread exits only once the queue is empty, so no records are lost.
 */
DatabaseRecorder::~DatabaseRecorder()
{
    {
        std::scoped_lock lock(m);
        stopWriterThread = true;
    }

    cvNotEmpty.notify_all();
    cvNotFull.notify_all();

    if (writerThread.joinable())
        writerThread.join();
}

uint64_t DatabaseRecorder::record(GeneralInfoRecord const& record)
{
    enqueue(record);
    return 0;
}

uint64_t DatabaseRecorder::record(TlmATRecord const& record)
{
    auto id = currentTlmAtRecordId++;
    enqueue(record);
    return id;
}

uint64_t DatabaseRecorder::record(TlmLTRecord const& record)
{
    // TODO: b_transport records are not persisted yet.
    (void)record;
    return 0;
}

uint64_t DatabaseRecorder::record(TlmDBGRecord const& record)
{
    // TODO: transport_dbg records are not persisted yet.
    (void)record;
    return 0;
}

uint64_t DatabaseRecorder::record(TransactionRecord const& record)
{
    auto id = transactionRecordId++;
    enqueue(record);
    return id;
}

uint64_t DatabaseRecorder::record(PhaseRecord const& record)
{
    enqueue(record);
    return record.phase_id;
}

uint64_t DatabaseRecorder::record(SignalRecord const& record)
{
    auto id = currentSignalRecordId++;
    enqueue(record);
    return id;
}

uint64_t DatabaseRecorder::record(CommandRecord const& record)
{
    enqueue(record);
    return record.command_id;
}

uint64_t DatabaseRecorder::record(CommandStrobeRecord const& record)
{
    auto id = commandStrobeRecordId++;
    enqueue(record);
    return id;
}

uint64_t DatabaseRecorder::record(DataStrobeRecord const& record)
{
    auto id = dataStrobeRecordId++;
    enqueue(record);
    return id;
}

uint64_t DatabaseRecorder::record(DramStateRecord const& record)
{
    auto id = dramStateRecordId++;
    enqueue(record);
    return id;
}

uint64_t DatabaseRecorder::record(BandwidthRecord const& record)
{
    enqueue(record);
    return 0;
}

uint64_t DatabaseRecorder::record(PowerRecord const& record)
{
    enqueue(record);
    return 0;
}

void DatabaseRecorder::consumerThread()
{
    std::vector<AsyncRecord> batch;
    batch.reserve(COMMIT_BATCH_SIZE);

    auto popToBatch = [&]
    {
        while (batch.size() < COMMIT_BATCH_SIZE && !queue.empty())
        {
            batch.push_back(std::move(queue.front()));
            queue.pop_front();
        }
    };

    std::unique_lock lock(m);
    for (;;)
    {
        // Wait until at least one record is queued or a stop was requested.
        cvNotEmpty.wait(lock, [&] { return stopWriterThread || !queue.empty(); });

        if (stopWriterThread && queue.empty())
            return; // all records flushed

        popToBatch();

        if (batch.size() < COMMIT_BATCH_SIZE && !stopWriterThread)
        {
            // Keep the acquire window open so that records arriving close in
            // time are committed in one batch instead of one per record
            // (each commit pays a disk-sync cost).
            cvNotEmpty.wait_for(
                lock, BATCH_ACQUIRE_TIME, [&] { return stopWriterThread || !queue.empty(); });
            popToBatch();
        }

        if (batch.empty())
            continue; // a stop was requested while nothing was pending

        lock.unlock();

        // Persist the batch in record order within a single SQLite transaction.
        database.execute("BEGIN;");
        for (auto& entry : batch)
            persist(entry);
        database.execute("COMMIT;");

        // Wake up a producer potentially blocked on backpressure: the popped records made space.
        cvNotFull.notify_all();

        batch.clear();
        lock.lock();
    }
}

void DatabaseRecorder::persist(AsyncRecord& record)
{
    std::visit(
        [this](auto& entry)
        {
            using T = std::decay_t<decltype(entry)>;
            if constexpr (std::is_same_v<T, GeneralInfoRecord>)
            {
                generalInfoTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, TlmATRecord>)
            {
                nbTransportTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, TransactionRecord>)
            {
                transactionTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, PhaseRecord>)
            {
                phaseTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, SignalRecord>)
            {
                signalTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, CommandRecord>)
            {
                commandTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, CommandStrobeRecord>)
            {
                commandStrobeTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, DataStrobeRecord>)
            {
                dataStrobeTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, BandwidthRecord>)
            {
                bandwidthTable.record(entry);
            }
            else if constexpr (std::is_same_v<T, PowerRecord>)
            {
                powerTable.record(entry);
            }
            else
            {
                static_assert(std::is_same_v<T, DramStateRecord>);
                dramStateTable.record(entry);
            }
        },
        record);
}

} // namespace DRAMSys::Recording
