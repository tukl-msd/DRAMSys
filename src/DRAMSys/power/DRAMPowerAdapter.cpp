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
#include <type_traits>

namespace DRAMSys
{

DRAMPowerAdapter::DRAMPowerAdapter(const sc_core::sc_module_name& name,
                                   DRAMPowerVariant DRAMPower,
                                   const SimConfig& simConfig,
                                   const MemSpec& memSpec,
                                   TlmRecorder* tlmRecorder) :
    sc_module(name),
    tCK(memSpec.tCK),
    groupsPerRank(memSpec.groupsPerRank),
    banksPerGroup(memSpec.banksPerGroup),
    tlmRecorder(tlmRecorder),
    powerWindowSize(memSpec.tCK * simConfig.windowSize),
    DRAMPower(std::move(DRAMPower))
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
    double coreEnergy = 0;
    double interfaceEnergy = 0;
    double energy = 0;
    double time = 0;
    std::visit(
        [&coreEnergy, &interfaceEnergy, &energy, &time](auto& var)
        {
            coreEnergy = var.calcCoreEnergy(var.getLastCommandTime()).total();
            interfaceEnergy = var.calcInterfaceEnergy(var.getLastCommandTime()).total();
            energy = coreEnergy + interfaceEnergy;
            time = var.getLastCommandTime();
        },
        DRAMPower);
    time *= tCK.to_seconds();

    // Print the final total energy and the average power for
    // the simulation:
    std::cout << name() << std::string("  Total Energy:   ") << std::defaultfloat
              << std::setprecision(FLOATPRECISION) << energy << std::string(" J") << "\n";

    std::cout << name() << std::string("  Core Energy:   ") << std::defaultfloat
              << std::setprecision(FLOATPRECISION) << coreEnergy << std::string(" J") << "\n";

    std::cout << name() << std::string("  Interface Energy:   ") << std::defaultfloat
              << std::setprecision(FLOATPRECISION) << interfaceEnergy << std::string(" J") << "\n";

    std::cout << name() << std::string("  Average Power:  ") << std::defaultfloat
              << std::setprecision(FLOATPRECISION) << energy / time << std::string(" W") << "\n";

    if (tlmRecorder != nullptr)
    {
        tlmRecorder->recordPower(sc_core::sc_time_stamp().to_seconds(), energy / time);
    }
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
