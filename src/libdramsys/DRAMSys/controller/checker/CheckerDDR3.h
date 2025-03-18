/*
 * Copyright (c) 2024, RPTU Kaiserslautern-Landau
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
 */

#ifndef CHECKERDDR3_H
#define CHECKERDDR3_H

#include "DRAMSys/controller/checker/CheckerIF.h"
#include "DRAMSys/configuration/memspec/MemSpecDDR3.h"

#include <queue>
#include <vector>

namespace DRAMSys
{

class CheckerDDR3 final : public CheckerIF
{
public:
    explicit CheckerDDR3(const MemSpecDDR3& memSpec);
    [[nodiscard]] sc_core::sc_time timeToSatisfyConstraints(Command command, const tlm::tlm_generic_payload& payload) const override;
    void insert(Command command, const tlm::tlm_generic_payload& payload) override;

private:
    const MemSpecDDR3& memSpec;

    sc_core::sc_time tBURST;
    sc_core::sc_time tRDWR;
    sc_core::sc_time tRDWR_R;
    sc_core::sc_time tWRRD;
    sc_core::sc_time tWRRD_R;
    sc_core::sc_time tWRPRE;
    sc_core::sc_time tRDPDEN;
    sc_core::sc_time tWRPDEN;
    sc_core::sc_time tWRAPDEN;
    template<typename T>
    using CommandArray = std::array<T, Command::END_ENUM>;
    template<typename T>
    using BankVector = ControllerVector<Bank, T>;
    template<typename T>
    using RankVector = ControllerVector<Rank, T>;

    
    CommandArray<BankVector<sc_core::sc_time>> nextCommandByBank;
    CommandArray<RankVector<sc_core::sc_time>> nextCommandByRank;
    
    RankVector<std::queue<sc_core::sc_time>> last4ActivatesOnRank;
    sc_core::sc_time nextCommandOnBus = sc_core::SC_ZERO_TIME;
};

} // namespace DRAMSys

#endif // CHECKERDDR3_H