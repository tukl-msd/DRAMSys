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

#include "CheckerHBM3_4.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerHBM3_4::CheckerHBM3_4(const MemSpecHBM3_4& memSpec) : memSpec(memSpec)
{
    nextCommandByBank.fill({BankVector<sc_time>(memSpec.numberOfChannels * memSpec.banksPerChannel, SC_ZERO_TIME)});
    nextCommandByBankGroup.fill({BankGroupVector<sc_time>(memSpec.numberOfChannels * memSpec.bankGroupsPerChannel, SC_ZERO_TIME)});
    nextCommandByRank.fill({RankVector<sc_time>(memSpec.numberOfChannels * memSpec.ranksPerChannel, SC_ZERO_TIME)});
    nextCommandByStack.fill({StackVector<sc_time>(memSpec.numberOfChannels * memSpec.ranksPerChannel * memSpec.stacksPerChannel, SC_ZERO_TIME)});
    last4ActivatesOnRank = RankVector<std::queue<sc_time>>(memSpec.numberOfChannels * memSpec.ranksPerChannel);
    bankwiseRefreshCounter = StackVector<unsigned>(memSpec.numberOfChannels * memSpec.ranksPerChannel * memSpec.stacksPerChannel, 0);
    nextCommandOnRasBus = std::vector<sc_time>(memSpec.numberOfChannels, SC_ZERO_TIME);
    nextCommandOnCasBus = std::vector<sc_time>(memSpec.numberOfChannels, SC_ZERO_TIME);

    tBURST = ((memSpec.defaultBurstLength / memSpec.dataRate) * memSpec.tCK);
    tWRPRE = memSpec.tWL + tBURST + memSpec.tWR;
    tWRRDS = memSpec.tWL + tBURST + memSpec.tWTRS;
    tWRRDL = memSpec.tWL + tBURST + memSpec.tWTRL;
}

Stack CheckerHBM3_4::getStackOfPseudoChannel(const tlm_generic_payload& payload, int controllerID) const
{
    // SID index unique per pseudo channel across all channels
    const auto rank = controllerID * memSpec.ranksPerChannel + static_cast<unsigned>(ControllerExtension::getRank(payload));
    const auto stack = static_cast<unsigned>(ControllerExtension::getStack(payload));
    return Stack(rank * memSpec.stacksPerChannel + stack);
}

sc_time CheckerHBM3_4::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload, int controllerID) const
{
    // State is indexed across all channels: component index = controllerID * componentsPerChannel + index in channel
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + static_cast<std::size_t>(ControllerExtension::getBank(payload)));
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const Rank rank = Rank(controllerID * memSpec.ranksPerChannel + static_cast<std::size_t>(ControllerExtension::getRank(payload)));
    const Stack stack = getStackOfPseudoChannel(payload, controllerID);

    sc_time earliestTimeToStart = sc_time_stamp();

    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][bank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBankGroup[command][bankGroup]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByRank[command][rank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByStack[command][stack]);
    if (command.isRasCommand())
    {
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnRasBus[controllerID]);
    }
    if (command.isCasCommand())
    {
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnCasBus[controllerID]);
    }
    // Only PREpb and PREab may use a falling CK edge; every other command starts on a rising edge
    // (JESD270-4A / JESD238 row and column command truth tables)
    if (command != Command::PREPB && command != Command::PREAB)
        earliestTimeToStart = alignAtNext(earliestTimeToStart, memSpec.tCK);

    return earliestTimeToStart;
}

