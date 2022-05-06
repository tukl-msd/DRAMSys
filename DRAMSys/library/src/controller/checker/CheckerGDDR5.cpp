/*
 * Copyright (c) 2019, Technische Universität Kaiserslautern
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

#include <algorithm>

#include "CheckerGDDR5.h"

using namespace sc_core;
using namespace tlm;

CheckerGDDR5::CheckerGDDR5(const Configuration& config)
{
    memSpec = dynamic_cast<const MemSpecGDDR5 *>(config.memSpec.get());
    if (memSpec == nullptr)
        SC_REPORT_FATAL("CheckerGDDR5", "Wrong MemSpec chosen");

    lastScheduledByCommandAndBank = std::vector<std::vector<sc_time>>
            (Command::numberOfCommands(), std::vector<sc_time>(memSpec->banksPerChannel, sc_max_time()));
    lastScheduledByCommandAndBankGroup = std::vector<std::vector<sc_time>>
            (Command::numberOfCommands(), std::vector<sc_time>(memSpec->bankGroupsPerChannel, sc_max_time()));
    lastScheduledByCommandAndRank = std::vector<std::vector<sc_time>>
            (Command::numberOfCommands(), std::vector<sc_time>(memSpec->ranksPerChannel, sc_max_time()));
    lastScheduledByCommand = std::vector<sc_time>(Command::numberOfCommands(), sc_max_time());
    lastCommandOnBus = sc_max_time();
    last4Activates = std::vector<std::queue<sc_time>>(memSpec->ranksPerChannel);
    last32Activates = std::vector<std::queue<sc_time>>(memSpec->ranksPerChannel);

    bankwiseRefreshCounter = std::vector<unsigned>(memSpec->ranksPerChannel);

    tBURST = memSpec->defaultBurstLength / memSpec->dataRate * memSpec->tCK;
    tRDSRE = memSpec->tCL + memSpec->tWCK2CKPIN + memSpec->tWCK2CK + memSpec->tWCK2DQO + tBURST;
    tWRSRE = memSpec->tWL + memSpec->tWCK2CKPIN + memSpec->tWCK2CK + memSpec->tWCK2DQI + tBURST;
    tRDWR_R = memSpec->tCL + tBURST + memSpec->tRTRS - memSpec->tWL;
    tWRRD_R = memSpec->tWL + tBURST + memSpec->tRTRS - memSpec->tCL;
    tWRRD_S = memSpec->tWL + tBURST + memSpec->tWTRS;
    tWRRD_L = memSpec->tWL + tBURST + memSpec->tWTRL;
    tWRPRE = memSpec->tWL + tBURST + memSpec->tWR;
}

sc_time CheckerGDDR5::timeToSatisfyConstraints(Command command, const tlm_generic_payload& payload) const
{
    Rank rank = DramExtension::getRank(payload);
    BankGroup bankGroup = DramExtension::getBankGroup(payload);
    Bank bank = DramExtension::getBank(payload);
    
    sc_time lastCommandStart;
    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::RD || command == Command::RDA)
    {
        assert(DramExtension::getBurstLength(payload) == 8);

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRCDRD);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RD][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::RD] != lastScheduledByCommandAndRank[Command::RD][rank.ID()] ? lastScheduledByCommand[Command::RD] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec->tRTRS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RDA][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::RDA] != lastScheduledByCommandAndRank[Command::RDA][rank.ID()] ? lastScheduledByCommand[Command::RDA] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec->tRTRS);

        if (command == Command::RDA)
        {
            lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank.ID()];
            if (lastCommandStart != sc_max_time())
                earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE - memSpec->tRTP);
        }

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_L);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_S);

        lastCommandStart = lastScheduledByCommand[Command::WR] != lastScheduledByCommandAndRank[Command::WR][rank.ID()] ? lastScheduledByCommand[Command::WR] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_R);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_L);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_S);

        lastCommandStart = lastScheduledByCommand[Command::WRA] != lastScheduledByCommandAndRank[Command::WRA][rank.ID()] ? lastScheduledByCommand[Command::WRA] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tLK);
    }
    else if (command == Command::WR || command == Command::WRA)
    {
        assert(DramExtension::getBurstLength(payload) == 8);

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRCDWR);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTW);

        lastCommandStart = lastScheduledByCommand[Command::RD] != lastScheduledByCommandAndRank[Command::RD][rank.ID()] ? lastScheduledByCommand[Command::RD] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTW);

        lastCommandStart = lastScheduledByCommand[Command::RDA] != lastScheduledByCommandAndRank[Command::RDA][rank.ID()] ? lastScheduledByCommand[Command::RDA] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR_R);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::WR] != lastScheduledByCommandAndRank[Command::WR][rank.ID()] ? lastScheduledByCommand[Command::WR] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec->tRTRS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::WRA] != lastScheduledByCommandAndRank[Command::WRA][rank.ID()] ? lastScheduledByCommand[Command::WRA] : sc_max_time();
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec->tRTRS);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tLK);
    }
    else if (command == Command::ACT)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::ACT][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PREPB][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFPB][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCPB);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRREFD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);

        if (last4Activates[rank.ID()].size() >= 4)
            earliestTimeToStart = std::max(earliestTimeToStart, last4Activates[rank.ID()].front() + memSpec->tFAW);

        if (last32Activates[rank.ID()].size() >= 32)
            earliestTimeToStart = std::max(earliestTimeToStart, last32Activates[rank.ID()].front() + memSpec->t32AW);
    }
    else if (command == Command::PREPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRAS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RD][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tPPD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);
    }
    else if (command == Command::PREAB)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRAS);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tPPD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCPB);
    }
    else if (command == Command::REFAB)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCPB);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);
    }
    else if (command == Command::REFPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::ACT][bankGroup.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PREPB][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFPB][bank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCPB);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
        {
            if (bankwiseRefreshCounter[rank.ID()] == 0)
                earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCPB);
            else
                earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRREFD);
        }

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);

        if (last4Activates[rank.ID()].size() >= 4)
            earliestTimeToStart = std::max(earliestTimeToStart, last4Activates[rank.ID()].front() + memSpec->tFAW);

        if (last32Activates[rank.ID()].size() >= 32)
            earliestTimeToStart = std::max(earliestTimeToStart, last32Activates[rank.ID()].front() + memSpec->t32AW);
    }
    else if (command == Command::PDEA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);
    }
    else if (command == Command::PDXA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::PDEA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tPD);
    }
    else if (command == Command::PDEP)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);
    }
    else if (command == Command::PDXP)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::PDEP][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tPD);
    }
    else if (command == Command::SREFEN)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDSRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + std::max(memSpec->tRTP + memSpec->tRP, tRDSRE));

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXPN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCPB);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);
    }
    else if (command == Command::SREFEX)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEN][rank.ID()];
        if (lastCommandStart != sc_max_time())
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCKE);
    }
    else
        SC_REPORT_FATAL("CheckerGDDR5", "Unknown command!");

    // Check if command bus is free
    if (lastCommandOnBus != sc_max_time())
        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnBus + memSpec->tCK);

    return earliestTimeToStart;
}

void CheckerGDDR5::insert(Command command, const tlm_generic_payload& payload)
{
    Rank rank = DramExtension::getRank(payload);
    BankGroup bankGroup = DramExtension::getBankGroup(payload);
    Bank bank = DramExtension::getBank(payload);

    PRINTDEBUGMESSAGE("CheckerGDDR5", "Changing state on bank " + std::to_string(bank.ID())
                      + " command is " + command.toString());

    lastScheduledByCommandAndBank[command][bank.ID()] = sc_time_stamp();
    lastScheduledByCommandAndBankGroup[command][bankGroup.ID()] = sc_time_stamp();
    lastScheduledByCommandAndRank[command][rank.ID()] = sc_time_stamp();
    lastScheduledByCommand[command] = sc_time_stamp();
    lastCommandOnBus = sc_time_stamp();

    if (command == Command::ACT || command == Command::REFPB)
    {
        if (last4Activates[rank.ID()].size() == 4)
            last4Activates[rank.ID()].pop();
        last4Activates[rank.ID()].push(lastCommandOnBus);

        if (last32Activates[rank.ID()].size() == 32)
            last32Activates[rank.ID()].pop();
        last32Activates[rank.ID()].push(lastCommandOnBus);
    }

    if (command == Command::REFPB)
        bankwiseRefreshCounter[rank.ID()] = (bankwiseRefreshCounter[rank.ID()] + 1) % memSpec->banksPerRank;
}
