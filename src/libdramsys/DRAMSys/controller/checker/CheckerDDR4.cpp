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

#include "CheckerDDR4.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerDDR4::CheckerDDR4(const MemSpecDDR4& memSpec) : memSpec(memSpec)
{
    
    nextCommandByBank.fill({BankVector<sc_time>(memSpec.banksPerChannel, SC_ZERO_TIME)});
    nextCommandByBankGroup.fill({BankGroupVector<sc_time>(memSpec.bankGroupsPerChannel, SC_ZERO_TIME)});
    nextCommandByRank.fill({RankVector<sc_time>(memSpec.ranksPerChannel, SC_ZERO_TIME)});
    last4ActivatesOnRank = RankVector<std::queue<sc_time>>(memSpec.ranksPerChannel);

    tBURST = ((memSpec.defaultBurstLength / memSpec.dataRate) * memSpec.tCK);
    tRDWR = ((((memSpec.tRL + tBURST) + memSpec.tCK) - memSpec.tWL) + memSpec.tWPRE);
    tRDWR_R = ((((memSpec.tRL + tBURST) + memSpec.tRTRS) - memSpec.tWL) + memSpec.tWPRE);
    tWRRD_S = (((memSpec.tWL + tBURST) + memSpec.tWTR_S) - memSpec.tAL);
    tWRRD_L = (((memSpec.tWL + tBURST) + memSpec.tWTR_L) - memSpec.tAL);
    tWRRD_R = ((((memSpec.tWL + tBURST) + memSpec.tRTRS) - memSpec.tRL) + memSpec.tRPRE);
    tRDAACT = ((memSpec.tAL + memSpec.tRTP) + memSpec.tRP);
    tWRPRE = ((memSpec.tWL + tBURST) + memSpec.tWR);
    tWRAACT = (tWRPRE + memSpec.tRP);
    tRDPDEN = ((memSpec.tRL + tBURST) + memSpec.tCK);
    tWRPDEN = ((memSpec.tWL + tBURST) + memSpec.tWR);
    tWRAPDEN = (((memSpec.tWL + tBURST) + memSpec.tWR) + memSpec.tCK);
    
}

sc_time CheckerDDR4::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload) const
{
    Bank bank = ControllerExtension::getBank(payload);
    BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    Rank rank = ControllerExtension::getRank(payload);
    

    sc_time earliestTimeToStart = sc_time_stamp();

    
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][bank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBankGroup[command][bankGroup]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByRank[command][rank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnBus);

    return earliestTimeToStart;
}

