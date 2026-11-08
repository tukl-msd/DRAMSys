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

#ifndef CHECKERLPDDR5_H
#define CHECKERLPDDR5_H

#include "DRAMSys/configuration/memspec/MemSpecLPDDR5.h"
#include "DRAMSys/controller/checker/CheckerIF.h"

#include <deque>
#include <optional>
#include <vector>

namespace DRAMSys
{

class CheckerLPDDR5 final : public CheckerIF
{
public:
    explicit CheckerLPDDR5(const MemSpecLPDDR5& memSpec);
    [[nodiscard]] sc_core::sc_time timeToSatisfyConstraints(Command command, const tlm::tlm_generic_payload& payload, int controllerID) const override;
    void insert(Command command, const tlm::tlm_generic_payload& payload, int controllerID) override;

private:
    const MemSpecLPDDR5& memSpec;

    sc_core::sc_time tRDPRE16;
    sc_core::sc_time tWRPRE16;
    sc_core::sc_time tWRRD_L16;
    sc_core::sc_time tWRRD_S16;
    sc_core::sc_time tRDRD_R16;
    sc_core::sc_time tWRWR_R16;
    sc_core::sc_time tRDWR_R16;
    sc_core::sc_time tWRRD_R16;
    sc_core::sc_time tRDPRE32;
    sc_core::sc_time tWRPRE32;
    sc_core::sc_time tWRRD_L32;
    sc_core::sc_time tWRRD_S32;
    sc_core::sc_time tRDRD_R32;
    sc_core::sc_time tWRWR_R32;
    sc_core::sc_time tRDWR_R32;
    sc_core::sc_time tWRRD_R32;

    template<typename T>
    using CommandArray = std::array<T, Command::END_ENUM>;
    template<typename T>
    using BankVector = ControllerVector<Bank, T>;
    template<typename T>
    using BankGroupVector = ControllerVector<BankGroup, T>;
    template<typename T>
    using RankVector = ControllerVector<Rank, T>;

    CommandArray<BankVector<sc_core::sc_time>> nextCommandByBank;
    CommandArray<BankGroupVector<sc_core::sc_time>> nextCommandByBankGroup;
    CommandArray<RankVector<sc_core::sc_time>> nextCommandByRank;

    // ACT-2 and REFpb times of the last (up to three) tFAW events
    RankVector<std::deque<sc_core::sc_time>> last4ActivatesOnRank;

    // ACTIVATE whose ACT-2 still needs a free CA cycle, at the latest at deadline (ACT-1 + tAAD)
    struct PendingActivate
    {
        sc_core::sc_time deadline;
        Bank bank;
        BankGroup bankGroup;
        Rank rank;
    };

    // Per controller (channel): CA bus and pending ACT-2s, oldest first
    std::vector<sc_core::sc_time> nextCommandOnBus;
    std::vector<std::deque<PendingActivate>> pendingActivate2;

    [[nodiscard]] Bank getPairedBank(Bank bank) const;
    [[nodiscard]] sc_core::sc_time activate2Bound(const PendingActivate& activate, const std::deque<PendingActivate>& pending,
                                                  const std::vector<sc_core::sc_time>& times) const;
    [[nodiscard]] bool placeActivates(const sc_core::sc_time& slot, const PendingActivate* newActivate,
                                      bool columnCommand, int controllerID, std::vector<sc_core::sc_time>& times) const;
    [[nodiscard]] bool slotAllowed(Command command, Bank bank, BankGroup bankGroup, Rank rank, const sc_core::sc_time& slot,
                                   int controllerID) const;
    [[nodiscard]] std::optional<sc_core::sc_time> activateDelay(Command command, bool sameBank, bool samePair,
                                                                bool sameBankGroup, bool sameRank) const;
    void applyActivate(const PendingActivate& activate, const sc_core::sc_time& currentTime);
};

} // namespace DRAMSys

#endif // CHECKERLPDDR5_H
