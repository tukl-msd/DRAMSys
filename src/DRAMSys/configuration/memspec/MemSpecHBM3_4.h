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
 *    Martinius
 */

#ifndef MEMSPECHBM3_4_H
#define MEMSPECHBM3_4_H

#include "DRAMSys/configuration/memspec/MemSpec.h"

#include <DRAMUtils/memspec/standards/MemSpecHBM3_4.h>

#include <string>
#include <systemc>

namespace DRAMSys
{

class MemSpecHBM3_4 final : public MemSpec
{
public:
    explicit MemSpecHBM3_4(const DRAMUtils::MemSpec::MemSpecHBM3& memSpec);
    explicit MemSpecHBM3_4(const DRAMUtils::MemSpec::MemSpecHBM4& memSpec);

    const std::string memoryType;
    const uint64_t banksPerStack;

    // Memspec Variables:
    const sc_core::sc_time tDQSCK;
    const sc_core::sc_time tRC;
    const sc_core::sc_time tRAS;
    const sc_core::sc_time tRCDRD;
    const sc_core::sc_time tRCDWR;
    const sc_core::sc_time tRRDL;
    const sc_core::sc_time tRRDS;
    const sc_core::sc_time tFAW;
    const sc_core::sc_time tRTP;
    const sc_core::sc_time tRP;
    const sc_core::sc_time tRL;
    const sc_core::sc_time tWL;
    const sc_core::sc_time tPL;
    const sc_core::sc_time tWR;
    const sc_core::sc_time tCCDL;
    const sc_core::sc_time tCCDS;
    const sc_core::sc_time tCCDR;
    const sc_core::sc_time tWTRL;
    const sc_core::sc_time tWTRS;
    const sc_core::sc_time tRTW;
    const sc_core::sc_time tXP;
    const sc_core::sc_time tCKE;
    const sc_core::sc_time tPD;
    const sc_core::sc_time tCKESR;
    const sc_core::sc_time tXS;
    const sc_core::sc_time tRFC;
    const sc_core::sc_time tRFCPB;
    const sc_core::sc_time tRREFD;
    const sc_core::sc_time tREFI;
    const sc_core::sc_time tREFIPB;
    const sc_core::sc_time tPPD;

    void print() const override;

    [[nodiscard]] sc_core::sc_time getRefreshIntervalAB() const override;
    [[nodiscard]] sc_core::sc_time getRefreshIntervalPB() const override;

    [[nodiscard]] bool hasRasAndCasBus() const override;
    [[nodiscard]] bool pseudoChannelMode() const override;

    [[nodiscard]] sc_core::sc_time
    getExecutionTime(Command command, const tlm::tlm_generic_payload& payload) const override;
    [[nodiscard]] TimeInterval
    getIntervalOnDataStrobe(Command command,
                            const tlm::tlm_generic_payload& payload) const override;

private:
    template <typename MemSpecType>
    MemSpecHBM3_4(const MemSpecType& memSpec, std::string_view memoryType);
};

} // namespace DRAMSys

#endif // MEMSPECHBM3_4_H
