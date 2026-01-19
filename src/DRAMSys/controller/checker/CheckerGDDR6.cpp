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

#include "CheckerGDDR6.h"

#include "DRAMSys/common/DebugManager.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerGDDR6::CheckerGDDR6(const MemSpecGDDR6& memSpec) :
    memSpec(memSpec)
{
    lastScheduledByCommandAndBank = std::vector<ControllerVector<Bank, sc_time>>(
        Command::numberOfCommands(),
        ControllerVector<Bank, sc_time>(memSpec.banksPerChannel, scMaxTime));
    lastScheduledByCommandAndBankGroup = std::vector<ControllerVector<BankGroup, sc_time>>(
        Command::numberOfCommands(),
        ControllerVector<BankGroup, sc_time>(memSpec.bankGroupsPerChannel, scMaxTime));
    lastScheduledByCommandAndRank = std::vector<ControllerVector<Rank, sc_time>>(
        Command::numberOfCommands(),
        ControllerVector<Rank, sc_time>(memSpec.ranksPerChannel, scMaxTime));
    lastScheduledByCommand = std::vector<sc_time>(Command::numberOfCommands(), scMaxTime);
    lastCommandOnBus = scMaxTime;
    last4Activates = ControllerVector<Rank, std::queue<sc_time>>(memSpec.ranksPerChannel);

    bankwiseRefreshCounter = ControllerVector<Rank, unsigned>(memSpec.ranksPerChannel);

    tBURST = memSpec.defaultBurstLength / memSpec.dataRate * memSpec.tCK;
    tRDSRE = memSpec.tRL + memSpec.tWCK2CKPIN + memSpec.tWCK2CK + memSpec.tWCK2DQO + tBURST;
    tWRSRE = memSpec.tWL + memSpec.tWCK2CKPIN + memSpec.tWCK2CK + memSpec.tWCK2DQI + tBURST;
    tRDWR_R = memSpec.tRL + tBURST + memSpec.tRTRS - memSpec.tWL;
    tWRRD_R = memSpec.tWL + tBURST + memSpec.tRTRS - memSpec.tRL;
    tWRRD_S = memSpec.tWL + tBURST + memSpec.tWTRS;
    tWRRD_L = memSpec.tWL + tBURST + memSpec.tWTRL;
    tWRPRE = memSpec.tWL + tBURST + memSpec.tWR;
}

