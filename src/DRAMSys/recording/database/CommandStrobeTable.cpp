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

#include "CommandStrobeTable.h"

#include "Database.h"

#include <limits>
#include <sqlite3.h>

namespace DRAMSys::Recording
{

constexpr char const* INSERT_COMMAND_STROBE_STRING = R"(
INSERT INTO command_strobes (begin, command_id, signal_id, transaction_id, rank, bank_group, bank, dual_bank, row, column)
VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)
)";

constexpr char const* CREATE_COMMAND_STROBE_TABLE_STRING = R"(
CREATE TABLE command_strobes (
    id INTEGER PRIMARY KEY,
    begin INTEGER NOT NULL,
    command_id INTEGER REFERENCES commands(id),
    signal_id INTEGER REFERENCES signals(id),
    transaction_id INTEGER REFERENCES transactions(ID),
    rank INTEGER NOT NULL,
    bank_group INTEGER NOT NULL,
    bank INTEGER NOT NULL,
    dual_bank INTEGER NOT NULL,
    row INTEGER NOT NULL,
    column INTEGER NOT NULL
)
)";

CommandStrobeTable::CommandStrobeTable(Database& database) : database(database)
{
    database.execute(CREATE_COMMAND_STROBE_TABLE_STRING);

    sqlite3_prepare_v2(
        database.getDb(), INSERT_COMMAND_STROBE_STRING, -1, &insertCommandStrobeStatement, nullptr);
}

CommandStrobeTable::~CommandStrobeTable()
{
    sqlite3_finalize(insertCommandStrobeStatement);
}

void CommandStrobeTable::record(CommandStrobeRecord const& record)
{
    sqlite3_bind_int64(insertCommandStrobeStatement, 1, static_cast<std::int64_t>(record.begin));
    sqlite3_bind_int(insertCommandStrobeStatement, 2, static_cast<std::int32_t>(record.command_id));
    sqlite3_bind_int(insertCommandStrobeStatement, 3, static_cast<std::int32_t>(record.signal_id));
    // Internal commands (refresh, power-down, ...) are not part of any transaction:
    // store NULL instead of the sentinel.
    if (record.transaction_id == std::numeric_limits<unsigned>::max())
        sqlite3_bind_null(insertCommandStrobeStatement, 4);
    else
        sqlite3_bind_int(
            insertCommandStrobeStatement, 4, static_cast<std::int32_t>(record.transaction_id));

    // Coordinate
    sqlite3_bind_int(insertCommandStrobeStatement, 5, static_cast<std::int32_t>(record.rank));
    sqlite3_bind_int(insertCommandStrobeStatement, 6, static_cast<std::int32_t>(record.bank_group));
    sqlite3_bind_int(insertCommandStrobeStatement, 7, static_cast<std::int32_t>(record.bank));
    sqlite3_bind_int(insertCommandStrobeStatement, 8, static_cast<std::int32_t>(record.dual_bank));
    sqlite3_bind_int(insertCommandStrobeStatement, 9, static_cast<std::int32_t>(record.row));
    sqlite3_bind_int(insertCommandStrobeStatement, 10, static_cast<std::int32_t>(record.column));

    Database::executeStatement(insertCommandStrobeStatement);
}

} // namespace DRAMSys::Recording
