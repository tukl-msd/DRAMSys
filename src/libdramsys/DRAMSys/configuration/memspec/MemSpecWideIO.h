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

#ifndef MEMSPECWIDEIO_H
#define MEMSPECWIDEIO_H

#include "DRAMSys/configuration/memspec/MemSpec.h"

#include <DRAMUtils/memspec/standards/MemSpecWideIO.h>

#include <systemc>

namespace DRAMSys
{

class MemSpecWideIO final : public MemSpec
{
public:
    explicit MemSpecWideIO(const DRAMUtils::MemSpec::MemSpecWideIO& memSpec);

    // Memspec Variables:
    const sc_core::sc_time tCKE;
    const sc_core::sc_time tCKESR;
    const sc_core::sc_time tRAS;
    const sc_core::sc_time tRC;
    const sc_core::sc_time tRCD;
    const sc_core::sc_time tRL;
    const sc_core::sc_time tWL;
    const sc_core::sc_time tWR;
    const sc_core::sc_time tXP;
    const sc_core::sc_time tXSR;
    const sc_core::sc_time tREFI;
    const sc_core::sc_time tRFC;
    const sc_core::sc_time tRP;
    const sc_core::sc_time tDQSCK;
    const sc_core::sc_time tAC;
    const sc_core::sc_time tCCD_R;
    const sc_core::sc_time tCCD_W;
    const sc_core::sc_time tRRD;
    const sc_core::sc_time tTAW;
    const sc_core::sc_time tWTR;
    const sc_core::sc_time tRTRS;

    [[nodiscard]] sc_core::sc_time getRefreshIntervalAB() const override;

    [[nodiscard]] sc_core::sc_time
    getExecutionTime(Command command, const tlm::tlm_generic_payload& payload) const override;
    [[nodiscard]] TimeInterval
    getIntervalOnDataStrobe(Command command,
                            const tlm::tlm_generic_payload& payload) const override;

    [[nodiscard]] bool requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const override;
};

} // namespace DRAMSys

#endif // MEMSPECWIDEIO_H
