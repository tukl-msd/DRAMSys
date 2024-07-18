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

#ifndef DRAMSYSCONFIGURATION_MEMSPEC_H
#define DRAMSYSCONFIGURATION_MEMSPEC_H

#include "DRAMSys/config/memspec/MemArchitectureSpec.h"
#include "DRAMSys/config/memspec/MemPowerSpec.h"
#include "DRAMSys/config/memspec/MemTimingSpec.h"
#include "DRAMSys/util/json.h"

#include <optional>

namespace DRAMSys::Config
{

enum class MemoryType
{
    DDR3,
    DDR4,
    DDR5,
    LPDDR4,
    LPDDR5,
    WideIO,
    WideIO2,
    GDDR5,
    GDDR5X,
    GDDR6,
    HBM2,
    HBM3,
    STTMRAM,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryType,
                             {{MemoryType::Invalid, nullptr},
                              {MemoryType::DDR3, "DDR3"},
                              {MemoryType::DDR4, "DDR4"},
                              {MemoryType::DDR5, "DDR5"},
                              {MemoryType::LPDDR4, "LPDDR4"},
                              {MemoryType::LPDDR5, "LPDDR5"},
                              {MemoryType::WideIO, "WIDEIO_SDR"},
                              {MemoryType::WideIO2, "WIDEIO2"},
                              {MemoryType::GDDR5, "GDDR5"},
                              {MemoryType::GDDR5X, "GDDR5X"},
                              {MemoryType::GDDR6, "GDDR6"},
                              {MemoryType::HBM2, "HBM2"},
                              {MemoryType::HBM3, "HBM3"},
                              {MemoryType::STTMRAM, "STTMRAM"}})

struct MemSpec
{
    static constexpr std::string_view KEY = "memspec";
    static constexpr std::string_view SUB_DIR = "memspec";

    MemArchitectureSpecType memarchitecturespec;
    std::string memoryId;
    MemoryType memoryType;
    MemTimingSpecType memtimingspec;
    std::optional<MemPowerSpec> mempowerspec;
};

NLOHMANN_JSONIFY_ALL_THINGS(
    MemSpec, memarchitecturespec, memoryId, memoryType, memtimingspec, mempowerspec)

} // namespace DRAMSys::Config

#endif // DRAMSYSCONFIGURATION_MEMSPEC_H
