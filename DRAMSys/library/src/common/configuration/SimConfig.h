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

#ifndef DRAMSYSCONFIGURATION_SIMCONFIG_H
#define DRAMSYSCONFIGURATION_SIMCONFIG_H

#include "util.h"

#include <nlohmann/json.hpp>
#include <optional>

namespace DRAMSysConfiguration
{
using json = nlohmann::json;

const std::string simConfigPath = "configs/simulator";

enum class StoreMode
{
    NoStorage,
    Store,
    ErrorModel,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(StoreMode, {{StoreMode::Invalid, nullptr},
                                         {StoreMode::NoStorage, "NoStorage"},
                                         {StoreMode::Store, "Store"},
                                         {StoreMode::ErrorModel, "ErrorModel"}})

struct SimConfig
{
    std::optional<uint64_t> addressOffset;
    std::optional<bool> checkTLM2Protocol;
    std::optional<bool> databaseRecording;
    std::optional<bool> debug;
    std::optional<bool> enableWindowing;
    std::optional<std::string> errorCsvFile;
    std::optional<unsigned int> errorChipSeed;
    std::optional<bool> powerAnalysis;
    std::optional<std::string> simulationName;
    std::optional<bool> simulationProgressBar;
    std::optional<StoreMode> storeMode;
    std::optional<bool> thermalSimulation;
    std::optional<bool> useMalloc;
    std::optional<unsigned int> windowSize;
};

void to_json(json &j, const SimConfig &c);
void from_json(const json &j, SimConfig &c);

void from_dump(const std::string &dump, SimConfig &c);
std::string dump(const SimConfig &c, unsigned int indentation = -1);

} // namespace Configuration

#endif // DRAMSYSCONFIGURATION_SIMCONFIG_H
