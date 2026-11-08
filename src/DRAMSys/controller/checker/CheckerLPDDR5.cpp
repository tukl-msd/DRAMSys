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

#include "CheckerLPDDR5.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"

#include <algorithm>
#include <cmath>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

namespace
{

sc_time positiveDifference(const sc_time& minuend, const sc_time& subtrahend)
{
    return minuend > subtrahend ? minuend - subtrahend : SC_ZERO_TIME;
}

} // namespace

CheckerLPDDR5::CheckerLPDDR5(const MemSpecLPDDR5& memSpec) : memSpec(memSpec)
{
    nextCommandByBank.fill({BankVector<sc_time>(memSpec.numberOfChannels * memSpec.banksPerChannel, SC_ZERO_TIME)});
    nextCommandByBankGroup.fill({BankGroupVector<sc_time>(memSpec.numberOfChannels * memSpec.bankGroupsPerChannel, SC_ZERO_TIME)});
    nextCommandByRank.fill({RankVector<sc_time>(memSpec.numberOfChannels * memSpec.ranksPerChannel, SC_ZERO_TIME)});
    last4ActivatesOnRank = RankVector<std::deque<sc_time>>(memSpec.numberOfChannels * memSpec.ranksPerChannel);
    nextCommandOnBus = std::vector<sc_time>(memSpec.numberOfChannels, SC_ZERO_TIME);
    pendingActivate2 = std::vector<std::deque<PendingActivate>>(memSpec.numberOfChannels);

    tRDPRE16 = memSpec.tBURST16 + memSpec.tRBTP;
    tWRPRE16 = memSpec.tWL + memSpec.tBURST16 + memSpec.tCK + memSpec.tWR;
    tWRRD_L16 = memSpec.tWL + memSpec.tBURSTMAX16 + memSpec.tWTR_L;
    tWRRD_S16 = memSpec.tWL + memSpec.tBURST16 + memSpec.tWTR_S;
    tRDRD_R16 = memSpec.tBURST16 + memSpec.tRTRS;
    tWRWR_R16 = memSpec.tBURST16 + memSpec.tRTRS;
    tRDWR_R16 = positiveDifference(memSpec.tRL + memSpec.tBURST16 + memSpec.tRTRS, memSpec.tWL);
    tWRRD_R16 = positiveDifference(memSpec.tWL + memSpec.tBURST16 + memSpec.tRTRS, memSpec.tRL);
    tRDPRE32 = memSpec.tBURST32 + memSpec.tRBTP;
    tWRPRE32 = memSpec.tWL + memSpec.tBURST32 + memSpec.tCK + memSpec.tWR;
    tWRRD_L32 = memSpec.tWL + memSpec.tBURSTMAX32 + memSpec.tWTR_L;
    tWRRD_S32 = memSpec.tWL + memSpec.tBURST32 + memSpec.tWTR_S;
    tRDRD_R32 = memSpec.tBURST32 + memSpec.tRTRS;
    tWRWR_R32 = memSpec.tBURST32 + memSpec.tRTRS;
    tRDWR_R32 = positiveDifference(memSpec.tRL + memSpec.tBURST32 + memSpec.tRTRS, memSpec.tWL);
    tWRRD_R32 = positiveDifference(memSpec.tWL + memSpec.tBURST32 + memSpec.tRTRS, memSpec.tRL);
}

Bank CheckerLPDDR5::getPairedBank(Bank bank) const
{
    // REFP2B payloads carry the lower bank of a pair; the partner is per2BankOffset banks above
    const unsigned offset = memSpec.getPer2BankOffset();
    const auto bankInRank = static_cast<unsigned>(static_cast<std::size_t>(bank) % memSpec.banksPerRank);
    if (offset == 0)
        return bank;
    if ((bankInRank / offset) % 2 == 0)
        return Bank(static_cast<std::size_t>(bank) + offset);
    return Bank(static_cast<std::size_t>(bank) - offset);
}

