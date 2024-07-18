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

#include "CheckerLPDDR4.h"

#include "DRAMSys/common/DebugManager.h"

#include <algorithm>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

CheckerLPDDR4::CheckerLPDDR4(const MemSpecLPDDR4& memSpec) :
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
    last4Activates = ControllerVector<Rank, std::queue<sc_time>>(memSpec.ranksPerChannel);

    lastBurstLengthByCommandAndBank = ControllerVector<Command, ControllerVector<Bank, uint8_t>>(
        Command::WRA + 1, ControllerVector<Bank, uint8_t>(memSpec.banksPerChannel));

    tBURST = memSpec.defaultBurstLength / memSpec.dataRate * memSpec.tCK;
    tRDWR =
        memSpec.tRL + memSpec.tDQSCK + tBURST - memSpec.tWL + memSpec.tWPRE + memSpec.tRPST;
    tRDWR_R = memSpec.tRL + tBURST + memSpec.tRTRS - memSpec.tWL;
    tWRRD = memSpec.tWL + memSpec.tCK + tBURST + memSpec.tWTR;
    tWRRD_R = memSpec.tWL + tBURST + memSpec.tRTRS - memSpec.tRL;
    tRDPRE = memSpec.tRTP + tBURST - 6 * memSpec.tCK;
    tRDAACT = memSpec.tRTP + tBURST - 8 * memSpec.tCK + memSpec.tRPpb;
    tWRPRE = memSpec.tWL + tBURST + memSpec.tCK + memSpec.tWR + 2 * memSpec.tCK;
    tWRAACT = memSpec.tWL + tBURST + memSpec.tCK + memSpec.tWR + memSpec.tRPpb;
    tACTPDEN = 3 * memSpec.tCK + memSpec.tCMDCKE;
    tPRPDEN = memSpec.tCK + memSpec.tCMDCKE;
    tRDPDEN = 3 * memSpec.tCK + memSpec.tRL + memSpec.tDQSCK + tBURST + memSpec.tRPST;
    tWRPDEN =
        3 * memSpec.tCK + memSpec.tWL +
        (std::ceil(memSpec.tDQSS / memSpec.tCK) + std::ceil(memSpec.tDQS2DQ / memSpec.tCK)) *
            memSpec.tCK +
        tBURST + memSpec.tWR;
    tWRAPDEN =
        3 * memSpec.tCK + memSpec.tWL +
        (std::ceil(memSpec.tDQSS / memSpec.tCK) + std::ceil(memSpec.tDQS2DQ / memSpec.tCK)) *
            memSpec.tCK +
        tBURST + memSpec.tWR + 2 * memSpec.tCK;
    tREFPDEN = memSpec.tCK + memSpec.tCMDCKE;
}

