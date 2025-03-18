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

#include "CheckerLPDDR4.h"
#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerLPDDR4::CheckerLPDDR4(const MemSpecLPDDR4& memSpec) : memSpec(memSpec)
{
    
    nextCommandByBank.fill({BankVector<sc_time>(memSpec.banksPerChannel, SC_ZERO_TIME)});
    nextCommandByRank.fill({RankVector<sc_time>(memSpec.ranksPerChannel, SC_ZERO_TIME)});
    last4ActivatesOnRank = RankVector<std::queue<sc_time>>(memSpec.ranksPerChannel);

    tBURST = ((memSpec.defaultBurstLength / memSpec.dataRate) * memSpec.tCK);
    tRDWR = (((((memSpec.tRL + memSpec.tDQSCK) + tBURST) - memSpec.tWL) + memSpec.tWPRE) + memSpec.tRPST);
    tRDWR_R = (((memSpec.tRL + tBURST) + memSpec.tRTRS) - memSpec.tWL);
    tWRRD = (((memSpec.tWL + memSpec.tCK) + tBURST) + memSpec.tWTR);
    tWRRD_R = (((memSpec.tWL + tBURST) + memSpec.tRTRS) - memSpec.tRL);
    tRDPRE = ((memSpec.tRTP + tBURST) - (memSpec.tCK * 6));
    tRDAACT = (((memSpec.tRTP + memSpec.tRPpb) + tBURST) - (memSpec.tCK * 8));
    tWRPRE = (((((memSpec.tCK * 2) + memSpec.tWL) + memSpec.tCK) + tBURST) + memSpec.tWR);
    tWRAACT = ((((memSpec.tWL + tBURST) + memSpec.tWR) + memSpec.tCK) + memSpec.tRPpb);
    tACTPDEN = ((memSpec.tCK * 3) + memSpec.tCMDCKE);
    tPRPDEN = (memSpec.tCK + memSpec.tCMDCKE);
    tRDPDEN = (((((memSpec.tCK * 3) + memSpec.tRL) + memSpec.tDQSCK) + tBURST) + memSpec.tRPST);
    tWRPDEN = (((((memSpec.tCK * 3) + memSpec.tWL) + ((std::ceil((memSpec.tDQSS / memSpec.tCK)) + std::ceil((memSpec.tDQS2DQ / memSpec.tCK))) * memSpec.tCK)) + tBURST) + memSpec.tWR);
    tWRAPDEN = ((((((memSpec.tCK * 3) + memSpec.tWL) + ((std::ceil((memSpec.tDQSS / memSpec.tCK)) + std::ceil((memSpec.tDQS2DQ / memSpec.tCK))) * memSpec.tCK)) + tBURST) + memSpec.tWR) + (memSpec.tCK * 2));
    tREFPDEN = (memSpec.tCK + memSpec.tCMDCKE);
    
}

sc_time CheckerLPDDR4::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload) const
{
    Bank bank = ControllerExtension::getBank(payload);
    Rank rank = ControllerExtension::getRank(payload);
    

    sc_time earliestTimeToStart = sc_time_stamp();

    
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByBank[command][bank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandByRank[command][rank]);
    earliestTimeToStart = std::max(earliestTimeToStart, nextCommandOnBus);

    return earliestTimeToStart;
}

