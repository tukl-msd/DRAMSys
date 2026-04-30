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

#include "SignalTable.h"

#include "Database.h"

#include <sqlite3.h>

namespace DRAMSys::Recording
{

constexpr char const* INSERT_SIGNAL_STRING = R"(
INSERT INTO signals (name, type, channel)
VALUES (?1, ?2, ?3)
)";

constexpr char const* CREATE_SIGNAL_TABLE_STRING = R"(
CREATE TABLE signals (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    channel INTEGER NOT NULL
)
)";

SignalTable::SignalTable(Database& database) : database(database)
{
    database.execute(CREATE_SIGNAL_TABLE_STRING);

    sqlite3_prepare_v2(database.getDb(), INSERT_SIGNAL_STRING, -1, &insertBusStatement, nullptr);
}

SignalTable::~SignalTable()
{
    sqlite3_finalize(insertBusStatement);
}

uint64_t SignalTable::record(SignalRecord const& record)
{
    const auto* type_name = [record]()
    {
        switch (record.type)
        {
        case SignalType::TLMBus:
            return "TLM";
        case SignalType::CommandBus:
            return "CMD";
        case SignalType::DataBus:
            return "DATA";
        }
        return "";
    }();

    sqlite3_bind_text(insertBusStatement,
                      1,
                      record.name.c_str(),
                      static_cast<int>(record.name.length()),
                      nullptr);

    sqlite3_bind_text(insertBusStatement, 2, type_name, -1, nullptr);
    sqlite3_bind_int(insertBusStatement, 3, static_cast<int>(record.channel));

    Database::executeStatement(insertBusStatement);
    return sqlite3_last_insert_rowid(database.getDb());
}

} // namespace DRAMSys::Recording
