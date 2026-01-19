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
 * Author: Lukas Steiner
 */

#ifndef CHECKERWIDEIO2_H
#define CHECKERWIDEIO2_H

#include "DRAMSys/configuration/memspec/MemSpecWideIO2.h"
#include "DRAMSys/controller/checker/CheckerIF.h"

#include <queue>
#include <vector>

namespace DRAMSys
{

class CheckerWideIO2 final : public CheckerIF
{
public:
    explicit CheckerWideIO2(const MemSpecWideIO2& memSpec);
    [[nodiscard]] sc_core::sc_time
    timeToSatisfyConstraints(Command command,
                             const tlm::tlm_generic_payload& payload) const override;
    void insert(Command command, const tlm::tlm_generic_payload& payload) override;

private:
    const MemSpecWideIO2& memSpec;

    std::vector<ControllerVector<Bank, sc_core::sc_time>> lastScheduledByCommandAndBank;
    std::vector<ControllerVector<Rank, sc_core::sc_time>> lastScheduledByCommandAndRank;
    std::vector<sc_core::sc_time> lastScheduledByCommand;
    sc_core::sc_time lastCommandOnBus;

    // Four activate window
    ControllerVector<Rank, std::queue<sc_core::sc_time>> last4Activates;

    const sc_core::sc_time scMaxTime = sc_core::sc_max_time();
    sc_core::sc_time tBURST;
    sc_core::sc_time tRDPRE;
    sc_core::sc_time tRDPDEN;
    sc_core::sc_time tRDWR;
    sc_core::sc_time tRDWR_R;
    sc_core::sc_time tWRPRE;
    sc_core::sc_time tWRPDEN;
    sc_core::sc_time tWRAPDEN;
    sc_core::sc_time tWRRD;
    sc_core::sc_time tWRRD_R;
};

} // namespace DRAMSys

#endif // CHECKERWIDEIO2_H