std::optional<sc_time> CheckerLPDDR5::activateDelay(Command command, bool sameBank, bool samePair,
                                                    bool sameBankGroup, bool sameRank) const
{
    sc_time delay = SC_ZERO_TIME;
    // Bank (ACT,RD) memSpec.tRCDRD
    if (command == Command::RD && sameBank)
        delay = std::max(delay, memSpec.tRCDRD);
    // Bank (ACT,RDA) memSpec.tRCDRD
    if (command == Command::RDA && sameBank)
        delay = std::max(delay, memSpec.tRCDRD);
    // Bank (ACT,WR) memSpec.tRCDWR
    if (command == Command::WR && sameBank)
        delay = std::max(delay, memSpec.tRCDWR);
    // Bank (ACT,WRA) memSpec.tRCDWR
    if (command == Command::WRA && sameBank)
        delay = std::max(delay, memSpec.tRCDWR);
    // Bank (ACT,PREPB) memSpec.tRAS
    if (command == Command::PREPB && sameBank)
        delay = std::max(delay, memSpec.tRAS);
    // Bank (ACT,ACT) memSpec.tRCpb
    if (command == Command::ACT && sameBank)
        delay = std::max(delay, memSpec.tRCpb);
    // BankPair (ACT,REFP2B) memSpec.tRCpb
    if (command == Command::REFP2B && samePair)
        delay = std::max(delay, memSpec.tRCpb);
    // Bank (ACT,REFPB) memSpec.tRCpb
    if (command == Command::REFPB && sameBank)
        delay = std::max(delay, memSpec.tRCpb);
    // Rank (ACT,ACT) memSpec.tRRD
    if (command == Command::ACT && sameRank)
        delay = std::max(delay, memSpec.tRRD);
    // Rank (ACT,PREAB) memSpec.tRAS
    if (command == Command::PREAB && sameRank)
        delay = std::max(delay, memSpec.tRAS);
    // Rank (ACT,REFAB) memSpec.tRCpb
    if (command == Command::REFAB && sameRank)
        delay = std::max(delay, memSpec.tRCpb);
    // Rank (ACT,REFP2B) memSpec.tRRD
    if (command == Command::REFP2B && sameRank)
        delay = std::max(delay, memSpec.tRRD);
    // Rank (ACT,REFPB) memSpec.tRRD
    if (command == Command::REFPB && sameRank)
        delay = std::max(delay, memSpec.tRRD);

    if (delay == SC_ZERO_TIME)
        return std::nullopt;
    return delay;
}

sc_time CheckerLPDDR5::activate2Bound(const PendingActivate& activate, const std::deque<PendingActivate>& pending,
                                      const std::vector<sc_time>& times) const
{
    // Earliest ACT-2: constraints of the commands inserted so far and of the ACT-2s placed before it (times)
    sc_time bound = std::max({nextCommandByBank[Command::ACT][activate.bank],
                              nextCommandByBankGroup[Command::ACT][activate.bankGroup],
                              nextCommandByRank[Command::ACT][activate.rank]});
    std::vector<sc_time> fawEvents(last4ActivatesOnRank[activate.rank].begin(), last4ActivatesOnRank[activate.rank].end());
    for (std::size_t i = 0; i < times.size(); i++)
    {
        const PendingActivate& previous = pending[i];
        if (auto delay = activateDelay(Command::ACT, previous.bank == activate.bank, false,
                                       previous.bankGroup == activate.bankGroup, previous.rank == activate.rank))
            bound = std::max(bound, times[i] + *delay);
        if (previous.rank == activate.rank)
            fawEvents.push_back(times[i]);
    }
    if (fawEvents.size() >= 4)
    {
        std::sort(fawEvents.begin(), fawEvents.end());
        bound = std::max(bound, fawEvents[fawEvents.size() - 4] + memSpec.tFAW);
    }
    return bound;
}

