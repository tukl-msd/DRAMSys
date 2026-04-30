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

#include "GeneralInfoTable.h"

#include "Database.h"

#include <sqlite3.h>
#include <systemc>

#include <cstdint>

namespace DRAMSys::Recording
{

constexpr char const* INSERT_GENERAL_INFO_STRING = R"(
INSERT INTO general_info (key, value)
VALUES (?1, ?2)
)";

constexpr char const* CREATE_GENERAL_INFO_TABLE_STRING = R"(
CREATE TABLE general_info (
    key TEXT PRIMARY KEY,
    value
)
)";

GeneralInfoTable::GeneralInfoTable(Database& database) : database(database)
{
    database.execute(CREATE_GENERAL_INFO_TABLE_STRING);

    sqlite3_prepare_v2(
        database.getDb(), INSERT_GENERAL_INFO_STRING, -1, &insertGeneralInfoStatement, nullptr);

    record(GeneralInfoRecord{"DatabaseVersion", "0.1.0"});
}

GeneralInfoTable::~GeneralInfoTable()
{
    sqlite3_finalize(insertGeneralInfoStatement);
}

void GeneralInfoTable::record(GeneralInfoRecord const& record)
{
    sqlite3_bind_text(insertGeneralInfoStatement,
                      1,
                      record.key.c_str(),
                      static_cast<int>(record.key.length()),
                      nullptr);

    if (record.value.type() == typeid(int))
    {
        sqlite3_bind_int(insertGeneralInfoStatement, 2, std::any_cast<int>(record.value));
    }
    else if (record.value.type() == typeid(bool))
    {
        sqlite3_bind_int(insertGeneralInfoStatement, 2, std::any_cast<bool>(record.value) ? 1 : 0);
    }
    else if (record.value.type() == typeid(unsigned))
    {
        sqlite3_bind_int64(insertGeneralInfoStatement,
                           2,
                           static_cast<sqlite3_int64>(std::any_cast<unsigned>(record.value)));
    }
    else if (record.value.type() == typeid(std::uint64_t))
    {
        sqlite3_bind_int64(insertGeneralInfoStatement,
                           2,
                           static_cast<sqlite3_int64>(std::any_cast<std::uint64_t>(record.value)));
    }
    else if (record.value.type() == typeid(std::string))
    {
        auto value = std::any_cast<std::string>(record.value);
        sqlite3_bind_text(
            insertGeneralInfoStatement, 2, value.c_str(), -1, SQLITE_TRANSIENT); // NOLINT
    }
    else if (record.value.type() == typeid(const char*))
    {
        const auto* value = std::any_cast<const char*>(record.value);
        sqlite3_bind_text(insertGeneralInfoStatement, 2, value, -1, SQLITE_TRANSIENT); // NOLINT
    }
    else
    {
        SC_REPORT_FATAL("GeneralInfoTable", "Unsupported value type!");
    }

    Database::executeStatement(insertGeneralInfoStatement);
}

} // namespace DRAMSys::Recording
