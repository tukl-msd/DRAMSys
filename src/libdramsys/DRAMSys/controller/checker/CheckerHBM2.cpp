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

#include "CheckerHBM2.h"

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/configuration/memspec/MemSpecHBM2.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerHBM2::CheckerHBM2(const MemSpecHBM2& memSpec) :
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
    lastCommandOnRasBus = scMaxTime;
    lastCommandOnCasBus = scMaxTime;
    last4Activates = ControllerVector<Rank, std::queue<sc_time>>(memSpec.ranksPerChannel);

    bankwiseRefreshCounter = ControllerVector<Rank, unsigned>(memSpec.ranksPerChannel);

    tBURST = memSpec.defaultBurstLength / memSpec.dataRate * memSpec.tCK;
    tRDPDE = memSpec.tRL + memSpec.tPL + tBURST + memSpec.tCK;
    tRDSRE = tRDPDE;
    tWRPRE = memSpec.tWL + tBURST + memSpec.tWR;
    tWRPDE = memSpec.tWL + memSpec.tPL + tBURST + memSpec.tCK + memSpec.tWR;
    tWRAPDE = memSpec.tWL + memSpec.tPL + tBURST + memSpec.tCK + memSpec.tWR;
    tWRRDS = memSpec.tWL + tBURST + memSpec.tWTRS;
    tWRRDL = memSpec.tWL + tBURST + memSpec.tWTRL;
}

sc_time CheckerHBM2::timeToSatisfyConstraints(Command command,
                                              const tlm_generic_payload& payload) const
{
    Rank rank = ControllerExtension::getRank(payload);
    BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    Bank bank = ControllerExtension::getBank(payload);

    sc_time lastCommandStart;
    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::RD || command == Command::RDA)
    {
        unsigned burstLength = ControllerExtension::getBurstLength(payload);
        assert(!(memSpec.ranksPerChannel == 1) ||
               (burstLength == 2 || burstLength == 4));                 // Legacy mode
        assert(!(memSpec.ranksPerChannel == 2) || (burstLength == 4)); // Pseudo-channel mode

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCDRD + memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RD][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::RDA][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        if (command == Command::RDA)
        {
            lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank];
            if (lastCommandStart != scMaxTime)
                earliestTimeToStart =
                    std::max(earliestTimeToStart, lastCommandStart + tWRPRE - memSpec.tRTP);
        }

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRDS);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
    }
    else if (command == Command::WR || command == Command::WRA || command == Command::MWR ||
             command == Command::MWRA)
    {
        unsigned burstLength = ControllerExtension::getBurstLength(payload);
        assert(!(memSpec.ranksPerChannel == 1) ||
               (burstLength == 2 || burstLength == 4));                 // Legacy mode
        assert(!(memSpec.ranksPerChannel == 2) || (burstLength == 4)); // Pseudo-channel mode

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCDWR + memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTW);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTW);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WR][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::WRA][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDL);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDS);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
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
                std::max(earliestTimeToStart,
                         lastCommandStart + memSpec.tRTP + memSpec.tRP - memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart,
                                           lastCommandStart + tWRPRE + memSpec.tRP - memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PREPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP - memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP - memSpec.tCK);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP - memSpec.tCK);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP - memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFC - memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCSB - memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRREFD - memSpec.tCK);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS - memSpec.tCK);

        if (last4Activates[rank].size() >= 4)
            earliestTimeToStart = std::max(
                earliestTimeToStart, last4Activates[rank].front() + memSpec.tFAW - memSpec.tCK);
    }
    else if (command == Command::PREPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRAS + memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RD][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRTP);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
    }
    else if (command == Command::PREAB)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRAS + memSpec.tCK);

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

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCSB);
    }
    else if (command == Command::REFAB)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC + memSpec.tCK);

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

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCSB);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);
    }
    else if (command == Command::REFPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC + memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBankGroup[Command::ACT][bankGroup];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRDL + memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRDS + memSpec.tCK);

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

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFC);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCSB);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
        {
            if (bankwiseRefreshCounter[rank] == 0)
                earliestTimeToStart =
                    std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCSB);
            else
                earliestTimeToStart =
                    std::max(earliestTimeToStart, lastCommandStart + memSpec.tRREFD);
        }

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);

        if (last4Activates[rank].size() >= 4)
            earliestTimeToStart =
                std::max(earliestTimeToStart, last4Activates[rank].front() + memSpec.tFAW);
    }
    else if (command == Command::PDEA)
    {
        lastCommandStart = lastScheduledByCommand[Command::RD];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::RDA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::WR];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPDE);

        lastCommandStart = lastScheduledByCommand[Command::WRA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRAPDE);

        lastCommandStart = lastScheduledByCommand[Command::PDXA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKE);
    }
    else if (command == Command::PDXA)
    {
        lastCommandStart = lastScheduledByCommand[Command::PDEA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tPD);
    }
    else if (command == Command::PDEP)
    {
        lastCommandStart = lastScheduledByCommand[Command::RD];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::RDA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDE);

        lastCommandStart = lastScheduledByCommand[Command::WRA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRAPDE);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKE);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);
    }
    else if (command == Command::PDXP)
    {
        lastCommandStart = lastScheduledByCommand[Command::PDEP];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tPD);
    }
    else if (command == Command::SREFEN)
    {
        lastCommandStart = lastScheduledByCommand[Command::ACT];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC + memSpec.tCK);

        lastCommandStart = lastScheduledByCommand[Command::RDA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart,
                         lastCommandStart + std::max(memSpec.tRTP + memSpec.tRP, tRDSRE));

        lastCommandStart = lastScheduledByCommand[Command::WRA];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec.tRP);

        lastCommandStart = lastScheduledByCommand[Command::PREPB];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommand[Command::PREAB];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRP);

        lastCommandStart = lastScheduledByCommand[Command::PDXP];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommand[Command::REFAB];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFC);

        lastCommandStart = lastScheduledByCommand[Command::REFPB];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFCSB);

        lastCommandStart = lastScheduledByCommand[Command::SREFEX];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXS);
    }
    else if (command == Command::SREFEX)
    {
        lastCommandStart = lastScheduledByCommand[Command::SREFEN];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKESR);
    }
    else
        SC_REPORT_FATAL("CheckerHBM2", "Unknown command!");

    if (command.isRasCommand())
    {
        if (lastCommandOnRasBus != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnRasBus + memSpec.tCK);
    }
    else
    {
        if (lastCommandOnCasBus != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnCasBus + memSpec.tCK);
    }

    return earliestTimeToStart;
}

