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

#include "BandwidthTable.h"

#include "Database.h"

#include <sqlite3.h>

namespace DRAMSys::Recording
{

constexpr char const* INSERT_BANDWIDTH_STRING = R"(
INSERT INTO bandwidth (window, signal_id, bandwidth)
VALUES (?1, ?2, ?3)
)";

constexpr char const* CREATE_BANDWIDTH_TABLE_STRING = R"(
CREATE TABLE bandwidth (
    id INTEGER PRIMARY KEY,
    window INTEGER NOT NULL,
    signal_id INTEGER REFERENCES signals(id),
    bandwidth REAL NOT NULL
)
)";

BandwidthTable::BandwidthTable(Database& database) : database(database)
{
    database.execute(CREATE_BANDWIDTH_TABLE_STRING);

    sqlite3_prepare_v2(
        database.getDb(), INSERT_BANDWIDTH_STRING, -1, &insertBandwidthStatement, nullptr);
}

BandwidthTable::~BandwidthTable()
{
    sqlite3_finalize(insertBandwidthStatement);
}

void BandwidthTable::record(BandwidthRecord const& record)
{
    sqlite3_bind_int64(insertBandwidthStatement, 1, static_cast<std::int64_t>(record.window));
    sqlite3_bind_int(insertBandwidthStatement, 2, static_cast<int>(record.signal_id));
    sqlite3_bind_double(insertBandwidthStatement, 3, record.bandwidth);

    Database::executeStatement(insertBandwidthStatement);
}

} // namespace DRAMSys::Recording
