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

#include "CheckerHBM2.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerHBM2::CheckerHBM2(const MemSpecHBM2& memSpec) : memSpec(memSpec)
{
    
    nextCommandByBank.fill({BankVector<sc_time>(memSpec.banksPerChannel, SC_ZERO_TIME)});
    nextCommandByBankGroup.fill({BankGroupVector<sc_time>(memSpec.bankGroupsPerChannel, SC_ZERO_TIME)});
    nextCommandByRank.fill({RankVector<sc_time>(memSpec.ranksPerChannel, SC_ZERO_TIME)});
    nextCommandByStack.fill({StackVector<sc_time>(memSpec.stacksPerChannel, SC_ZERO_TIME)});
    last4ActivatesOnRank = RankVector<std::queue<sc_time>>(memSpec.ranksPerChannel);

    tBURST = ((memSpec.defaultBurstLength / memSpec.dataRate) * memSpec.tCK);
    tRDPDE = (((memSpec.tRL + memSpec.tPL) + tBURST) + memSpec.tCK);
    tRDSRE = tRDPDE;
    tWRPRE = ((memSpec.tWL + tBURST) + memSpec.tWR);
    tWRPDE = ((((memSpec.tWL + memSpec.tPL) + tBURST) + memSpec.tCK) + memSpec.tWR);
    tWRAPDE = ((((memSpec.tWL + memSpec.tPL) + tBURST) + memSpec.tCK) + memSpec.tWR);
    tWRRDS = ((memSpec.tWL + tBURST) + memSpec.tWTRS);
    tWRRDL = ((memSpec.tWL + tBURST) + memSpec.tWTRL);
    
    bankwiseRefreshCounter = ControllerVector<Rank, unsigned>(memSpec.ranksPerChannel);
}

sc_time CheckerHBM2::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload) const
{
    Bank bank = ControllerExtension::getBank(payload);
    BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    Rank rank = ControllerExtension::getRank(payload);
    Stack stack = ControllerExtension::getStack(payload);
    

    sc_time earliestTimeToStart = sc_time_stamp();

    
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][bank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBankGroup[command][bankGroup]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByRank[command][rank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByStack[command][stack]);
    if (command.isRasCommand())
    {
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnRasBus);
    }
    if (command.isCasCommand())
    {
        earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnCasBus);
    }

    return earliestTimeToStart;
}

