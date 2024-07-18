/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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
 *    Lukas Steiner
 */

#include "DramRecordable.h"

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/common/utils.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

DramRecordable::DramRecordable(const sc_module_name& name,
                               const SimConfig& simConfig,
                               const MemSpec& memSpec,
                               TlmRecorder& tlmRecorder) :
    Dram(name, simConfig, memSpec),
    tlmRecorder(tlmRecorder),
    powerWindowSize(memSpec.tCK * simConfig.windowSize)
{
#ifdef DRAMPOWER
    // Create a thread that is triggered every $powerWindowSize
    // to generate a Power over Time plot in the Trace analyzer:
    if (simConfig.powerAnalysis && simConfig.enableWindowing)
        SC_THREAD(powerWindow);
#endif
}

void DramRecordable::reportPower()
{
    Dram::reportPower();
#ifdef DRAMPOWER
    tlmRecorder.recordPower(sc_time_stamp().to_seconds(),
                            this->DRAMPower->getPower().window_average_power *
                                this->memSpec.devicesPerRank);
#endif
}

tlm_sync_enum
DramRecordable::nb_transport_fw(tlm_generic_payload& trans, tlm_phase& phase, sc_time& delay)
{
    tlmRecorder.recordPhase(trans, phase, delay);
    return Dram::nb_transport_fw(trans, phase, delay);
}

#ifdef DRAMPOWER
// This Thread is only triggered when Power Simulation is enabled.
// It estimates the current average power which will be stored in the trace database for
// visualization purposes.
void DramRecordable::powerWindow()
{
    int64_t clkCycles = 0;

    while (true)
    {
        // At the very beginning (zero clock cycles) the energy is 0, so we wait first
        sc_module::wait(powerWindowSize);

        clkCycles = std::lround(sc_time_stamp() / this->memSpec.tCK);

        this->DRAMPower->calcWindowEnergy(clkCycles);

        // During operation the energy should never be zero since the device is always consuming
        assert(!isEqual(this->DRAMPower->getEnergy().window_energy, 0.0));

        // Store the time (in seconds) and the current average power (in mW) into the database
        tlmRecorder.recordPower(sc_time_stamp().to_seconds(),
                                this->DRAMPower->getPower().window_average_power *
                                    this->memSpec.devicesPerRank);

        // Here considering that DRAMPower provides the energy in pJ and the power in mW
        PRINTDEBUGMESSAGE(this->name(),
                          std::string("\tWindow Energy: \t") +
                              std::to_string(this->DRAMPower->getEnergy().window_energy *
                                             this->memSpec.devicesPerRank) +
                              std::string("\t[pJ]"));
        PRINTDEBUGMESSAGE(this->name(),
                          std::string("\tWindow Average Power: \t") +
                              std::to_string(this->DRAMPower->getPower().window_average_power *
                                             this->memSpec.devicesPerRank) +
                              std::string("\t[mW]"));
    }
}
#endif

} // namespace DRAMSys
