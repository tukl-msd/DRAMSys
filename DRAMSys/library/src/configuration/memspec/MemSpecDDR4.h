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

#ifndef MEMSPECDDR4_H
#define MEMSPECDDR4_H

#include "MemSpec.h"
#include "../../common/third_party/nlohmann/single_include/nlohmann/json.hpp"

class MemSpecDDR4 final : public MemSpec
{
public:
    MemSpecDDR4(nlohmann::json &memspec);

    // Memspec Variables:
    const sc_time tCKE;
    const sc_time tPD;
    const sc_time tCKESR;
    const sc_time tRAS;
    const sc_time tRC;
    const sc_time tRCD;
    const sc_time tRL;
    const sc_time tRTP;
    const sc_time tWL;
    const sc_time tWR;
    const sc_time tXP;
    const sc_time tXS;
    const sc_time tREFI;
    const sc_time tRFC;
    const sc_time tRP;
    const sc_time tDQSCK;
    const sc_time tCCD_S;
    const sc_time tCCD_L;
    const sc_time tFAW;
    const sc_time tRRD_S;
    const sc_time tRRD_L;
    const sc_time tWTR_S;
    const sc_time tWTR_L;
    const sc_time tAL;
    const sc_time tXPDLL;
    const sc_time tXSDLL;
    const sc_time tACTPDEN;
    const sc_time tPRPDEN;
    const sc_time tREFPDEN;
    const sc_time tRTRS;

    // Currents and Voltages:
    const double iDD0;
    const double iDD2N;
    const double iDD3N;
    const double iDD4R;
    const double iDD4W;
    const double iDD5;
    const double iDD6;
    const double vDD;
    const double iDD02;
    const double iDD2P0;
    const double iDD2P1;
    const double iDD3P0;
    const double iDD3P1;
    const double iDD62;
    const double vDD2;

    virtual sc_time getRefreshIntervalPB() const override;
    virtual sc_time getRefreshIntervalAB() const override;

    virtual sc_time getExecutionTime(Command, const tlm::tlm_generic_payload &) const override;
    virtual TimeInterval getIntervalOnDataStrobe(Command) const override;
};

#endif // MEMSPECDDR4_H