sc_time CheckerLPDDR4::timeToSatisfyConstraints(Command command,
                                                const tlm_generic_payload& payload) const
{
    Rank rank = ControllerExtension::getRank(payload);
    Bank bank = ControllerExtension::getBank(payload);

    sc_time lastCommandStart;
    sc_time earliestTimeToStart = sc_time_stamp();

    if (command == Command::RD || command == Command::RDA)
    {
        unsigned burstLength = ControllerExtension::getBurstLength(payload);
        assert((burstLength == 16) || (burstLength == 32)); // TODO: BL16/32 OTF
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
                    std::max(earliestTimeToStart, lastCommandStart + tWRPRE - tRDPRE);
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
        assert((burstLength == 16) || (burstLength == 32));
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

        if (command == Command::MWR || command == Command::MWRA)
        {
            lastCommandStart = lastScheduledByCommandAndBank[Command::WR][bank];
            if (lastCommandStart != scMaxTime)
            {
                if (lastBurstLengthByCommandAndBank[Command::WR][bank] == 32)
                    earliestTimeToStart = std::max(
                        earliestTimeToStart, lastCommandStart + memSpec.tCCDMW + 8 * memSpec.tCK);
                else
                    earliestTimeToStart =
                        std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDMW);
            }

            lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank];
            {
                if (lastBurstLengthByCommandAndBank[Command::WRA][bank] == 32)
                    earliestTimeToStart = std::max(
                        earliestTimeToStart, lastCommandStart + memSpec.tCCDMW + 8 * memSpec.tCK);
                else
                    earliestTimeToStart =
                        std::max(earliestTimeToStart, lastCommandStart + memSpec.tCCDMW);
            }
        }

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);
    }
    else if (command == Command::ACT)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRD);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDAACT);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tWRAACT);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PREPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPpb - 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPab - 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXP][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart,
                                           lastCommandStart + memSpec.tRFCab - 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::REFPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart,
                                           lastCommandStart + memSpec.tRFCpb - 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRD - 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEX][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR - 2 * memSpec.tCK);

        if (last4Activates[rank].size() >= 4)
            earliestTimeToStart =
                std::max(earliestTimeToStart,
                         last4Activates[rank].front() + memSpec.tFAW - 3 * memSpec.tCK);
    }
    else if (command == Command::PREPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRAS + 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RD][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPRE);

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
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRAS + 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RD][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPRE);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tRDPRE);

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
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCpb + 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tRDPRE + memSpec.tRPpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec.tRPpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPab);

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
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR);
    }
    else if (command == Command::REFPB)
    {
        lastCommandStart = lastScheduledByCommandAndBank[Command::ACT][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCpb + 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRRD + 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndBank[Command::RDA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tRDPRE + memSpec.tRPpb);

        lastCommandStart = lastScheduledByCommandAndBank[Command::WRA][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + tWRPRE + memSpec.tRPpb);

        lastCommandStart = lastScheduledByCommandAndBank[Command::PREPB][bank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPab);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PDXA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXP);

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
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR);

        if (last4Activates[rank].size() >= 4)
            earliestTimeToStart = std::max(
                earliestTimeToStart, last4Activates[rank].front() + memSpec.tFAW - memSpec.tCK);
    }
    else if (command == Command::PDEA)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::ACT][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tACTPDEN);

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

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tPRPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tREFPDEN);

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

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tPRPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tPRPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tREFPDEN);

        lastCommandStart = lastScheduledByCommandAndRank[Command::REFPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + tREFPDEN);

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
            earliestTimeToStart =
                std::max(earliestTimeToStart, lastCommandStart + memSpec.tRCpb + 2 * memSpec.tCK);

        lastCommandStart = lastScheduledByCommandAndRank[Command::RDA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(
                earliestTimeToStart, lastCommandStart + std::max(tRDPDEN, tRDPRE + memSpec.tRPpb));

        lastCommandStart = lastScheduledByCommandAndRank[Command::WRA][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart =
                std::max(earliestTimeToStart,
                         lastCommandStart + std::max(tWRAPDEN, tWRPRE + memSpec.tRPpb));

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREPB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPpb);

        lastCommandStart = lastScheduledByCommandAndRank[Command::PREAB][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tRPab);

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
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tXSR);
    }
    else if (command == Command::SREFEX)
    {
        lastCommandStart = lastScheduledByCommandAndRank[Command::SREFEN][rank];
        if (lastCommandStart != scMaxTime)
            earliestTimeToStart = std::max(earliestTimeToStart, lastCommandStart + memSpec.tSR);
    }
    else
        SC_REPORT_FATAL("CheckerLPDDR4", "Unknown command!");

    // Check if command bus is free
    if (lastCommandOnBus != scMaxTime)
        earliestTimeToStart = std::max(earliestTimeToStart, lastCommandOnBus + memSpec.tCK);

    return earliestTimeToStart;
}

void CheckerLPDDR4::insert(Command command, const tlm_generic_payload& payload)
{
    Rank rank = ControllerExtension::getRank(payload);
    Bank bank = ControllerExtension::getBank(payload);

    // Hack: Convert MWR to WR and MWRA to WRA
    if (command == Command::MWR)
        command = Command::WR;
    else if (command == Command::MWRA)
        command = Command::WRA;

    PRINTDEBUGMESSAGE("CheckerLPDDR4",
                      "Changing state on bank " + std::to_string(static_cast<std::size_t>(bank)) +
                          " command is " + command.toString());

    lastScheduledByCommandAndBank[command][bank] = sc_time_stamp();
    lastScheduledByCommandAndRank[command][rank] = sc_time_stamp();
    lastScheduledByCommand[command] = sc_time_stamp();

    lastCommandOnBus = sc_time_stamp() + memSpec.getCommandLength(command) - memSpec.tCK;

    if (command == Command::ACT || command == Command::REFPB)
    {
        if (last4Activates[rank].size() == 4)
            last4Activates[rank].pop();
        last4Activates[rank].push(lastCommandOnBus);
    }

    if (command.isCasCommand())
    {
        unsigned burstLength = ControllerExtension::getBurstLength(payload);
        lastBurstLengthByCommandAndBank[command][bank] = burstLength;
    }
}

} // namespace DRAMSys
