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

#ifndef DRAMSYSCONFIGURATION_CONFIGURATION_H
#define DRAMSYSCONFIGURATION_CONFIGURATION_H

#include "AddressMapping.h"
#include "McConfig.h"
#include "SimConfig.h"
#include "ThermalConfig.h"
#include "TraceSetup.h"
#include "memspec/MemSpec.h"
#include "util.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

/**
 * To support polymorphic configurations, a Json "type" tag is used
 * to determine the correct type before further parsing.
 *
 * To support optional values, std::optional is used. The default
 * values will be provided by DRAMSys itself.
 *
 * To achieve static polymorphism, std::variant is used.
 */

namespace DRAMSysConfiguration
{
using json = nlohmann::json;

struct Configuration
{
    AddressMapping addressMapping;
    McConfig mcConfig;
    MemSpec memSpec;
    SimConfig simConfig;
    std::string simulationId;
    std::optional<ThermalConfig> thermalConfig;
    std::optional<TraceSetup> traceSetup;

    static std::string resourceDirectory;
};

void to_json(json &j, const Configuration &p);
void from_json(const json &j, Configuration &p);

void from_dump(const std::string &dump, Configuration &c);
std::string dump(const Configuration &c, unsigned int indentation = -1);

Configuration from_path(const std::string &path, const std::string &resourceDirectory = DRAMSysResourceDirectory);

} // namespace DRAMSysConfiguration

#endif // DRAMSYSCONFIGURATION_CONFIGURATION_H
