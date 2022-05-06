/*
 * Copyright (c) 2019, Technische Universität Kaiserslautern
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

#include <cmath>
#include "../../common/TlmRecorder.h"
#include "../../common/utils.h"
#include "DramDDR3.h"
#include "DramDDR4.h"
#include "DramDDR5.h"
#include "DramWideIO.h"
#include "DramLPDDR4.h"
#include "DramLPDDR5.h"
#include "DramWideIO2.h"
#include "DramHBM2.h"
#include "DramGDDR5.h"
#include "DramGDDR5X.h"
#include "DramGDDR6.h"
#include "DramSTTMRAM.h"

using namespace sc_core;
using namespace tlm;

template<class BaseDram>
DramRecordable<BaseDram>::DramRecordable(const sc_module_name& name, const Configuration& config,
                                         TemperatureController& temperatureController, TlmRecorder& tlmRecorder)
    : BaseDram(name, config, temperatureController), tlmRecorder(tlmRecorder),
    powerWindowSize(config.memSpec->tCK * config.windowSize)
{
    // Create a thread that is triggered every $powerWindowSize
    // to generate a Power over Time plot in the Trace analyzer:
    if (config.powerAnalysis && config.enableWindowing)
        SC_THREAD(powerWindow);
}

template<class BaseDram>
void DramRecordable<BaseDram>::reportPower()
{
    BaseDram::reportPower();
    tlmRecorder.recordPower(sc_time_stamp().to_seconds(),
                             this->DRAMPower->getPower().window_average_power
                             * this->memSpec.devicesPerRank);
}

template<class BaseDram>
tlm_sync_enum DramRecordable<BaseDram>::nb_transport_fw(tlm_generic_payload &payload,
                                          tlm_phase &phase, sc_time &delay)
{
    recordPhase(payload, phase, delay);
    return BaseDram::nb_transport_fw(payload, phase, delay);
}

template<class BaseDram>
void DramRecordable<BaseDram>::recordPhase(tlm_generic_payload &trans, const tlm_phase &phase, const sc_time &delay)
{
    sc_time recTime = sc_time_stamp() + delay;

    // These are terminating phases recorded by the DRAM. The execution
    // time of the related command must be taken into consideration.
    if (phase == END_PDNA || phase == END_PDNP || phase == END_SREF)
        recTime += this->memSpec.getCommandLength(Command(phase));

    NDEBUG_UNUSED(unsigned thr) = DramExtension::getExtension(trans).getThread().ID();
    NDEBUG_UNUSED(unsigned ch) = DramExtension::getExtension(trans).getChannel().ID();
    NDEBUG_UNUSED(unsigned bg) = DramExtension::getExtension(trans).getBankGroup().ID();
    NDEBUG_UNUSED(unsigned bank) = DramExtension::getExtension(trans).getBank().ID();
    NDEBUG_UNUSED(unsigned row) = DramExtension::getExtension(trans).getRow().ID();
    NDEBUG_UNUSED(unsigned col) = DramExtension::getExtension(trans).getColumn().ID();

    PRINTDEBUGMESSAGE(this->name(), "Recording " + getPhaseName(phase) +  " thread " +
                      std::to_string(thr) + " channel " + std::to_string(ch) + " bank group " + std::to_string(
                          bg) + " bank " + std::to_string(bank) + " row " + std::to_string(row) + " column " +
                      std::to_string(col) + " at " + recTime.to_string());

    tlmRecorder.recordPhase(trans, phase, recTime);

    if (phaseNeedsEnd(phase))
    {
        recTime += this->memSpec.getExecutionTime(Command(phase), trans);
        tlmRecorder.recordPhase(trans, getEndPhase(phase), recTime);
    }

}


// This Thread is only triggered when Power Simulation is enabled.
// It estimates the current average power which will be stored in the trace database for visualization purposes.
template<class BaseDram>
void DramRecordable<BaseDram>::powerWindow()
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
                                 this->DRAMPower->getPower().window_average_power
                                 * this->memSpec.devicesPerRank);

        // Here considering that DRAMPower provides the energy in pJ and the power in mW
        PRINTDEBUGMESSAGE(this->name(), std::string("\tWindow Energy: \t") + std::to_string(
                              this->DRAMPower->getEnergy().window_energy *
                              this->memSpec.devicesPerRank) + std::string("\t[pJ]"));
        PRINTDEBUGMESSAGE(this->name(), std::string("\tWindow Average Power: \t") + std::to_string(
                              this->DRAMPower->getPower().window_average_power *
                              this->memSpec.devicesPerRank) + std::string("\t[mW]"));

    }
}

template class DramRecordable<DramDDR3>;
template class DramRecordable<DramDDR4>;
template class DramRecordable<DramDDR5>;
template class DramRecordable<DramLPDDR4>;
template class DramRecordable<DramLPDDR5>;
template class DramRecordable<DramWideIO>;
template class DramRecordable<DramWideIO2>;
template class DramRecordable<DramGDDR5>;
template class DramRecordable<DramGDDR5X>;
template class DramRecordable<DramGDDR6>;
template class DramRecordable<DramHBM2>;
template class DramRecordable<DramSTTMRAM>;
