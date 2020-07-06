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

#include "CheckerHBM2.h"

CheckerHBM2::CheckerHBM2()
{
    Configuration &config = Configuration::getInstance();
    memSpec = dynamic_cast<MemSpecHBM2 *>(config.memSpec);
    if (memSpec == nullptr)
        SC_REPORT_FATAL("CheckerHBM2", "Wrong MemSpec chosen");

    lastScheduledByCommandAndBank = std::vector<std::vector<sc_time>>
            (numberOfCommands(), std::vector<sc_time>(memSpec->numberOfBanks));
    lastScheduledByCommandAndBankGroup = std::vector<std::vector<sc_time>>
            (numberOfCommands(), std::vector<sc_time>(memSpec->numberOfBankGroups));
    lastScheduledByCommandAndRank = std::vector<std::vector<sc_time>>
            (numberOfCommands(), std::vector<sc_time>(memSpec->numberOfRanks));
    lastScheduledByCommand = std::vector<sc_time>(numberOfCommands());

    last4Activates = std::vector<std::queue<sc_time>>(memSpec->numberOfRanks);
    bankwiseRefreshCounter = std::vector<unsigned>(memSpec->numberOfRanks);

    tBURST = memSpec->burstLength / memSpec->dataRate * memSpec->tCK;
    tRDPDE = memSpec->tRL + memSpec->tPL + tBURST + memSpec->tCK;
    tRDSRE = tRDPDE;
    tWRPRE = memSpec->tWL + tBURST + memSpec->tWR;
    tWRPDE = memSpec->tWL + memSpec->tPL + tBURST + memSpec->tCK + memSpec->tWR;
    tWRAPDE = memSpec->tWL + memSpec->tPL + tBURST + memSpec->tCK + memSpec->tWR;
    tWRRDS = memSpec->tWL + tBURST + memSpec->tWTRS;
    tWRRDL = memSpec->tWL + tBURST + memSpec->tWTRL;
}

sc_time CheckerHBM2::timeToSatisfyConstraints(Command command, Rank rank, BankGroup bankgroup, Bank bank) const
{
    sc_time lastCommandStart;
    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::RD || command == Command::RDA)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRCDRD + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RD][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RDA][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        if (command == Command::RDA)
        {
            lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank.ID()];
            if (lastCommandStart != SC_ZERO_TIME)
                earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE - memSpec->tRTP);
        }

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDS);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnCasBus + memSpec->tCK);
    }
    else if (command == Command::WR || command == Command::WRA)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRCDWR + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTW);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTW);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnCasBus + memSpec->tCK);
    }
    else if (command == Command::ACT)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::ACT][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP + memSpec->tRP - memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP - memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PRE][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP - memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP - memSpec->tCK);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP - memSpec->tCK);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP - memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC - memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFB][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCSB - memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFB][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRREFD - memSpec->tCK);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS - memSpec->tCK);

        if (last4Activates[rank.ID()].size() >= 4)
            earliestTimeToStart = std::max(earliestTimeToStart, last4Activates[rank.ID()].front() + memSpec->tFAW -  memSpec->tCK);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::PRE)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRAS + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RD][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::PREA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRAS + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFB][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCSB);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::REFA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PRE][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFB][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCSB);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::REFB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::ACT][bankgroup.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDL + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRRDS + memSpec->tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRTP + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PRE][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFA][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFB][bank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCSB);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFB][rank.ID()];
        if (lastCommandStart != SC_ZERO_TIME)
        {
            if (bankwiseRefreshCounter[rank.ID()] == 0)
                earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCSB);
            else
                earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRREFD);
        }

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);

        if (last4Activates[rank.ID()].size() >= 4)
            earliestTimeToStart = std::max(earliestTimeToStart, last4Activates[rank.ID()].front() + memSpec->tFAW);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::PDEA)
    {
        lastCommandStart = lastScheduledByCommand[Command::RD];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::RDA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::WR];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPDE);

        lastCommandStart = lastScheduledByCommand[Command::WRA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRAPDE);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCKE);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::PDXA)
    {
        lastCommandStart = lastScheduledByCommand[Command::PDEA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tPD);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::PDEP)
    {
        lastCommandStart = lastScheduledByCommand[Command::RD];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::RDA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::WRA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRAPDE);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCKE);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::PDXP)
    {
        lastCommandStart = lastScheduledByCommand[Command::PDEP];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tPD);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::SREFEN)
    {
        lastCommandStart = lastScheduledByCommand[Command::ACT];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRC + memSpec->tCK);

        lastCommandStart = lastScheduledByCommand[Command::RDA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + std::max(memSpec->tRTP + memSpec->tRP, tRDSRE));

        lastCommandStart = lastScheduledByCommand[Command::WRA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec->tRP);

        lastCommandStart = lastScheduledByCommand[Command::PRE];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommand[Command::PREA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRP);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXP);

        lastCommandStart = lastScheduledByCommand[Command::REFA];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFC);

        lastCommandStart = lastScheduledByCommand[Command::REFB];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tRFCSB);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tXS);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else if (command == Command::SREFEX)
    {
        lastCommandStart = lastScheduledByCommand[Command::SREFEN];
        if (lastCommandStart != SC_ZERO_TIME)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec->tCKESR);

        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec->tCK);
    }
    else
        SC_REPORT_FATAL("CheckerHBM2", "Unknown command!");

    return earliestTimeToStart;
}

void CheckerHBM2::insert(Command command, Rank rank, BankGroup bankgroup, Bank bank)
{
    PRINTDEBUGMESSAGE("CheckerHBM2", "Changing state on bank " + std::to_string(bank.ID())
                      + " command is " + commandToString(command));

    lastScheduledByCommandAndBank[command][bank.ID()] = sc_time_stamp();
    lastScheduledByCommandAndBankGroup[command][bankgroup.ID()] = sc_time_stamp();
    lastScheduledByCommandAndRank[command][rank.ID()] = sc_time_stamp();
    lastScheduledByCommand[command] = sc_time_stamp();

    if (isCasCommand(command))
        lastCommandOnCasBus = sc_time_stamp();
    else if (command == Command::ACT)
        lastCommandOnRasBus = sc_time_stamp() + memSpec->tCK;
    else
        lastCommandOnRasBus = sc_time_stamp();

    if (command == Command::ACT || command == Command::REFB)
    {
        if (last4Activates[rank.ID()].size() == 4)
            last4Activates[rank.ID()].pop();
        last4Activates[rank.ID()].push(lastCommandOnRasBus);
    }

    if (command == Command::REFB)
        bankwiseRefreshCounter[rank.ID()] = (bankwiseRefreshCounter[rank.ID()] + 1) % memSpec->banksPerRank;
}
