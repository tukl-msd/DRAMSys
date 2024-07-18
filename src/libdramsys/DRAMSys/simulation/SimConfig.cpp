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
 */

#include "SimConfig.h"
#include "DRAMSys/config/SimConfig.h"

#include <systemc>

namespace DRAMSys
{

SimConfig::SimConfig(const Config::SimConfig& simConfig) :
    simulationName(simConfig.SimulationName.value_or(DEFAULT_SIMULATION_NAME.data())),
    databaseRecording(simConfig.DatabaseRecording.value_or(DEFAULT_DATABASE_RECORDING)),
    powerAnalysis(simConfig.PowerAnalysis.value_or(DEFAULT_POWER_ANALYSIS)),
    enableWindowing(simConfig.EnableWindowing.value_or(DEFAULT_ENABLE_WINDOWING)),
    windowSize(simConfig.WindowSize.value_or(DEFAULT_WINDOW_SIZE)),
    debug(simConfig.Debug.value_or(DEFAULT_DEBUG)),
    simulationProgressBar(
        simConfig.SimulationProgressBar.value_or(DEFAULT_SIMULATION_PROGRESS_BAR)),
    checkTLM2Protocol(simConfig.CheckTLM2Protocol.value_or(DEFAULT_CHECK_TLM2_PROTOCOL)),
    useMalloc(simConfig.UseMalloc.value_or(DEFAULT_USE_MALLOC)),
    addressOffset(simConfig.AddressOffset.value_or(DEFAULT_ADDRESS_OFFSET)),
    storeMode(simConfig.StoreMode.value_or(DEFAULT_STORE_MODE))
{
    if (storeMode == Config::StoreModeType::Invalid)
        SC_REPORT_FATAL("SimConfig", "Invalid StoreMode");

    if (windowSize == 0)
        SC_REPORT_FATAL("SimConfig", "Minimum window size is 1");

#ifndef DRAMPOWER
    if (powerAnalysis)
        SC_REPORT_FATAL("SimConfig",
                        "Power analysis is only supported with included DRAMPower library!");
#endif
}

} // namespace DRAMSys