bool CheckerLPDDR5::placeActivates(const sc_time& slot, const PendingActivate* newActivate, bool columnCommand,
                                   int controllerID, std::vector<sc_time>& times) const
{
    // ACT-2s take CA cycles in order, each at its earliest time: idle cycles before slot, then cycles after slot.
    // A pending ACT-2 that can go in slot gives it up only to a READ or WRITE (columnCommand) and moves on;
    // any other command does not fit there. Fails as well if an ACT-2 would miss its tAAD deadline.
    const std::deque<PendingActivate>& pending = pendingActivate2[controllerID];
    times.clear();
    sc_time cycle = nextCommandOnBus[controllerID];
    auto place = [&](const PendingActivate& activate, const sc_time& notBefore)
    {
        sc_time time = std::max({cycle, notBefore, activate2Bound(activate, pending, times)});
        if (time == slot)
        {
            if (!columnCommand)
                return false;
            time += memSpec.tCK;
        }
        if (time > activate.deadline)
            return false;
        times.push_back(time);
        cycle = time + memSpec.tCK;
        return true;
    };
    for (const PendingActivate& activate : pending)
    {
        if (!place(activate, SC_ZERO_TIME))
            return false;
    }
    if (newActivate == nullptr)
        return true;
    // No ACT-1 between another ACTIVATE's ACT-1 and ACT-2 (JESD209-5C 7.3.1)
    if (!times.empty() && times.back() > slot)
        return false;
    return place(*newActivate, slot + memSpec.tCK);
}

bool CheckerLPDDR5::slotAllowed(Command command, Bank bank, BankGroup bankGroup, Rank rank, const sc_time& slot,
                                int controllerID) const
{
    std::vector<sc_time> times;
    if (command == Command::ACT)
    {
        // ACT-1 itself has no timing; its ACT-2 needs a free CA cycle within tAAD that meets all constraints
        const PendingActivate activate{slot + memSpec.tAAD, bank, bankGroup, rank};
        return placeActivates(slot, &activate, false, controllerID, times);
    }
    if (!placeActivates(slot, nullptr, command.isCasCommand(), controllerID, times))
        return false;

    // Timings that start at a pending ACT-2
    const std::deque<PendingActivate>& pending = pendingActivate2[controllerID];
    const bool countsForFaw = command == Command::REFP2B || command == Command::REFPB;
    std::vector<sc_time> fawEvents(last4ActivatesOnRank[rank].begin(), last4ActivatesOnRank[rank].end());
    for (std::size_t i = 0; i < pending.size(); i++)
    {
        const PendingActivate& activate = pending[i];
        const bool samePair = activate.bank == bank || activate.bank == getPairedBank(bank);
        if (auto delay = activateDelay(command, activate.bank == bank, samePair, activate.bankGroup == bankGroup,
                                       activate.rank == rank))
        {
            if (slot < times[i] + *delay)
                return false;
        }
        if (countsForFaw && activate.rank == rank)
            fawEvents.push_back(times[i]);
    }
    if (countsForFaw && fawEvents.size() >= 4)
    {
        std::sort(fawEvents.begin(), fawEvents.end());
        if (slot < fawEvents[fawEvents.size() - 4] + memSpec.tFAW)
            return false;
    }
    return true;
}

void CheckerLPDDR5::applyActivate(const PendingActivate& activate, const sc_time& currentTime)
{
    // currentTime is the CA cycle of ACT-2
    const Bank bank = activate.bank;
    const Bank pairedBank = getPairedBank(bank);
    const Rank rank = activate.rank;
    [[maybe_unused]] const BankGroup bankGroup = activate.bankGroup;

        // Bank (ACT,RD) memSpec.tRCDRD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCDRD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RD][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,RDA) memSpec.tRCDRD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCDRD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WR) memSpec.tRCDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCDWR;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WRA) memSpec.tRCDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCDWR;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,PREPB) memSpec.tRAS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRAS;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,ACT) memSpec.tRCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCpb;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankPair (ACT,REFP2B) memSpec.tRCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCpb;
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::REFP2B][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (ACT,REFPB) memSpec.tRCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCpb;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,ACT) memSpec.tRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,PREAB) memSpec.tRAS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRAS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFAB) memSpec.tRCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFP2B) memSpec.tRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFPB) memSpec.tRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // tFAW counts ACT-2 and REFpb (rolling window of four)
        last4ActivatesOnRank[rank].push_back(currentTime);

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            const sc_time windowEnd = last4ActivatesOnRank[rank].front() + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }

            last4ActivatesOnRank[rank].pop_front();
        }

}