void CheckerHBM2::insert(Command command, const tlm_generic_payload& payload)
{
    Rank rank = ControllerExtension::getRank(payload);
    BankGroup bankGroup = ControllerExtension::getBankGroup(payload);
    Bank bank = ControllerExtension::getBank(payload);

    // Hack: Convert MWR to WR and MWRA to WRA
    if (command == Command::MWR)
        command = Command::WR;
    else if (command == Command::MWRA)
        command = Command::WRA;

    PRINTDEBUGMESSAGE("CheckerHBM2",
                      "Changing state on bank " + std::to_string(static_cast<std::size_t>(bank)) +
                          " command is " + command.toString());

    lastScheduledByCommandAndBank[command][bank] = sc_time_stamp();
    lastScheduledByCommandAndBankGroup[command][bankGroup] = sc_time_stamp();
    lastScheduledByCommandAndRank[command][rank] = sc_time_stamp();
    lastScheduledByCommand[command] = sc_time_stamp();

    if (command.isCasCommand())
        lastCommandOnCasBus = sc_time_stamp();
    else if (command == Command::ACT)
        lastCommandOnRasBus = sc_time_stamp() + memSpec.tCK;
    else
        lastCommandOnRasBus = sc_time_stamp();

    if (command == Command::ACT || command == Command::REFPB)
    {
        if (last4Activates[rank].size() == 4)
            last4Activates[rank].pop();
        last4Activates[rank].push(lastCommandOnRasBus);
    }

    if (command == Command::REFPB)
        bankwiseRefreshCounter[rank] = (bankwiseRefreshCounter[rank] + 1) % memSpec.banksPerRank;
}

} // namespace DRAMSys