void CheckerHBM2::insert(Command command, const tlm_generic_payload& payload)
{
    const Bank bank = ControllerExtension::getBank(payload);
    const BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    const Rank rank = ControllerExtension::getRank(payload);
    const Stack stack = ControllerExtension::getStack(payload);
    

    PRINTDEBUGMESSAGE("CheckerHBM2", "Changing state on bank " + std::to_string(static_cast<std::size_t>(bank))
                      + " command is " + command.toString());
    
    const sc_time& currentTime = sc_time_stamp();
    if (command == Command::REFPB || command == Command::RFMPB)
    {
        bankwiseRefreshCounter[rank] = (bankwiseRefreshCounter[rank] + 1) % memSpec.banksPerRank;
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
        
        // Rank (RD,PREAB) memSpec.tRTP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,PDEA) tRDPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,PDEP) tRDPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
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
        
        // Rank (RD,MWR) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,WRA) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,MWRA) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (RD,RD) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = memSpec.stacksPerChannel * static_cast<unsigned>(0); i < memSpec.stacksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Stack currentStack{i};
                
                if (currentStack == stack)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByStack[Command::RD][currentStack];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RD,RDA) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = memSpec.stacksPerChannel * static_cast<unsigned>(0); i < memSpec.stacksPerChannel * (1 + static_cast<unsigned>(0)); i++)
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
        
        // BankGroup (WR,MWR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,WRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,MWRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
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
        
        // Rank (WR,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,PDEA) tWRPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,WR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,MWR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,WRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,MWRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
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
        

        break;
    }
    case Command::MWR:
    {
        // Bank (MWR,PREPB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWR,RDA) ((memSpec.tWL + tBURST) + std::max((memSpec.tWR - memSpec.tRTP), memSpec.tWTRL)) [] SameComponent()
        {
            const sc_time constraint = currentTime + ((memSpec.tWL + tBURST) + std::max((memSpec.tWR - memSpec.tRTP), memSpec.tWTRL));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,WR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,MWR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,WRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,MWRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,RD) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,RDA) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,PDEA) tWRPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,WR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,MWR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,WRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,MWRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,RD) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,RDA) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
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
        
        // Rank (RDA,PDEA) tRDPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,PDEP) tRDPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
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
        
        // Rank (RDA,MWR) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,WRA) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,MWRA) memSpec.tRTW [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTW;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,REFAB) (memSpec.tRTP + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRTP + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,PREAB) memSpec.tRTP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRTP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,SREFEN) std::max((memSpec.tRTP + memSpec.tRP), tRDSRE) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max((memSpec.tRTP + memSpec.tRP), tRDSRE);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (RDA,RD) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = memSpec.stacksPerChannel * static_cast<unsigned>(0); i < memSpec.stacksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Stack currentStack{i};
                
                if (currentStack == stack)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByStack[Command::RD][currentStack];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RDA,RDA) memSpec.tCCDR [] Different(level=<ComponentLevel.Stack: 7>)
        {
            const sc_time constraint = currentTime + memSpec.tCCDR;
            for (unsigned int i = memSpec.stacksPerChannel * static_cast<unsigned>(0); i < memSpec.stacksPerChannel * (1 + static_cast<unsigned>(0)); i++)
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
        
        // BankGroup (WRA,MWR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,WRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,MWRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
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
        
        // Rank (WRA,PDEA) tWRAPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,PDEP) tWRAPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,WR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,MWR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,WRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,MWRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
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
        
        // Rank (WRA,REFAB) (tWRPRE + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,SREFEN) (tWRPRE + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::MWRA:
    {
        // Bank (MWRA,ACT) ((tWRPRE + memSpec.tRP) - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + ((tWRPRE + memSpec.tRP) - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWRA,REFPB) (tWRPRE + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,WR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,MWR) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,WRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,MWRA) memSpec.tCCDL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,RD) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,RDA) tWRRDL [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDL;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,PDEA) tWRAPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,PDEP) tWRAPDE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,WR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,MWR) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,WRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,MWRA) memSpec.tCCDS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,RD) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,RDA) tWRRDS [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRDS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,REFAB) (tWRPRE + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,SREFEN) (tWRPRE + memSpec.tRP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::ACT:
    {
        // Bank (ACT,PREPB) (memSpec.tRAS + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRAS + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
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
        
        // Bank (ACT,MWR) (memSpec.tRCDWR + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCDWR + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WRA) (memSpec.tRCDWR + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCDWR + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,MWRA) (memSpec.tRCDWR + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCDWR + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
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
        
        // Rank (ACT,REFAB) (memSpec.tRC + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRC + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,SREFEN) (memSpec.tRC + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRC + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,PREAB) (memSpec.tRAS + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRAS + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFPB) (memSpec.tRRDS + memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRRDS + memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        last4ActivatesOnRank[rank].push(currentTime + memSpec.getCommandLength(command));

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            sc_time constraint = last4ActivatesOnRank[rank].front() - memSpec.getCommandLength(command) + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
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
        
        // Rank (PREPB,REFAB) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,SREFEN) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::REFPB:
    {
        // Bank (REFPB,ACT) (memSpec.tRFCSB - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRFCSB - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (REFPB,REFPB) memSpec.tRFCSB [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCSB;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (REFPB,REFPB) memSpec.tRREFD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRREFD;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::REFPB][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (REFPB,REFAB) memSpec.tRFCSB [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCSB;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::REFAB][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (REFPB,PREAB) memSpec.tRFCSB [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCSB;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::PREAB][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (REFPB,SREFEN) memSpec.tRFCSB [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCSB;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::SREFEN][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,ACT) (memSpec.tRREFD - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRREFD - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,REFPB) memSpec.tRFCSB [BankwiseRefreshCounter(counter=0, inversed=False)] SameComponent()
        {
            if (bankwiseRefreshCounter[rank] == 0)
            {
            const sc_time constraint = currentTime + memSpec.tRFCSB;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (REFPB,REFPB) memSpec.tRREFD [BankwiseRefreshCounter(counter=0, inversed=True)] SameComponent()
        {
            if (bankwiseRefreshCounter[rank] != 0)
            {
            const sc_time constraint = currentTime + memSpec.tRREFD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        last4ActivatesOnRank[rank].push(currentTime + memSpec.getCommandLength(command));

        if (last4ActivatesOnRank[rank].size() >= 4)
        {
            sc_time constraint = last4ActivatesOnRank[rank].front() - memSpec.getCommandLength(command) + memSpec.tFAW;
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
            {
                sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }

            last4ActivatesOnRank[rank].pop();
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
        
        // Rank (PREAB,SREFEN) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
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
        
        // Rank (REFAB,SREFEN) memSpec.tRFC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PDEA:
    {
        // Rank (PDEA,PDXA) memSpec.tPD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDXA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PDEP:
    {
        // Rank (PDEP,PDXP) memSpec.tPD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDXP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PDXA:
    {
        // Rank (PDXA,PDEA) memSpec.tCKE [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCKE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,REFPB) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,PREPB) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,PREAB) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,RD) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,RDA) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,WR) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,WRA) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,ACT) (memSpec.tXP - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tXP - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PDXP:
    {
        // Rank (PDXP,PDEP) memSpec.tCKE [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCKE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXP,REFAB) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXP,REFPB) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXP,SREFEN) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXP,ACT) (memSpec.tXP - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tXP - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::SREFEX:
    {
        // Rank (SREFEX,ACT) (memSpec.tXS - memSpec.tCK) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tXS - memSpec.tCK);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,REFPB) memSpec.tXS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,REFAB) memSpec.tXS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,PDEP) memSpec.tXS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,SREFEN) memSpec.tXS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,SREFEX) memSpec.tCKESR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCKESR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEX][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    
    }
    if (command.isRasCommand())
    {
        nextCommandOnRasBus = std::max(nextCommandOnRasBus, currentTime + memSpec.getCommandLength(command));
    }
    if (command.isCasCommand())
    {
        nextCommandOnCasBus = std::max(nextCommandOnCasBus, currentTime + memSpec.getCommandLength(command));
    }
}

} // namespace DRAMSys