void CheckerDDR4::insert(Command command, const tlm_generic_payload& payload)
{
    const Bank bank = ControllerExtension::getBank(payload);
    const BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    const Rank rank = ControllerExtension::getRank(payload);
    

    PRINTDEBUGMESSAGE("CheckerDDR4", "Changing state on bank " + std::to_string(static_cast<std::size_t>(bank))
                      + " command is " + command.toString());
    
    const sc_time& currentTime = sc_time_stamp();
    
    switch (command)
    {
    case Command::RD:
    {
        // Bank (RD,PREPB) (memSpec.tAL + memSpec.tRTP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tAL + memSpec.tRTP);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (RD,RD) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RD][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (RD,RDA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (RD,WR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (RD,MWR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (RD,WRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (RD,MWRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,RD) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,RDA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,WR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,MWR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,WRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RD,MWRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,PREAB) (memSpec.tAL + memSpec.tRTP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tAL + memSpec.tRTP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,PDEA) tRDPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,PDEP) tRDPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,RD) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,RDA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,WR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,MWR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,WRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,MWRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (RD,RD) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RD,RDA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RD,WR) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RD,MWR) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RD,WRA) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RD,MWRA) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][currentRank];
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
        
        // Bank (WR,WR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WR,MWR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WR,WRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WR,MWRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WR,RD) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RD][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WR,RDA) std::max(tWRRD_L, ((tWRPRE - memSpec.tRTP) - memSpec.tAL)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tWRRD_L, ((tWRPRE - memSpec.tRTP) - memSpec.tAL));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,WR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,MWR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,WRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,MWRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,RD) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WR,RDA) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,PDEA) tWRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,WR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,MWR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,WRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,MWRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,RD) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,RDA) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (WR,WR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WR,MWR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WR,WRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WR,MWRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WR,RD) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WR,RDA) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
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
        
        // Bank (MWR,WR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWR,MWR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWR,WRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWR,MWRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWR,RD) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RD][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWR,RDA) std::max(tWRRD_L, ((tWRPRE - memSpec.tRTP) - memSpec.tAL)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tWRRD_L, ((tWRPRE - memSpec.tRTP) - memSpec.tAL));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,WR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,MWR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,WRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,MWRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,RD) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWR,RDA) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,PDEA) tWRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,WR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,MWR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,WRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,MWRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,RD) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,RDA) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (MWR,WR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWR,MWR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWR,WRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWR,MWRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWR,RD) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWR,RDA) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        

        break;
    }
    case Command::RDA:
    {
        // Bank (RDA,ACT) tRDAACT [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDAACT;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,RD) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,RDA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,WR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,MWR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,WRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (RDA,MWRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,PDEA) tRDPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,PDEP) tRDPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,RD) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,RDA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,WR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,MWR) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,WRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,MWRA) tRDWR [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDWR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,PREAB) (memSpec.tAL + memSpec.tRTP) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tAL + memSpec.tRTP);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,REFAB) tRDAACT [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDAACT;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,SREFEN) std::max(tRDPDEN, ((memSpec.tAL + memSpec.tRTP) + memSpec.tRP)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tRDPDEN, ((memSpec.tAL + memSpec.tRTP) + memSpec.tRP));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (RDA,WR) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RDA,MWR) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RDA,WRA) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RDA,MWRA) tRDWR_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tRDWR_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RDA,RD) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (RDA,RDA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        

        break;
    }
    case Command::WRA:
    {
        // Bank (WRA,ACT) tWRAACT [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAACT;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,WR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,MWR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,WRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,MWRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,RD) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (WRA,RDA) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,PDEA) tWRAPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,PDEP) tWRAPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,WR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,MWR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,WRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,MWRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,RD) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,RDA) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
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
        
        // Rank (WRA,SREFEN) std::max(tWRAPDEN, (tWRPRE + memSpec.tRP)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tWRAPDEN, (tWRPRE + memSpec.tRP));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (WRA,WR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WRA,MWR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WRA,WRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WRA,MWRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WRA,RD) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (WRA,RDA) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        

        break;
    }
    case Command::MWRA:
    {
        // BankGroup (MWRA,WR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,MWR) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWR][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,WRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::WRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,MWRA) memSpec.tCCD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::MWRA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,RD) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RD][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (MWRA,RDA) tWRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::RDA][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,PDEA) tWRAPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,PDEP) tWRAPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,WR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,MWR) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,WRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,MWRA) memSpec.tCCD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,RD) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,RDA) tWRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Channel (MWRA,WR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWRA,MWR) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWRA,WRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWRA,MWRA) (tBURST + memSpec.tRTRS) [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + (tBURST + memSpec.tRTRS);
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWRA,RD) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Channel (MWRA,RDA) tWRRD_R [] Different(level=<ComponentLevel.Rank: 3>)
        {
            const sc_time constraint = currentTime + tWRRD_R;
            for (unsigned int i = memSpec.ranksPerChannel * static_cast<unsigned>(0); i < memSpec.ranksPerChannel * (1 + static_cast<unsigned>(0)); i++)
            {
                Rank currentRank{i};
                
                if (currentRank == rank)
                    continue;
                
                sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][currentRank];
                earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        

        break;
    }
    case Command::ACT:
    {
        // Bank (ACT,PREPB) memSpec.tRAS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRAS;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,PREAB) memSpec.tRAS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRAS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,RD) (memSpec.tRCD - memSpec.tAL) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCD - memSpec.tAL);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RD][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WR) (memSpec.tRCD - memSpec.tAL) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCD - memSpec.tAL);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,MWR) (memSpec.tRCD - memSpec.tAL) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCD - memSpec.tAL);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,RDA) (memSpec.tRCD - memSpec.tAL) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCD - memSpec.tAL);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WRA) (memSpec.tRCD - memSpec.tAL) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCD - memSpec.tAL);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,MWRA) (memSpec.tRCD - memSpec.tAL) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCD - memSpec.tAL);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,ACT) memSpec.tRCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,ACT) memSpec.tRRD_S [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRD_S;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // BankGroup (ACT,ACT) memSpec.tRRD_L [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRD_L;
            sc_time &earliestTimeToStart = nextCommandByBankGroup[Command::ACT][bankGroup];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,PDEA) memSpec.tACTPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tACTPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFAB) memSpec.tRC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,SREFEN) memSpec.tRC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
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

            last4ActivatesOnRank[rank].pop();
        }

        break;
    }
    case Command::PREPB:
    {
        // Bank (PREPB,ACT) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,REFAB) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,PDEA) memSpec.tPRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,PDEP) memSpec.tPRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
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
    case Command::PREAB:
    {
        // Rank (PREAB,ACT) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,REFAB) memSpec.tRP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,PDEP) memSpec.tPRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tPRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
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
        // Rank (REFAB,ACT) memSpec.tRFC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFAB) memSpec.tRFC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,SREFEN) memSpec.tRFC [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFC;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,PDEP) memSpec.tREFPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tREFPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
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
        
        // Rank (PDXA,ACT) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
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
        
        // Rank (PDXA,MWR) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,WRA) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,MWRA) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
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
        
        // Rank (PDXP,SREFEN) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXP,ACT) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::SREFEX:
    {
        // Rank (SREFEX,ACT) memSpec.tXS [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXS;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
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
        
        // Rank (SREFEX,RD) memSpec.tXSDLL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSDLL;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,RDA) memSpec.tXSDLL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSDLL;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,WR) memSpec.tXSDLL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSDLL;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,MWR) memSpec.tXSDLL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSDLL;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,WRA) memSpec.tXSDLL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSDLL;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,MWRA) memSpec.tXSDLL [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSDLL;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
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
    nextCommandOnBus = std::max(nextCommandOnBus, currentTime + memSpec.getCommandLength(command));
}

} // namespace DRAMSys