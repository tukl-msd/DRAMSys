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

#ifndef DRAMSYSCONFIGURATION_SIMCONFIG_H
#define DRAMSYSCONFIGURATION_SIMCONFIG_H

#include "DRAMSys/util/json.h"

#include <optional>

namespace DRAMSys::Config
{

enum class StoreModeType
{
    NoStorage,
    Store,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(StoreModeType,
                             {{StoreModeType::Invalid, nullptr},
                              {StoreModeType::NoStorage, "NoStorage"},
                              {StoreModeType::Store, "Store"}})

struct SimConfig
{
    static constexpr std::string_view KEY = "simconfig";
    static constexpr std::string_view SUB_DIR = "simconfig";

    std::optional<uint64_t> AddressOffset;
    std::optional<bool> CheckTLM2Protocol;
    std::optional<bool> DatabaseRecording;
    std::optional<bool> Debug;
    std::optional<bool> EnableWindowing;
    std::optional<bool> PowerAnalysis;
    std::optional<std::string> SimulationName;
    std::optional<bool> SimulationProgressBar;
    std::optional<StoreModeType> StoreMode;
    std::optional<bool> ThermalSimulation;
    std::optional<bool> UseMalloc;
    std::optional<unsigned int> WindowSize;
};

NLOHMANN_JSONIFY_ALL_THINGS(SimConfig,
                            AddressOffset,
                            CheckTLM2Protocol,
                            DatabaseRecording,
                            Debug,
                            EnableWindowing,
                            PowerAnalysis,
                            SimulationName,
                            SimulationProgressBar,
                            StoreMode,
                            ThermalSimulation,
                            UseMalloc,
                            WindowSize)

} // namespace DRAMSys::Config

#endif // DRAMSYSCONFIGURATION_SIMCONFIG_H
