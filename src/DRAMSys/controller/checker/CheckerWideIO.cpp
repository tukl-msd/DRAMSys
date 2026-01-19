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

#include "CheckerWideIO.h"

#include "DRAMSys/common/DebugManager.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerWideIO::CheckerWideIO(const MemSpecWideIO& memSpec) :
    memSpec(memSpec)
{
    lastScheduledByCommandAndBank = std::vector<ControllerVector<Bank, sc_time>>(
        Command::numberOfCommands(),
        ControllerVector<Bank, sc_time>(memSpec.banksPerChannel, scMaxTime));
    lastScheduledByCommandAndRank = std::vector<ControllerVector<Rank, sc_time>>(
        Command::numberOfCommands(),
        ControllerVector<Rank, sc_time>(memSpec.ranksPerChannel, scMaxTime));
    lastScheduledByCommand = std::vector<sc_time>(Command::numberOfCommands(), scMaxTime);
    lastCommandOnBus = scMaxTime;
    last2Activates = ControllerVector<Rank, std::queue<sc_time>>(memSpec.ranksPerChannel);

    tBURST = memSpec.defaultBurstLength * memSpec.tCK;
    tRDWR = memSpec.tRL + tBURST + memSpec.tCK;
    tRDWR_R = memSpec.tRL + tBURST + memSpec.tRTRS - memSpec.tWL;
    tWRPRE = memSpec.tWL + tBURST - memSpec.tCK + memSpec.tWR;
    tWRRD = memSpec.tWL + tBURST - memSpec.tCK + memSpec.tWTR;
    tWRRD_R = memSpec.tWL + tBURST + memSpec.tRTRS - memSpec.tRL;
    tRDPDEN = memSpec.tRL + tBURST; // + memSpec.tCK; ??
    tWRPDEN = memSpec.tWL + tBURST + memSpec.tWR - memSpec.tCK;
    tWRAPDEN = memSpec.tWL + tBURST + memSpec.tWR; // + memSpec.tCK; ??
}

sc_time CheckerWideIO::timeToSatisfyConstraints(Command command,
                                                const tlm_generic_payload& payload) const
{
    Rank rank = ControllerExtension::getRank(payload);
    Bank bank = ControllerExtension::getBank(payload);

    sc_time lastCommandStart;
    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::RD || command == Command::RDA)
    {
        unsigned burstLength = ControllerExtension::getBurstLength(payload);
        assert((burstLength == 2) || (burstLength == 4));
        assert(burstLength <= memSpec.maxBurstLength);

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST);

        lastCommandStart =
            lastScheduledByCommand[Command::RD] != lastScheduledByCommandAndRank[Command::RD][rank]
                ? lastScheduledByCommand[Command::RD]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRTRS);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST);

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
                    std::max(earliestTimeToStart, lastCommandStart + tWRPRE - tBURST);
        }

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD);

        lastCommandStart =
            lastScheduledByCommand[Command::WR] != lastScheduledByCommandAndRank[Command::WR][rank]
                ? lastScheduledByCommand[Command::WR]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD);

        lastCommandStart = lastScheduledByCommand[Command::WRA] !=
                                   lastScheduledByCommandAndRank[Command::WRA][rank]
                               ? lastScheduledByCommand[Command::WRA]
                               : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRRD_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
    }
    else if (command == Command::WR || command == Command::WRA || command == Command::MWR ||
             command == Command::MWRA)
    {
        unsigned burstLength = ControllerExtension::getBurstLength(payload);
        assert((burstLength == 2) || (burstLength == 4));
        assert(burstLength <= memSpec.maxBurstLength);

        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCD);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR);

        lastCommandStart =
            lastScheduledByCommand[Command::RD] != lastScheduledByCommandAndRank[Command::RD][rank]
                ? lastScheduledByCommand[Command::RD]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR);

        lastCommandStart = lastScheduledByCommand[Command::RDA] !=
                                   lastScheduledByCommandAndRank[Command::RDA][rank]
                               ? lastScheduledByCommand[Command::RDA]
                               : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDWR_R);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST);

        lastCommandStart =
            lastScheduledByCommand[Command::WR] != lastScheduledByCommandAndRank[Command::WR][rank]
                ? lastScheduledByCommand[Command::WR]
                : scMaxTime;
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRTRS);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST);

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
    }
    else if (command == Command::ACT)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRD);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRP);

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

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR);

        if (last2Activates[rank].size() >= 2)
            earliestTimeToStart =
                std::max(earliestTimeToStart, last2Activates[rank].front() + memSpec.tTAW);
    }
    else if (command == Command::PREPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRAS);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RD][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

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
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tBURST);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
    }
    else if (command == Command::REFAB)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tBURST + memSpec.tRP);

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

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR);
    }
    else if (command == Command::PDEA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WR][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRAPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKE);
    }
    else if (command == Command::PDXA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::PDEA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKE);
    }
    else if (command == Command::PDEP)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRAPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR);
    }
    else if (command == Command::PDXP)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::PDEP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKE);
    }
    else if (command == Command::SREFEN)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(
                earliestTimeToStart, lastCommandStart + std::max(tRDPDEN, tBURST + memSpec.tRP));

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(
                earliestTimeToStart, lastCommandStart + std::max(tWRAPDEN, tWRPRE + memSpec.tRP));

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
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRFC);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR);
    }
    else if (command == Command::SREFEX)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEN][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tCKESR);
    }
    else
        SC_REPORT_FATAL("CheckerWideIO", "Unknown command!");

    // Check if command bus is free
    if (lastCommandOnBus != scMaxTime)
        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnBus + memSpec.tCK);

    return earliestTimeToStart;
}

void CheckerWideIO::insert(Command command, const tlm_generic_payload& payload)
{
    Rank rank = ControllerExtension::getRank(payload);
    Bank bank = ControllerExtension::getBank(payload);

    // Hack: Convert MWR to WR and MWRA to WRA
    if (command == Command::MWR)
        command = Command::WR;
    else if (command == Command::MWRA)
        command = Command::WRA;

    PRINTDEBUGMESSAGE("CheckerWideIO",
                      "Changing state on bank " + std::to_string(static_cast<std::size_t>(bank)) +
                          " command is " + command.toString());

    lastScheduledByCommandAndBank[command][bank] = sc_time_stamp();
    lastScheduledByCommandAndRank[command][rank] = sc_time_stamp();
    lastScheduledByCommand[command] = sc_time_stamp();
    lastCommandOnBus = sc_time_stamp();

    if (command == Command::ACT)
    {
        if (last2Activates[rank].size() == 2)
            last2Activates[rank].pop();
        last2Activates[rank].push(sc_time_stamp());
    }
}

} // namespace DRAMSys
