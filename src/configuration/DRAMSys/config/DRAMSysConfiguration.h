/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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

#ifndef DRAMSYSCONFIGURATION_DRAMSYSCONFIGURATION_H
#define DRAMSYSCONFIGURATION_DRAMSYSCONFIGURATION_H

#include "DRAMSys/config/AddressMapping.h"
#include "DRAMSys/config/McConfig.h"
#include "DRAMSys/config/SimConfig.h"
#include "DRAMSys/config/TraceSetup.h"
#include "DRAMSys/config/memspec/MemSpec.h"

#include <optional>
#include <string>

/**
 * To support optional values, std::optional is used. The default
 * values will be provided by DRAMSys itself.
 *
 * To achieve static polymorphism, std::variant is used. The concrete
 * type is determined by the provided fields automatically.
 *
 * To achieve backwards compatibility, this library manipulates the json
 * data type as it is parsed in to substitute paths to sub-configurations
 * with the actual json object that is stored at this path.
 */

namespace DRAMSys::Config
{

struct Configuration
{
    static constexpr std::string_view KEY = "simulation";

    AddressMapping addressmapping;
    McConfig mcconfig;
    MemSpec memspec;
    SimConfig simconfig;
    std::string simulationid;
    std::optional<std::vector<Initiator>> tracesetup;
};

NLOHMANN_JSONIFY_ALL_THINGS(
    Configuration, addressmapping, mcconfig, memspec, simconfig, simulationid, tracesetup)

Configuration from_path(std::string_view path,
                        std::string_view resourceDirectory = DRAMSYS_RESOURCE_DIR);

} // namespace DRAMSys::Config

#endif // DRAMSYSCONFIGURATION_DRAMSYSCONFIGURATION_H