void CheckerLPDDR4::insert(Command command, const tlm_generic_payload& payload)
{
    const Bank bank = ControllerExtension::getBank(payload);
    const Rank rank = ControllerExtension::getRank(payload);
    const unsigned burstLength = ControllerExtension::getBurstLength(payload);

    PRINTDEBUGMESSAGE("CheckerLPDDR4", "Changing state on bank " + std::to_string(static_cast<std::size_t>(bank))
                      + " command is " + command.toString());
    
    const sc_time& currentTime = sc_time_stamp();
    
    switch (command)
    {
    case Command::RD:
    {
        // Bank (RD,PREPB) (memSpec.tRRD + (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRRD + (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,PREAB) tRDPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPRE;
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
        
        // Rank (RD,RD) memSpec.tCCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RD,RDA) memSpec.tCCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD;
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
        
        // Bank (WR,MWR) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WR,MWRA) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WR,MWR) (memSpec.tCCDMW + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCDMW + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WR,MWRA) (memSpec.tCCDMW + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCDMW + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WR,RDA) std::max(tWRRD, (tWRPRE - tRDPRE)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tWRRD, (tWRPRE - tRDPRE));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
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
        
        // Rank (WR,WR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,MWR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,WRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,MWRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,WR) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,MWR) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,WRA) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,MWRA) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WR,RD) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WR,RDA) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
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
        
        // Bank (MWR,MWR) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (MWR,MWRA) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (MWR,RDA) std::max(tWRRD, (tWRPRE - tRDPRE)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tWRRD, (tWRPRE - tRDPRE));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
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
        
        // Rank (MWR,WR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWR,MWR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWR,WRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWR,MWRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWR,RD) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWR,RDA) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
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
        
        // Bank (RDA,REFPB) (tRDPRE + memSpec.tRPpb) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tRDPRE + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,PREAB) tRDPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tRDPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
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
        
        // Rank (RDA,RD) memSpec.tCCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,RDA) memSpec.tCCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCCD;
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
        
        // Rank (RDA,REFAB) (tRDPRE + memSpec.tRPpb) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tRDPRE + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (RDA,SREFEN) std::max(tRDPDEN, (tRDPRE + memSpec.tRPpb)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tRDPDEN, (tRDPRE + memSpec.tRPpb));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
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
        

        break;
    }
    case Command::WRA:
    {
        // Bank (WRA,MWR) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WRA,MWRA) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WRA,MWR) (memSpec.tCCDMW + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCDMW + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WRA,MWRA) (memSpec.tCCDMW + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCDMW + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (WRA,ACT) tWRAACT [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAACT;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (WRA,REFPB) (tWRPRE + memSpec.tRPpb) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
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
        
        // Rank (WRA,WR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,MWR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,WRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,MWRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,WR) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,MWR) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,WRA) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,MWRA) (memSpec.tCCD + (memSpec.tCK * 8)) [LastBurstLength(burst_length=32, inversed=False)] SameComponent()
        {
            if (burstLength == 32)
            {
            const sc_time constraint = currentTime + (memSpec.tCCD + (memSpec.tCK * 8));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (WRA,RD) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,RDA) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,REFAB) (tWRPRE + memSpec.tRPpb) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (WRA,SREFEN) std::max(tWRAPDEN, (tWRPRE + memSpec.tRPpb)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tWRAPDEN, (tWRPRE + memSpec.tRPpb));
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
        // Bank (MWRA,MWR) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (MWRA,MWRA) memSpec.tCCDMW [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCDMW;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Bank (MWRA,ACT) tWRAACT [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRAACT;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (MWRA,REFPB) (tWRPRE + memSpec.tRPpb) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,PREAB) tWRPRE [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRPRE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
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
        
        // Rank (MWRA,WR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWRA,MWR) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWR][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWRA,WRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWRA,MWRA) memSpec.tCCD [LastBurstLength(burst_length=32, inversed=True)] SameComponent()
        {
            if (burstLength != 32)
            {
            const sc_time constraint = currentTime + memSpec.tCCD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::MWRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
            }
        }
        
        // Rank (MWRA,RD) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RD][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,RDA) tWRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + tWRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::RDA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,REFAB) (tWRPRE + memSpec.tRPpb) [] SameComponent()
        {
            const sc_time constraint = currentTime + (tWRPRE + memSpec.tRPpb);
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (MWRA,SREFEN) std::max(tWRAPDEN, (tWRPRE + memSpec.tRPpb)) [] SameComponent()
        {
            const sc_time constraint = currentTime + std::max(tWRAPDEN, (tWRPRE + memSpec.tRPpb));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
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
        // Bank (ACT,PREPB) (memSpec.tRAS + (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRAS + (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::PREPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,RD) memSpec.tRCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RD][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WR) memSpec.tRCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,MWR) memSpec.tRCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWR][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,RDA) memSpec.tRCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::RDA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,WRA) memSpec.tRCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::WRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,MWRA) memSpec.tRCD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCD;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::MWRA][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,ACT) memSpec.tRCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRCpb;
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Bank (ACT,REFPB) (memSpec.tRCpb + (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCpb + (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::REFPB][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,PREAB) (memSpec.tRAS + (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRAS + (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,ACT) memSpec.tRRD [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRRD;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFAB) (memSpec.tRCpb + (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCpb + (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,SREFEN) (memSpec.tRCpb + (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRCpb + (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,REFPB) (memSpec.tRRD + (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRRD + (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (ACT,PDEA) tACTPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tACTPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
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
        // Bank (PREPB,ACT) (memSpec.tRPpb - (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRPpb - (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
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
        
        // Rank (PREPB,SREFEN) memSpec.tRPpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,PDEA) tPRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tPRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREPB,PDEP) tPRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tPRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::REFPB:
    {
        // Bank (REFPB,ACT) (memSpec.tRFCpb - (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRFCpb - (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByBank[Command::ACT][bank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,REFAB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,REFPB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,ACT) (memSpec.tRRD - (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRRD - (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,PREAB) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PREAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,SREFEN) memSpec.tRFCpb [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCpb;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,PDEA) tREFPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tREFPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFPB,PDEP) tREFPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tREFPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
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
    case Command::PREAB:
    {
        // Rank (PREAB,ACT) (memSpec.tRPab - (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRPab - (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,REFAB) memSpec.tRPab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,SREFEN) memSpec.tRPab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,REFPB) memSpec.tRPab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRPab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PREAB,PDEP) tPRPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tPRPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::REFAB:
    {
        // Rank (REFAB,ACT) (memSpec.tRFCab - (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tRFCab - (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,PDEP) tREFPDEN [] SameComponent()
        {
            const sc_time constraint = currentTime + tREFPDEN;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFAB) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,REFPB) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (REFAB,SREFEN) memSpec.tRFCab [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tRFCab;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PDEA:
    {
        // Rank (PDEA,PDXA) memSpec.tCKE [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCKE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDXA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::PDEP:
    {
        // Rank (PDEP,PDXP) memSpec.tCKE [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tCKE;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDXP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    case Command::SREFEN:
    {
        // Rank (SREFEN,SREFEX) memSpec.tSR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tSR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEX][rank];
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
        
        // Rank (PDXA,WRA) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::WRA][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (PDXA,REFPB) memSpec.tXP [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXP;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
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
        // Rank (SREFEX,ACT) (memSpec.tXSR - (memSpec.tCK * 2)) [] SameComponent()
        {
            const sc_time constraint = currentTime + (memSpec.tXSR - (memSpec.tCK * 2));
            sc_time &earliestTimeToStart = nextCommandByRank[Command::ACT][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,REFAB) memSpec.tXSR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFAB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,REFPB) memSpec.tXSR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::REFPB][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,PDEP) memSpec.tXSR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::PDEP][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        
        // Rank (SREFEX,SREFEN) memSpec.tXSR [] SameComponent()
        {
            const sc_time constraint = currentTime + memSpec.tXSR;
            sc_time &earliestTimeToStart = nextCommandByRank[Command::SREFEN][rank];
            earliestTimeToStart = std::max(earliestTimeToStart, constraint);
        }
        

        break;
    }
    
    }
    nextCommandOnBus = std::max(nextCommandOnBus, currentTime + memSpec.getCommandLength(command));
}

} // namespace DRAMSys