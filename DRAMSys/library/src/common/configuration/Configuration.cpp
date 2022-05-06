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

#include "Configuration.h"

#include <exception>
#include <fstream>

namespace DRAMSysConfiguration
{

std::string Configuration::resourceDirectory;

void to_json(json &j, const Configuration &c)
{
    j = json{{"addressmapping", c.addressMapping}, {"mcconfig", c.mcConfig},   {"memspec", c.memSpec},
             {"simulationid", c.simulationId},     {"simconfig", c.simConfig}, {"thermalconfig", c.thermalConfig},
             {"tracesetup", c.traceSetup}};

    remove_null_values(j);
}

void from_json(const json &j, Configuration &c)
{
    j.at("addressmapping").get_to(c.addressMapping);
    j.at("mcconfig").get_to(c.mcConfig);
    j.at("memspec").get_to(c.memSpec);
    j.at("simulationid").get_to(c.simulationId);
    j.at("simconfig").get_to(c.simConfig);

    if (j.contains("thermalconfig"))
        j.at("thermalconfig").get_to(c.thermalConfig);

    if (j.contains("tracesetup"))
        j.at("tracesetup").get_to(c.traceSetup);
}

void from_dump(const std::string &dump, Configuration &c)
{
    json json_simulation = json::parse(dump).at("simulation");
    json_simulation.get_to(c);
}

std::string dump(const Configuration &c, unsigned int indentation)
{
    json json_simulation;
    json_simulation["simulation"] = c;
    return json_simulation.dump(indentation);
}

Configuration from_path(const std::string &path, const std::string &resourceDirectory)
{
    Configuration::resourceDirectory = resourceDirectory;

    std::ifstream file(path);

    if (file.is_open())
    {
        json simulation = json::parse(file).at("simulation");
        return simulation.get<DRAMSysConfiguration::Configuration>();
    }
    else
    {
        throw std::runtime_error("Failed to open file " + path);
    }
}

} // namespace DRAMSysConfiguration