sc_time CheckerGDDR6::timeToSatisfyConstraints(Command command,
                                               const tlm_generic_payload& payload) const
{
    Rank rank = ControllerExtension::getRank(payload);
    BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    Bank bank = ControllerExtension::getBank(payload);

    sc_time lastCommandStart;
    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::RD || command == Command::RDA)
    {
        assert(ControllerExtension::getBurstLength(payload) == 16);

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCDRD);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RD][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        lastCommandStart =
            lastScheduledByCommand[Command::RD] != lastScheduledByCommandAndRank[Command::RD][rank]
                ? lastScheduledByCommand[Command::RD]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRTRS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RDA][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::RDA] !=
                                   lastScheduledByCommandAndRank[Command::RDA][rank]
                               ? lastScheduledByCommand[Command::RDA]
                               : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRTRS);

        if (command == Command::RDA)
        {
            lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank];
            if (lastCommandStart != scMaxTime)
                earliestTimeToStart =
                    std::max(earliestTimeToStart, lastCommandStart + tWRPRE - memSpec.tRTP);
        }

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_L);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_S);

        lastCommandStart =
            lastScheduledByCommand[Command::WR] != lastScheduledByCommandAndRank[Command::WR][rank]
                ? lastScheduledByCommand[Command::WR]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_R);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_L);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_S);

        lastCommandStart = lastScheduledByCommand[Command::WRA] !=
                                   lastScheduledByCommandAndRank[Command::WRA][rank]
                               ? lastScheduledByCommand[Command::WRA]
                               : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tLK);
    }
    else if (command == Command::WR || command == Command::WRA)
    {
        assert(ControllerExtension::getBurstLength(payload) == 16);

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCDWR);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTW);

        lastCommandStart =
            lastScheduledByCommand[Command::RD] != lastScheduledByCommandAndRank[Command::RD][rank]
                ? lastScheduledByCommand[Command::RD]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTW);

        lastCommandStart = lastScheduledByCommand[Command::RDA] !=
                                   lastScheduledByCommandAndRank[Command::RDA][rank]
                               ? lastScheduledByCommand[Command::RDA]
                               : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR_R);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        lastCommandStart =
            lastScheduledByCommand[Command::WR] != lastScheduledByCommandAndRank[Command::WR][rank]
                ? lastScheduledByCommand[Command::WR]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRTRS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::WRA] !=
                                   lastScheduledByCommandAndRank[Command::WRA][rank]
                               ? lastScheduledByCommand[Command::WRA]
                               : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRTRS);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tLK);
    }
    else if (command == Command::ACT)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::ACT][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRDS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTP + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PREPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCab);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRREFD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);

        if (last4Activates[rank].size() >= 4)
            earliestTimeToStart =
                std::max(earliestTimeToStart, last4Activates[rank].front() + memSpec.tFAW);
    }
    else if (command == Command::PREPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRAS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RD][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tPPD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
    }
    else if (command == Command::PREAB)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRAS);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tPPD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCpb);
    }
    else if (command == Command::REFAB)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTP + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCab);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);
    }
    else if (command == Command::REFPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::ACT][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRDS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTP + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PREPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCab);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
        {
            if (bankwiseRefreshCounter[rank] == 0)
                earliestTimeToStart =
                    std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCpb);
            else
                earliestTimeToStart =
                    std::max(earliestTimeToStart, lastCommandStart + memSpec.tRREFD);
        }

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);

        if (last4Activates[rank].size() >= 4)
            earliestTimeToStart =
                std::max(earliestTimeToStart, last4Activates[rank].front() + memSpec.tFAW);
    }
    else if (command == Command::PDEA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tACTPDE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tPREPDE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tREFPDE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
    }
    else if (command == Command::PDXA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::PDEA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tPD);
    }
    else if (command == Command::PDEP)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tPREPDE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tPREPDE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tREFPDE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tREFPDE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);
    }
    else if (command == Command::PDXP)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::PDEP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tPD);
    }
    else if (command == Command::SREFEN)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart,
                         lastCommandStart + std::max(memSpec.tRTP + memSpec.tRP, tRDSRE));

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCab);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);
    }
    else if (command == Command::SREFEX)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEN][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKESR);
    }
    else
        SC_REPORT_FATAL("CheckerGDDR6", "Unknown command!");

    // Check if command bus is free
    if (lastCommandOnBus != scMaxTime)
        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnBus + memSpec.tCK);

    return earliestTimeToStart;
}

void CheckerGDDR6::insert(Command command, const tlm_generic_payload& payload)
{
    Rank rank = ControllerExtension::getRank(payload);
    BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    Bank bank = ControllerExtension::getBank(payload);

    PRINTDEBUGMESSAGE("CheckerGDDR6",
                      "Changing state on bank " + std::to_string(static_cast<std::size_t>(bank)) +
                          " command is " + command.toString());

    lastScheduledByCommandAndBank[command][bank] = sc_time_stamp();
    lastScheduledByCommandAndBankGroup[command][bankGroup] = sc_time_stamp();
    lastScheduledByCommandAndRank[command][rank] = sc_time_stamp();
    lastScheduledByCommand[command] = sc_time_stamp();
    lastCommandOnBus = sc_time_stamp();

    if (command == Command::ACT || command == Command::REFPB)
    {
        if (last4Activates[rank].size() == 4)
            last4Activates[rank].pop();
        last4Activates[rank].push(lastCommandOnBus);
    }

    if (command == Command::REFPB)
        bankwiseRefreshCounter[rank] = (bankwiseRefreshCounter[rank] + 1) % memSpec.banksPerRank;
}

} // namespace DRAMSys