sc_time CheckerLPDDR5::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload, int controllerID) const
{
    // State is indexed across all channels: component index = controllerID * componentsPerChannel + index in channel
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + static_cast<std::size_t>(ControllerExtension::getBank(payload)));
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const Rank rank = Rank(controllerID * memSpec.ranksPerChannel + static_cast<std::size_t>(ControllerExtension::getRank(payload)));

    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::ACT)
    {
        // The constraints bound ACT-2. ACT-1 goes one CA cycle before the earliest ACT-2, not earlier: while its
        // ACT-2 is pending no other ACTIVATE may start (JESD209-5C 7.3.1); the tAAD window absorbs CA conflicts.
        const sc_time activate2 = std::max({nextCommandByBank[command][bank], nextCommandByBankGroup[command][bankGroup],
                                            nextCommandByRank[command][rank]});
        if (activate2 > memSpec.tCK)
            earliestTimeToStart = std::max(earliestTimeToStart, activate2 - memSpec.tCK);
    }
    else
    {
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][bank]);
        if (command == Command::REFP2B)
            earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][getPairedBank(bank)]);
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBankGroup[command][bankGroup]);
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByRank[command][rank]);
    }
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnBus[controllerID]);
    while (!slotAllowed(command, bank, bankGroup, rank, earliestTimeToStart, controllerID))
        earliestTimeToStart += memSpec.tCK;

    return earliestTimeToStart;
}

