/*
 * Copyright (c) 2024, RPTU Kaiserslautern-Landau
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
 *    Marco Mörz
 */

#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <DRAMSys/config/SimConfig.h>
#include <DRAMUtils/config/toggling_rate.h>

#include <string>
#include <optional>

namespace DRAMSys
{

struct SimConfig
{
    SimConfig(const Config::SimConfig& simConfig);

    std::string simulationName;
    bool databaseRecording;
    bool powerAnalysis;
    bool enableWindowing;
    unsigned int windowSize;
    bool debug;
    bool simulationProgressBar;
    bool checkTLM2Protocol;
    bool useMalloc;
    unsigned long long int addressOffset;
    Config::StoreModeType storeMode;
    std::optional<DRAMUtils::Config::ToggleRateDefinition> togglingRate;

    static constexpr std::string_view DEFAULT_SIMULATION_NAME = "default";
    static constexpr bool DEFAULT_DATABASE_RECORDING = false;
    static constexpr bool DEFAULT_POWER_ANALYSIS = false;
    static constexpr bool DEFAULT_ENABLE_WINDOWING = false;
    static constexpr unsigned int DEFAULT_WINDOW_SIZE = 1000;
    static constexpr bool DEFAULT_DEBUG = false;
    static constexpr bool DEFAULT_SIMULATION_PROGRESS_BAR = false;
    static constexpr bool DEFAULT_CHECK_TLM2_PROTOCOL = false;
    static constexpr bool DEFAULT_USE_MALLOC = false;
    static constexpr unsigned long long int DEFAULT_ADDRESS_OFFSET = 0;
    static constexpr Config::StoreModeType DEFAULT_STORE_MODE = Config::StoreModeType::NoStorage;
};

} // namespace DRAMSys

#endif
