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

#include "DramStateTable.h"

#include "Database.h"

#include <sqlite3.h>

namespace DRAMSys::Recording
{

constexpr char const* INSERT_DRAM_STATE_STRING = R"(
INSERT INTO dram_states (begin, end, command_strobe_id)
VALUES (?1, ?2, ?3)
)";

constexpr char const* CREATE_DRAM_STATE_TABLE_STRING = R"(
CREATE TABLE dram_states (
    id INTEGER PRIMARY KEY,
    begin INTEGER NOT NULL,
    end INTEGER NOT NULL,
    command_strobe_id INTEGER REFERENCES command_strobes(id)
)
)";

DramStateTable::DramStateTable(Database& database) : database(database)
{
    database.execute(CREATE_DRAM_STATE_TABLE_STRING);

    sqlite3_prepare_v2(
        database.getDb(), INSERT_DRAM_STATE_STRING, -1, &insertDramStateStatement, nullptr);
}

DramStateTable::~DramStateTable()
{
    sqlite3_finalize(insertDramStateStatement);
}

void DramStateTable::record(DramStateRecord const& record)
{
    sqlite3_bind_int64(insertDramStateStatement, 1, static_cast<std::int64_t>(record.begin));
    sqlite3_bind_int64(insertDramStateStatement, 2, static_cast<std::int64_t>(record.end));
    sqlite3_bind_int(insertDramStateStatement, 3, static_cast<int>(record.command_strobe_id));

    Database::executeStatement(insertDramStateStatement);
}

} // namespace DRAMSys::Recording
