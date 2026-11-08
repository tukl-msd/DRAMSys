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

#ifndef MEMSPECLPDDR5_H
#define MEMSPECLPDDR5_H

#include "DRAMSys/configuration/memspec/MemSpec.h"

#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>

#include <systemc>

namespace DRAMSys
{

class MemSpecLPDDR5 final : public MemSpec
{
public:
    enum class BankArchitecture
    {
        BG,
        B16,
        B8
    };

    explicit MemSpecLPDDR5(const DRAMUtils::MemSpec::MemSpecLPDDR5& memSpec);

    const BankArchitecture bankArchitecture;
    const uint64_t wckToCkRatio;
    const unsigned per2BankOffset;

    // Memspec Variables:
    const sc_core::sc_time tREFI;
    const sc_core::sc_time tREFIpb;
    const sc_core::sc_time tRFCab;
    const sc_core::sc_time tRFCpb;
    const sc_core::sc_time tRAS;
    const sc_core::sc_time tRPab;
    const sc_core::sc_time tRPpb;
    const sc_core::sc_time tRCpb;
    const sc_core::sc_time tRCab;
    const sc_core::sc_time tPPD;
    const sc_core::sc_time tRCD;
    const sc_core::sc_time tFAW;
    const sc_core::sc_time tRRD;
    const sc_core::sc_time tRL;
    const sc_core::sc_time tRBTP;
    const sc_core::sc_time tWL;
    const sc_core::sc_time tWR;
    const sc_core::sc_time tRTRS;
    const sc_core::sc_time tBURST16;
    const sc_core::sc_time tBURSTMAX16;
    const sc_core::sc_time tCCD_L16;
    const sc_core::sc_time tCCD_S16;
    const sc_core::sc_time tBURST32;
    const sc_core::sc_time tBURSTMAX32;
    const sc_core::sc_time tCCD_L32;
    const sc_core::sc_time tCCD_S32;
    const sc_core::sc_time tWTR_L;
    const sc_core::sc_time tWTR_S;
    const sc_core::sc_time tRTW_L16;
    const sc_core::sc_time tRTW_S16;
    const sc_core::sc_time tRTW_L32;
    const sc_core::sc_time tRTW_S32;
    const sc_core::sc_time tWCK2DQO;
    const sc_core::sc_time tpbR2act;
    const sc_core::sc_time tpbR2pbR;
    const sc_core::sc_time tAAD;

    void print() const override;

    [[nodiscard]] sc_core::sc_time getRefreshIntervalAB() const override;
    [[nodiscard]] sc_core::sc_time getRefreshIntervalPB() const override;
    [[nodiscard]] sc_core::sc_time getRefreshIntervalP2B() const override;

    [[nodiscard]] unsigned getPer2BankOffset() const override;

    [[nodiscard]] sc_core::sc_time
    getExecutionTime(Command command, const tlm::tlm_generic_payload& payload) const override;
    [[nodiscard]] TimeInterval
    getIntervalOnDataStrobe(Command command,
                            const tlm::tlm_generic_payload& payload) const override;

private:
    [[nodiscard]] sc_core::sc_time getBurstDuration(const tlm::tlm_generic_payload& payload) const;
};

} // namespace DRAMSys

#endif // MEMSPECLPDDR5_H
