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
 *    Thomas Zimmermann
 */

#ifndef CHECKERGDDR5X_H
#define CHECKERGDDR5X_H

#include "DRAMSys/controller/checker/CheckerIF.h"
#include "DRAMSys/configuration/memspec/MemSpecGDDR5X.h"

#include <array>
#include <queue>
#include <vector>

namespace DRAMSys
{

class CheckerGDDR5X final : public CheckerIF
{
public:
    explicit CheckerGDDR5X(const MemSpecGDDR5X& memSpec);
    [[nodiscard]] sc_core::sc_time timeToSatisfyConstraints(Command command, const tlm::tlm_generic_payload& payload, int controllerID) const override;
    void insert(Command command, const tlm::tlm_generic_payload& payload, int controllerID) override;

private:
    const MemSpecGDDR5X& memSpec;

    sc_core::sc_time tBURST;
    sc_core::sc_time tRDSRE;
    sc_core::sc_time tWRSRE;
    sc_core::sc_time tRDWR_R;
    sc_core::sc_time tWRRD_S;
    sc_core::sc_time tWRRD_L;
    sc_core::sc_time tWRRD_R;
    sc_core::sc_time tWRPRE;

    template<typename T>
    using CommandArray = std::array<T, Command::END_ENUM>;
    template<typename T>
    using BankVector = ControllerVector<Bank, T>;
    template<typename T>
    using BankGroupVector = ControllerVector<BankGroup, T>;
    template<typename T>
    using RankVector = ControllerVector<Rank, T>;

    CommandArray<BankVector<sc_core::sc_time>> lastScheduledByCommandAndBank;
    CommandArray<BankGroupVector<sc_core::sc_time>> lastScheduledByCommandAndBankGroup;
    CommandArray<RankVector<sc_core::sc_time>> lastScheduledByCommandAndRank;
    CommandArray<sc_core::sc_time> lastScheduledByCommand;

    std::vector<sc_core::sc_time> lastCommandOnBus;

    // 4 and 32 activate window
    RankVector<std::queue<sc_core::sc_time>> last4ActivatesOnRank;
    RankVector<std::queue<sc_core::sc_time>> last32ActivatesOnRank;

    ControllerVector<Rank, unsigned> bankwiseRefreshCounter;
};

} // namespace DRAMSys

#endif // CHECKERGDDR5X_H