void CheckerHBM3_4::insert(Command command, const tlm_generic_payload& payload, int controllerID)
{
    const std::size_t bankRelative = static_cast<std::size_t>(ControllerExtension::getBank(payload));
    const Bank bank = Bank(controllerID * memSpec.banksPerChannel + bankRelative);
    const BankGroup bankGroup = BankGroup(controllerID * memSpec.bankGroupsPerChannel + static_cast<std::size_t>(ControllerExtension::getBankGroup(payload)));
    const Rank rank = Rank(controllerID * memSpec.ranksPerChannel + static_cast<std::size_t>(ControllerExtension::getRank(payload)));
    const Stack stack = getStackOfPseudoChannel(payload, controllerID);

    PRINTDEBUGMESSAGE("CheckerHBM3_4", "Changing state on bank " + std::to_string(bankRelative)
                      + " command is " + command.toString());

    const sc_time& currentTime = sc_time_stamp();
    if (command == Command::REFPB)
    {
        // Per-bank refresh sets are counted per SID (JESD270-4A Table 38, note 3)
        bankwiseRefreshCounter[stack] = (bankwiseRefreshCounter[stack] + 1) % memSpec.banksPerStack;
    }

    switch (command)
    {
    case Command::RD:
    {
        // Bank (RD,PREPB) memSpec.tRTP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTP;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,RD) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,RDA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,RD) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,RDA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,WR) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,WRA) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,PREAB) memSpec.tRTP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,RD) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = static_cast<unsigned>(rank) * memSpec.stacksPerChannel; i < (static_cast<unsigned>(rank) + 1) * memSpec.stacksPerChannel; i++)
            {
                Stack currentStack{i};
                
                if (currentStack == stack)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByStack[Command::RD][currentStack];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RD,RDA) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = static_cast<unsigned>(rank) * memSpec.stacksPerChannel; i < (static_cast<unsigned>(rank) + 1) * memSpec.stacksPerChannel; i++)
            {
                Stack currentStack{i};
                
                if (currentStack == stack)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByStack[Command::RDA][currentStack];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        

        break;
    }
    case Command::RDA:
    {
        // Bank (RDA,ACT) ((memSpec.tRTP + memSpec.tRP) - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + ((memSpec.tRTP + memSpec.tRP) - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (RDA,REFPB) (memSpec.tRTP + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRTP + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,RD) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,RDA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,RD) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,RDA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,WR) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,WRA) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,PREAB) memSpec.tRTP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,REFAB) (memSpec.tRTP + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRTP + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,RD) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = static_cast<unsigned>(rank) * memSpec.stacksPerChannel; i < (static_cast<unsigned>(rank) + 1) * memSpec.stacksPerChannel; i++)
            {
                Stack currentStack{i};
                
                if (currentStack == stack)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByStack[Command::RD][currentStack];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (RDA,RDA) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = static_cast<unsigned>(rank) * memSpec.stacksPerChannel; i < (static_cast<unsigned>(rank) + 1) * memSpec.stacksPerChannel; i++)
            {
                Stack currentStack{i};
                
                if (currentStack == stack)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByStack[Command::RDA][currentStack];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        

        break;
    }
    case Command::WR:
    {
        // Bank (WR,PREPB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WR,RDA) ((memSpec.tWL + tBURST) + std::max((memSpec.tWR - memSpec.tRTP), memSpec.tWTRL)) [] SameComponent()
        {
            const sc_time constraint = currentTime + ((memSpec.tWL + tBURST) + std::max((memSpec.tWR - memSpec.tRTP), memSpec.tWTRL));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,WR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,WRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,RD) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,RDA) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,WR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,WRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,RD) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,RDA) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::WRA:
    {
        // Bank (WRA,ACT) ((tWRPRE + memSpec.tRP) - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + ((tWRPRE + memSpec.tRP) - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WRA,REFPB) (tWRPRE + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,WR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,WRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,RD) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,RDA) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,WR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,WRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,RD) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,RDA) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,REFAB) (tWRPRE + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::ACT:
    {
        // Bank (ACT,RD) (memSpec.tRCDRD + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCDRD + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RD][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,RDA) (memSpec.tRCDRD + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCDRD + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WR) (memSpec.tRCDWR + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCDWR + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WRA) (memSpec.tRCDWR + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCDWR + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,PREPB) (memSpec.tRAS + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRAS + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,ACT) memSpec.tRC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRC;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,REFPB) (memSpec.tRC + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRC + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (ACT,ACT) memSpec.tRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::ACT][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (ACT,REFPB) (memSpec.tRRDL + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRRDL + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::REFPB][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,ACT) memSpec.tRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFPB) (memSpec.tRRDS + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRRDS + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,PREAB) (memSpec.tRAS + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRAS + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFAB) (memSpec.tRC + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRC + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // tFAW counts ACT (from its second rising CK edge) and REFpb (JESD270-4A Table 108 note 10)
        last4ActivatesOnRank[rank].push(command == Command::ACT ? currentTime + memSpec.tCK : currentTime);

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            const sc_time windowEnd = last4ActivatesOnRank[rank].front() + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd - memSpec.tCK);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }

            last4ActivatesOnRank[rank].pop();
        }


        break;
    }
    case Command::PREPB:
    {
        // Bank (PREPB,ACT) (memSpec.tRP - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRP - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (PREPB,REFPB) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
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
        
        // Rank (PREPB,REFAB) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PREAB:
    {
        // Rank (PREAB,ACT) (memSpec.tRP - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRP - memSpec.tCK);
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
        
        // Rank (PREAB,REFAB) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,REFPB) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::REFAB:
    {
        // Rank (REFAB,ACT) (memSpec.tRFC - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRFC - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFAB) memSpec.tRFC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFPB) memSpec.tRFC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::REFPB:
    {
        // Bank (REFPB,ACT) (memSpec.tRFCPB - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRFCPB - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (REFPB,REFPB) memSpec.tRFCPB [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCPB;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,ACT) (memSpec.tRREFD - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRREFD - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,REFPB) memSpec.tRREFD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRREFD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,REFAB) memSpec.tRFCPB [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCPB;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,PREAB) memSpec.tRFCPB [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCPB;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // tFAW counts ACT (from its second rising CK edge) and REFpb (JESD270-4A Table 108 note 10)
        last4ActivatesOnRank[rank].push(command == Command::ACT ? currentTime + memSpec.tCK : currentTime);

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            const sc_time windowEnd = last4ActivatesOnRank[rank].front() + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd - memSpec.tCK);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, windowEnd);
            }

            last4ActivatesOnRank[rank].pop();
        }

        // Stack (REFPB,REFPB) memSpec.tRFCPB [BankwiseRefreshCounter(counter=0, inversed=False)] SameComponent()
        {
            if (bankwiseRefreshCounter[stack] == 0)
            {
            const sc_time constraint = currentTime + memSpec.tRFCPB;
            sc_time &earliestTimeToStart = nextCommandByStack[Command::REFPB][stack];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }


        break;
    }
    default:
        SC_REPORT_FATAL("CheckerHBM3_4", ("Command " + command.toString() + " is not supported for HBM3/HBM4: "
                                          "power-down and self refresh are common to both pseudo channels "
                                          "(use PowerDownPolicy NoPowerDown), refresh management is not modeled").c_str());
        break;
    }
    if (command.isRasCommand())
    {
        nextCommandOnRasBus[controllerID] = std::max(nextCommandOnRasBus[controllerID], currentTime + memSpec.getCommandLength(command));
    }
    if (command.isCasCommand())
    {
        nextCommandOnCasBus[controllerID] = std::max(nextCommandOnCasBus[controllerID], currentTime + memSpec.getCommandLength(command));
    }
}

} // namespace DRAMSys
