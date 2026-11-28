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

#include "Database.h"

#include <sqlite3.h>
#include <systemc>

namespace DRAMSys::Recording
{

Database::Database(std::filesystem::path const& path)
{
    if (std::filesystem::exists(path))
    {
        SC_REPORT_WARNING("Database", "Removing already existing database file");
        std::filesystem::remove(path);
    }

    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
    {
        sqlite3_close(db);
        auto const* errmsg = sqlite3_errmsg(db);
        SC_REPORT_FATAL("Database", errmsg);
    }

    execute("PRAGMA main.page_size=4096");
    execute("PRAGMA main.cache_size=10000");
    execute("PRAGMA main.locking_mode=EXCLUSIVE");
    execute("PRAGMA main.synchronous=OFF");
    execute("PRAGMA journal_mode=OFF");
}

Database::~Database()
{
    sqlite3_close(db);
}

void Database::execute(const char* query)
{
    char* errmsg{};
    if (sqlite3_exec(db, query, nullptr, nullptr, &errmsg) != SQLITE_OK)
    {
        SC_REPORT_WARNING("Database", errmsg);
        sqlite3_free(errmsg);
    }
};

void Database::executeStatement(sqlite3_stmt* statement)
{
    int rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        SC_REPORT_WARNING("Database", sqlite3_errstr(rc));
    }

    sqlite3_reset(statement);
}

} // namespace DRAMSys::Recording
