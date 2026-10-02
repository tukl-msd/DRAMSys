/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 *    Marco Mörz
 */

#ifdef USE_DRAMPOWER

#include "DRAMSys/power/DRAMPowerAdapter.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/power/DRAMPowerVariant.h"

#include <DRAMPower/command/Command.h>
#include <DRAMPower/simconfig/simconfig.h>
#include <DRAMUtils/util/types.h>

#include <cassert>
#include <cstdlib>

namespace DRAMSys
{

DRAMPowerAdapter::DRAMPowerAdapter(const sc_core::sc_module_name& name,
                                   DRAMPowerVariant DRAMPower,
                                   const SimConfig& simConfig,
                                   const MemSpec& memSpec,
                                   TlmRecorder* tlmRecorder) :
    sc_module(name),
    tCK(memSpec.tCK),
    tlmRecorder(tlmRecorder),
    powerWindowSize(memSpec.tCK * simConfig.windowSize),
    DRAMPower(std::move(DRAMPower)),
    stats(*this)
{
    assert(simConfig.powerAnalysis && "DRAMPowerObject created for simConfig.powerAnalysis=false");

    if (simConfig.powerAnalysis && simConfig.enableWindowing)
        SC_THREAD(powerWindow);
}

const DRAMPowerVariant& DRAMPowerAdapter::getDRAMPowerVariant() const
{
    return DRAMPower;
}

void DRAMPowerAdapter::reportPower()
{
    // Record the final average power of the whole simulation into the trace database:
    if (tlmRecorder != nullptr)
    {
        tlmRecorder->recordPower(sc_core::sc_time_stamp().to_seconds(),
                                 computePowerData().averagePower);
    }
}

DRAMPowerAdapter::PowerData DRAMPowerAdapter::computePowerData()
{
    PowerData data;
    std::visit(
        [this, &data](auto& var)
        {
            data.coreEnergy = var.calcCoreEnergy(var.getLastCommandTime()).total();
            data.interfaceEnergy = var.calcInterfaceEnergy(var.getLastCommandTime()).total();
            data.totalEnergy = data.coreEnergy + data.interfaceEnergy;
            double time = var.getLastCommandTime();
            time *= tCK.to_seconds();
            data.averagePower = data.totalEnergy / time;
        },
        DRAMPower);
    return data;
}

DRAMPowerAdapter::PowerStats::PowerStats(DRAMPowerAdapter const& adapter) :
    Group(adapter.basename()),
    totalEnergy(addStat<Stats::ScalarStat>(
        "TotalEnergy", "Total energy consumed by the memory", Stats::Quantity::Energy)),
    coreEnergy(addStat<Stats::ScalarStat>(
        "CoreEnergy", "Energy consumed by the memory core", Stats::Quantity::Energy)),
    interfaceEnergy(addStat<Stats::ScalarStat>(
        "InterfaceEnergy", "Energy consumed by the memory interface", Stats::Quantity::Energy)),
    averagePower(addStat<Stats::ScalarStat>(
        "AveragePower", "Average power over the simulation duration", Stats::Quantity::Power))
{
}

void DRAMPowerAdapter::updateStats()
{
    PowerData data = computePowerData();
    stats.totalEnergy = data.totalEnergy;
    stats.coreEnergy = data.coreEnergy;
    stats.interfaceEnergy = data.interfaceEnergy;
    stats.averagePower = data.averagePower;
}

void DRAMPowerAdapter::handleTransaction(std::size_t channel,
                                         const tlm::tlm_generic_payload& trans,
                                         const tlm::tlm_phase& phase,
                                         const sc_core::sc_time& delay)
{
    assert(phase >= BEGIN_RD && phase <= END_SREF);
    std::visit([channel, &trans, &phase, &delay](auto& var) {
        return var.doCommand(channel, trans, phase, delay);
    }, DRAMPower);
}

void DRAMPowerAdapter::serialize(std::ostream& stream) const
{
    std::visit([&stream](auto& var) { var.serialize(stream); }, DRAMPower);
}

void DRAMPowerAdapter::deserialize(std::istream& stream)
{
    std::visit([&stream](auto& var) { var.deserialize(stream); }, DRAMPower);
}

void DRAMPowerAdapter::powerWindow()
{
    int64_t clkCycles = 0;
    double previousEnergy = 0;
    double currentEnergy = 0;
    double windowEnergy = 0;
    double powerWindowSizeSeconds = powerWindowSize.to_seconds();

    while (true)
    {
        // At the very beginning (zero clock cycles) the energy is 0, so we wait first
        sc_module::wait(powerWindowSize);

        clkCycles = std::lround(sc_core::sc_time_stamp() / tCK);

        std::visit([&currentEnergy, &clkCycles](auto& var)
                   { currentEnergy = var.getTotalEnergy(clkCycles); },
                   DRAMPower);
        windowEnergy = currentEnergy - previousEnergy;
        previousEnergy = currentEnergy;

        // During operation the energy should never be zero since the device is always consuming
        assert(!(windowEnergy < MINENERGYPERWINDOW));

        if (nullptr != tlmRecorder)
        {
            // Store the time (in seconds) and the current average power (in mW) into the database
            tlmRecorder->recordPower(sc_core::sc_time_stamp().to_seconds(),
                                     windowEnergy / powerWindowSizeSeconds);
        }

        // Here considering that DRAMPower provides the energy in J and the power in W
        PRINTDEBUGMESSAGE(this->name(),
                          std::string("\tWindow Energy: \t") + std::to_string(windowEnergy) +
                              std::string("\t[J]"));
        PRINTDEBUGMESSAGE(this->name(),
                          std::string("\tWindow Average Power: \t") +
                              std::to_string(windowEnergy / powerWindowSizeSeconds) +
                              std::string("\t[W]"));
    }
}

} // namespace DRAMSys

#endif // USE_DRAMPOWER
