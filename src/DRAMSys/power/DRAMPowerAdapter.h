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

#ifndef DRAMPOWERADAPTER_H
#define DRAMPOWERADAPTER_H

#include "DRAMSys/common/Deserialize.h"
#include "DRAMSys/common/Serialize.h"
#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/power/DRAMPowerVariant.h"
#include "DRAMSys/simulation/SimConfig.h"

#include <DRAMPower/command/CmdType.h>
#include <DRAMPower/dram/dram_base.h>

#include <systemc>
#include <tlm>

namespace DRAMSys
{

class DRAMPowerAdapter : public sc_core::sc_module, public Serialize, public Deserialize
{
private:
    static constexpr double MINENERGYPERWINDOW = 1e-15;
    static constexpr unsigned char BITSPERBYTE = 8;
    static constexpr int FLOATPRECISION = 6;

    sc_core::sc_time tCK;
    uint64_t groupsPerRank;
    uint64_t banksPerGroup;

    // Data Storage:
    TlmRecorder* const tlmRecorder;
    sc_core::sc_time powerWindowSize;

    DRAMPowerVariant DRAMPower;

    // This Thread is only triggered when Power Simulation is enabled.
    // It estimates the current average power which will be stored in the trace database for
    // visualization purposes.
    void powerWindow();

public:
    void handleTransaction(std::size_t channel,
                           const tlm::tlm_generic_payload& trans,
                           const tlm::tlm_phase& phase,
                           const sc_core::sc_time& delay);
    SC_HAS_PROCESS(DRAMPowerAdapter);
    DRAMPowerAdapter(const sc_core::sc_module_name& name,
                     DRAMPowerVariant DRAMPower,
                     const SimConfig& simConfig,
                     const MemSpec& memSpec,
                     TlmRecorder* tlmRecorder);

    DRAMPowerAdapter(const DRAMPowerAdapter&) = delete;
    DRAMPowerAdapter(DRAMPowerAdapter&&) = delete;
    DRAMPowerAdapter& operator=(const DRAMPowerAdapter&) = delete;
    DRAMPowerAdapter& operator=(DRAMPowerAdapter&&) = delete;
    ~DRAMPowerAdapter() override = default;

    void reportPower();
    const DRAMPowerVariant& getDRAMPowerVariant() const;

    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;
};

} // namespace DRAMSys

#endif /* DRAMPOWERADAPTER_H */
