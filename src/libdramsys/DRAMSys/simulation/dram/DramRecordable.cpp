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
#include "DRAMSys/simulation/dram/DramDDR3.h"
#include "DRAMSys/simulation/dram/DramDDR4.h"
#include "DRAMSys/simulation/dram/DramGDDR5.h"
#include "DRAMSys/simulation/dram/DramGDDR5X.h"
#include "DRAMSys/simulation/dram/DramGDDR6.h"
#include "DRAMSys/simulation/dram/DramHBM2.h"
#include "DRAMSys/simulation/dram/DramLPDDR4.h"
#include "DRAMSys/simulation/dram/DramSTTMRAM.h"
#include "DRAMSys/simulation/dram/DramWideIO.h"
#include "DRAMSys/simulation/dram/DramWideIO2.h"

#ifdef DDR5_SIM
#include "DRAMSys/simulation/dram/DramDDR5.h"
#endif
#ifdef LPDDR5_SIM
#include "DRAMSys/simulation/dram/DramLPDDR5.h"
#endif
#ifdef HBM3_SIM
#include "DRAMSys/simulation/dram/DramHBM3.h"
#endif

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

template <typename BaseDram>
DramRecordable<BaseDram>::DramRecordable(const sc_module_name& name,
                                         const Configuration& config,
                                         TlmRecorder& tlmRecorder) :
    BaseDram(name, config),
    tlmRecorder(tlmRecorder),
    powerWindowSize(config.memSpec->tCK * config.windowSize)
{
#ifdef DRAMPOWER
    // Create a thread that is triggered every $powerWindowSize
    // to generate a Power over Time plot in the Trace analyzer:
    if (config.powerAnalysis && config.enableWindowing)
        SC_THREAD(powerWindow);
#endif
}

template <typename BaseDram> void DramRecordable<BaseDram>::reportPower()
{
    BaseDram::reportPower();
#ifdef DRAMPOWER
    tlmRecorder.recordPower(sc_time_stamp().to_seconds(),
                            this->DRAMPower->getPower().window_average_power *
                                this->memSpec.devicesPerRank);
#endif
}

template <typename BaseDram>
tlm_sync_enum DramRecordable<BaseDram>::nb_transport_fw(tlm_generic_payload& trans,
                                                        tlm_phase& phase,
                                                        sc_time& delay)
{
    tlmRecorder.recordPhase(trans, phase, delay);
    return BaseDram::nb_transport_fw(trans, phase, delay);
}

#ifdef DRAMPOWER
// This Thread is only triggered when Power Simulation is enabled.
// It estimates the current average power which will be stored in the trace database for
// visualization purposes.
template <typename BaseDram> void DramRecordable<BaseDram>::powerWindow()
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

template class DramRecordable<DramDDR3>;
template class DramRecordable<DramDDR4>;
template class DramRecordable<DramLPDDR4>;
template class DramRecordable<DramWideIO>;
template class DramRecordable<DramWideIO2>;
template class DramRecordable<DramGDDR5>;
template class DramRecordable<DramGDDR5X>;
template class DramRecordable<DramGDDR6>;
template class DramRecordable<DramHBM2>;
template class DramRecordable<DramSTTMRAM>;
#ifdef DDR5_SIM
template class DramRecordable<DramDDR5>;
#endif
#ifdef LPDDR5_SIM
template class DramRecordable<DramLPDDR5>;
#endif
#ifdef HBM3_SIM
template class DramRecordable<DramHBM3>;
#endif

} // namespace DRAMSys
