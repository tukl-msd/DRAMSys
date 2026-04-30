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

#include "DataStrobeTable.h"

#include "Database.h"

#include <sqlite3.h>

namespace DRAMSys::Recording
{

constexpr char const* INSERT_DATA_STROBE_STRING = R"(
INSERT INTO data_strobes (begin, end, burst_length, signal_id, command_strobe_id)
VALUES (?1, ?2, ?3, ?4, ?5)
)";

constexpr char const* CREATE_DATA_STROBE_TABLE_STRING = R"(
CREATE TABLE data_strobes (
    id INTEGER PRIMARY KEY,
    begin INTEGER NOT NULL,
    end INTEGER NOT NULL,
    burst_length INTEGER NOT NULL,
    signal_id INTEGER REFERENCES signals(id),
    command_strobe_id INTEGER REFERENCES command_strobes(id)
)
)";

DataStrobeTable::DataStrobeTable(Database& database) : database(database)
{
    database.execute(CREATE_DATA_STROBE_TABLE_STRING);

    sqlite3_prepare_v2(
        database.getDb(), INSERT_DATA_STROBE_STRING, -1, &insertDataStrobeStatement, nullptr);
}

DataStrobeTable::~DataStrobeTable()
{
    sqlite3_finalize(insertDataStrobeStatement);
}

void DataStrobeTable::record(DataStrobeRecord const& record)
{
    sqlite3_bind_int64(insertDataStrobeStatement, 1, static_cast<std::int64_t>(record.begin));
    sqlite3_bind_int64(insertDataStrobeStatement, 2, static_cast<std::int64_t>(record.end));
    sqlite3_bind_int(insertDataStrobeStatement, 3, static_cast<int>(record.burst_length));
    sqlite3_bind_int(insertDataStrobeStatement, 4, static_cast<int>(record.signal_id));
    sqlite3_bind_int(insertDataStrobeStatement, 5, static_cast<int>(record.command_strobe_id));

    Database::executeStatement(insertDataStrobeStatement);
}

} // namespace DRAMSys::Recording
