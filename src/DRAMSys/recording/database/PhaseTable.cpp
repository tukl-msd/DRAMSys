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

#include "PhaseTable.h"

#include "Database.h"

#include <sqlite3.h>
#include <sysc/kernel/sc_object.h>

namespace DRAMSys::Recording
{

constexpr char const* INSERT_PHASE_NAME_STRING = R"(
INSERT INTO phases (id, name, stage, type)
VALUES (?1, ?2, ?3, ?4)
)";

constexpr char const* CREATE_PHASE_NAME_TABLE_STRING = R"(
CREATE TABLE phases (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    stage TEXT NOT NULL,
    type TEXT NOT NULL
)
)";

PhaseTable::PhaseTable(Database& database) : database(database)
{
    database.execute(CREATE_PHASE_NAME_TABLE_STRING);

    sqlite3_prepare_v2(
        database.getDb(), INSERT_PHASE_NAME_STRING, -1, &insertPhaseNameStatement, nullptr);
}

PhaseTable::~PhaseTable()
{
    sqlite3_finalize(insertPhaseNameStatement);
}

uint64_t PhaseTable::record(PhaseRecord const& record)
{
    // Get the phase stage
    std::string stage_name(record.name);
    std::size_t delimiter_pos = stage_name.find('_');
    stage_name.erase(0, delimiter_pos + 1);

    // Use the BEGIN_ phase enum value as the index as its unique
    std::string_view phase_type = record.name.at(0) == 'B' ? "BEGIN" : "END";

    sqlite3_bind_int(insertPhaseNameStatement, 1, static_cast<int>(record.phase_id));
    sqlite3_bind_text(insertPhaseNameStatement,
                      2,
                      record.name.data(),
                      static_cast<int>(record.name.length()),
                      nullptr);
    sqlite3_bind_text(insertPhaseNameStatement,
                      3,
                      stage_name.c_str(),
                      static_cast<int>(stage_name.length()),
                      nullptr);
    sqlite3_bind_text(insertPhaseNameStatement,
                      4,
                      phase_type.data(),
                      static_cast<int>(phase_type.length()),
                      nullptr);

    Database::executeStatement(insertPhaseNameStatement);
    return sqlite3_last_insert_rowid(database.getDb());
}

} // namespace DRAMSys::Recording
