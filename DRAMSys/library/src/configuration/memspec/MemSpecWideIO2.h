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

#ifndef MEMSPECWIDEIO2_H
#define MEMSPECWIDEIO2_H

#include "MemSpec.h"
#include "../../common/third_party/nlohmann/single_include/nlohmann/json.hpp"

class MemSpecWideIO2 final : public MemSpec
{
public:
    MemSpecWideIO2(nlohmann::json &memspec);

    // Memspec Variables:
    const sc_time tDQSCK;
    const sc_time tDQSS;
    const sc_time tCKE;
    const sc_time tRL;
    const sc_time tWL;
    const sc_time tRCpb;
    const sc_time tRCab;
    const sc_time tCKESR;
    const sc_time tXSR;
    const sc_time tXP;
    const sc_time tCCD;
    const sc_time tRTP;
    const sc_time tRCD;
    const sc_time tRPpb;
    const sc_time tRPab;
    const sc_time tRAS;
    const sc_time tWR;
    const sc_time tWTR;
    const sc_time tRRD;
    const sc_time tFAW;
    const sc_time tREFI;
    const sc_time tREFIpb;
    const sc_time tRFCab;
    const sc_time tRFCpb;
    const sc_time tRTRS;

    // Currents and Voltages:
    // TODO: to be completed

    virtual sc_time getRefreshIntervalPB() const override;
    virtual sc_time getRefreshIntervalAB() const override;

    virtual sc_time getExecutionTime(Command, const tlm::tlm_generic_payload &) const override;
    virtual TimeInterval getIntervalOnDataStrobe(Command) const override;
};

#endif // MEMSPECWIDEIO2_H
