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
 *    Derek Christ
 *    Marco Mörz
 */

#ifndef MEMSPECDDR4_H
#define MEMSPECDDR4_H

#include "DRAMSys/configuration/memspec/MemSpec.h"

#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>

#include <systemc>

namespace DRAMSys
{

class MemSpecDDR4 final : public MemSpec
{
public:
    explicit MemSpecDDR4(const DRAMUtils::MemSpec::MemSpecDDR4& memSpec);

    // Memspec Variables:
    const DRAMUtils::MemSpec::MemSpecDDR4& memSpec;
    const sc_core::sc_time tCKE;
    const sc_core::sc_time tPD;
    const sc_core::sc_time tCKESR;
    const sc_core::sc_time tRAS;
    const sc_core::sc_time tRC;
    const sc_core::sc_time tRCD;
    const sc_core::sc_time tRL;
    const sc_core::sc_time tRPRE;
    const sc_core::sc_time tRTP;
    const sc_core::sc_time tWL;
    const sc_core::sc_time tWPRE;
    const sc_core::sc_time tWR;
    const sc_core::sc_time tXP;
    const sc_core::sc_time tXS;
    const sc_core::sc_time tREFI;
    const sc_core::sc_time tRFC;
    const sc_core::sc_time tRP;
    const sc_core::sc_time tDQSCK;
    const sc_core::sc_time tCCD_S;
    const sc_core::sc_time tCCD_L;
    const sc_core::sc_time tFAW;
    const sc_core::sc_time tRRD_S;
    const sc_core::sc_time tRRD_L;
    const sc_core::sc_time tWTR_S;
    const sc_core::sc_time tWTR_L;
    const sc_core::sc_time tAL;
    const sc_core::sc_time tXPDLL;
    const sc_core::sc_time tXSDLL;
    const sc_core::sc_time tACTPDEN;
    const sc_core::sc_time tPRPDEN;
    const sc_core::sc_time tREFPDEN;
    const sc_core::sc_time tRTRS;

    [[nodiscard]] sc_core::sc_time getRefreshIntervalAB() const override;

    [[nodiscard]] sc_core::sc_time
    getExecutionTime(Command command, const tlm::tlm_generic_payload& payload) const override;
    [[nodiscard]] TimeInterval
    getIntervalOnDataStrobe(Command command,
                            const tlm::tlm_generic_payload& payload) const override;

    [[nodiscard]] bool requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const override;

    [[nodiscard]] std::unique_ptr<DRAMPower::dram_base<DRAMPower::CmdType>> toDramPowerObject() const override;
};

} // namespace DRAMSys

#endif // MEMSPECDDR4_H
