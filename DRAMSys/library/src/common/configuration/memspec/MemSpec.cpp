/*
 * Copyright (c) 2021, Technische Universität Kaiserslautern
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

#include "MemSpec.h"

namespace DRAMSysConfiguration
{

void to_json(json &j, const MemSpec &c)
{
    j = json{{"memarchitecturespec", c.memArchitectureSpec},
             {"memoryId", c.memoryId},
             {"memoryType", c.memoryType},
             {"memtimingspec", c.memTimingSpec},
             {"mempowerspec", c.memPowerSpec}};

    remove_null_values(j);
}

void from_json(const json &j, MemSpec &c)
{
    json j_memspecs = get_config_json(j, memSpecPath, "memspec");

    j_memspecs.at("memarchitecturespec").get_to(c.memArchitectureSpec);
    j_memspecs.at("memoryId").get_to(c.memoryId);
    j_memspecs.at("memoryType").get_to(c.memoryType);
    j_memspecs.at("memtimingspec").get_to(c.memTimingSpec);

    if (j_memspecs.contains("mempowerspec"))
        j_memspecs.at("mempowerspec").get_to(c.memPowerSpec);
}

void from_dump(const std::string &dump, MemSpec &c)
{
    json json_memspec = json::parse(dump).at("memspec");
    json_memspec.get_to(c);
}

std::string dump(const MemSpec &c, unsigned int indentation)
{
    json json_memspec;
    json_memspec["memspec"] = c;
    return json_memspec.dump(indentation);
}

} // namespace Configuration