void CheckerLPDDR5::insert(Command command, const tlm_generic_payload& payload, int controllerID)
{
    const std::size_t bankRelative = static_cast<std::size_t>(ControllerExtension::getBank(payload));
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + bankRelative);
    const Bank pairedBank = getPairedBank(bank);
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const std::size_t rankOffset = controllerID * memSpec.ranksPerChannel;
    const Rank rank = Rank(rankOffset + static_cast<std::size_t>(ControllerExtension::getRank(payload)));
    const unsigned burstLength = command.isCasCommand() ? ControllerExtension::getBurstLength(payload) : 0;
    sc_time& nextCommandOnChannelBus = nextCommandOnBus[controllerID];
    std::deque<PendingActivate>& pending = pendingActivate2[controllerID];

    PRINTDEBUGMESSAGE("CheckerLPDDR5", "Changing state on bank " + std::to_string(bankRelative)
                      + " command is " + command.toString());

    const sc_time& currentTime = sc_time_stamp();

    // ACT-2s placed in the idle CA cycles since the previous command (JESD209-5: ACT-2 within tAAD of ACT-1); the
    // timings that start at an ACTIVATE start there
    const PendingActivate activate{currentTime + memSpec.tAAD, bank, bankGroup, rank};
    std::vector<sc_time> activate2Times;
    if (!placeActivates(currentTime, command == Command::ACT ? &activate : nullptr, command.isCasCommand(), controllerID,
                        activate2Times))
        SC_REPORT_FATAL("CheckerLPDDR5", "ACT-2 missed its tAAD window");
    for (const sc_time& activate2 : activate2Times)
    {
        if (activate2 > currentTime || pending.empty())
            break;
        applyActivate(pending.front(), activate2);
        pending.pop_front();
    }

    switch (command)
    {
    case Command::ACT:
        pending.push_back(activate);
        break;
    case Command::RD:
    {
        // Bank (RD,PREPB) tRDPRE16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDPRE16;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (RD,PREPB) tRDPRE32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDPRE32;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,RD) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,RDA) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,WR) memSpec.tRTW_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,WRA) memSpec.tRTW_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,RD) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,RDA) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,WR) memSpec.tRTW_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RD,WRA) memSpec.tRTW_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,RD) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,RDA) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,WR) memSpec.tRTW_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,WRA) memSpec.tRTW_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,PREAB) tRDPRE16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDPRE16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,RD) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,RDA) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,WR) memSpec.tRTW_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,WRA) memSpec.tRTW_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,PREAB) tRDPRE32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDPRE32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RD,RD) tRDRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RD,RDA) tRDRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RD,WR) tRDWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RD,WRA) tRDWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RD,RD) tRDRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RD,RDA) tRDRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RD,WR) tRDWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RD,WRA) tRDWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        

        break;
    }
    case Command::RDA:
    {
        // Bank (RDA,PREPB) tRDPRE16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDPRE16;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (RDA,ACT) (tRDPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tRDPRE16 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankPair (RDA,REFP2B) (tRDPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tRDPRE16 + memSpec.tRPpb);
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::REFP2B][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Bank (RDA,REFPB) (tRDPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tRDPRE16 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (RDA,PREPB) tRDPRE32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDPRE32;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (RDA,ACT) (tRDPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tRDPRE32 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankPair (RDA,REFP2B) (tRDPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tRDPRE32 + memSpec.tRPpb);
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::REFP2B][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Bank (RDA,REFPB) (tRDPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tRDPRE32 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,RD) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,RDA) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,WR) memSpec.tRTW_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,WRA) memSpec.tRTW_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,RD) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,RDA) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,WR) memSpec.tRTW_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (RDA,WRA) memSpec.tRTW_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,RD) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,RDA) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,WR) memSpec.tRTW_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,WRA) memSpec.tRTW_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,PREAB) tRDPRE16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDPRE16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,REFAB) (tRDPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tRDPRE16 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,RD) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,RDA) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,WR) memSpec.tRTW_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,WRA) memSpec.tRTW_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tRTW_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,PREAB) tRDPRE32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDPRE32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,REFAB) (tRDPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tRDPRE32 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RDA,RD) tRDRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RDA,RDA) tRDRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RDA,WR) tRDWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RDA,WRA) tRDWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tRDWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RDA,RD) tRDRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RDA,RDA) tRDRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RDA,WR) tRDWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (RDA,WRA) tRDWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tRDWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        

        break;
    }
    case Command::WR:
    {
        // Bank (WR,PREPB) tWRPRE16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRPRE16;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WR,RDA) std::max(tWRRD_L16, (tWRPRE16 - tRDPRE16)) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + std::max(tWRRD_L16, (tWRPRE16 - tRDPRE16));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WR,PREPB) tWRPRE32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRPRE32;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WR,RDA) std::max(tWRRD_L32, (tWRPRE32 - tRDPRE32)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + std::max(tWRRD_L32, (tWRPRE32 - tRDPRE32));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,WR) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,WRA) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,RD) tWRRD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,RDA) tWRRD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,WR) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,WRA) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,RD) tWRRD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WR,RDA) tWRRD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,WR) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,WRA) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,RD) tWRRD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,RDA) tWRRD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,PREAB) tWRPRE16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRPRE16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,WR) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,WRA) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,RD) tWRRD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,RDA) tWRRD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,PREAB) tWRPRE32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRPRE32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WR,WR) tWRWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WR,WRA) tWRWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WR,RD) tWRRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WR,RDA) tWRRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WR,WR) tWRWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WR,WRA) tWRWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WR,RD) tWRRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WR,RDA) tWRRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        

        break;
    }
    case Command::WRA:
    {
        // Bank (WRA,ACT) (tWRPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tWRPRE16 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankPair (WRA,REFP2B) (tWRPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tWRPRE16 + memSpec.tRPpb);
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::REFP2B][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Bank (WRA,REFPB) (tWRPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tWRPRE16 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WRA,ACT) (tWRPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tWRPRE32 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankPair (WRA,REFP2B) (tWRPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tWRPRE32 + memSpec.tRPpb);
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::REFP2B][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Bank (WRA,REFPB) (tWRPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tWRPRE32 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,WR) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,WRA) memSpec.tCCD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,RD) tWRRD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,RDA) tWRRD_L16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_L16;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,WR) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,WRA) memSpec.tCCD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,RD) tWRRD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankGroup (WRA,RDA) tWRRD_L32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_L32;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,WR) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,WRA) memSpec.tCCD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,RD) tWRRD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,RDA) tWRRD_S16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_S16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,PREAB) tWRPRE16 [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRPRE16;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,REFAB) (tWRPRE16 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + (tWRPRE16 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,WR) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,WRA) memSpec.tCCD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,RD) tWRRD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,RDA) tWRRD_S32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_S32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,PREAB) tWRPRE32 [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRPRE32;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,REFAB) (tWRPRE32 + memSpec.tRPpb) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (tWRPRE32 + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WRA,WR) tWRWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WRA,WRA) tWRWR_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRWR_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WRA,RD) tWRRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WRA,RDA) tWRRD_R16 [LastBurstLength(burst_length=32, inversed=True)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + tWRRD_R16;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WRA,WR) tWRWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WRA,WRA) tWRWR_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRWR_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WRA,RD) tWRRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        
        // Channel (WRA,RDA) tWRRD_R32 [LastBurstLength(burst_length=32, inversed=False)] Different(level=<ComponentLevel.Rank: 3>)
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + tWRRD_R32;
            for (unsigned int i = rankOffset; i < rankOffset + memSpec.ranksPerChannel; i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            }
        }
        

        break;
    }
    case Command::PREPB:
    {
        // Bank (PREPB,ACT) memSpec.tRPpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPpb;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankPair (PREPB,REFP2B) memSpec.tRPpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPpb;
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::REFP2B][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (PREPB,REFPB) memSpec.tRPpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPpb;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,PREPB) memSpec.tPPD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPPD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,PREAB) memSpec.tPPD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPPD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,REFAB) memSpec.tRPpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PREAB:
    {
        // Rank (PREAB,ACT) memSpec.tRPab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,PREPB) memSpec.tPPD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPPD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,PREAB) memSpec.tPPD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPPD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,REFAB) memSpec.tRPab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,REFP2B) memSpec.tRPab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,REFPB) memSpec.tRPab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::REFAB:
    {
        // Rank (REFAB,ACT) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,PREAB) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFAB) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFP2B) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFPB) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::REFP2B:
    {
        // BankPair (REFP2B,ACT) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // BankPair (REFP2B,REFP2B) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            for (Bank currentBank : {bank, pairedBank})
            {
                sc_time &earliestTimeToStart = nextCommandByBank[Command::REFP2B][currentBank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (REFP2B,ACT) memSpec.tpbR2act [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tpbR2act;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFP2B,REFP2B) memSpec.tpbR2pbR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tpbR2pbR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFP2B,REFAB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFP2B,PREAB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // tFAW counts ACT-2 and REFpb (rolling window of four)
        last4ActivatesOnRank[rank].push_back(currentTime);

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            const sc_time windowEnd = last4ActivatesOnRank[rank].front() + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }

            last4ActivatesOnRank[rank].pop_front();
        }


        break;
    }
    case Command::REFPB:
    {
        // Bank (REFPB,ACT) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (REFPB,REFPB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,ACT) memSpec.tpbR2act [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tpbR2act;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,REFPB) memSpec.tpbR2pbR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tpbR2pbR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,REFAB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,PREAB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // tFAW counts ACT-2 and REFpb (rolling window of four)
        last4ActivatesOnRank[rank].push_back(currentTime);

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            const sc_time windowEnd = last4ActivatesOnRank[rank].front() + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFP2B][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }

            last4ActivatesOnRank[rank].pop_front();
        }


        break;
    }
    default:
        SC_REPORT_FATAL("CheckerLPDDR5", ("Command " + command.toString() + " is not supported for LPDDR5").c_str());
        break;
    }
    nextCommandOnChannelBus = std::max(nextCommandOnChannelBus, currentTime + memSpec.getCommandLength(command));
}

} // namespace DRAMSys
