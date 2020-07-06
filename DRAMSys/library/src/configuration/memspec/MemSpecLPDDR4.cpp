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
 * Authors:
 *    Lukas Steiner
 */

#include "MemSpecLPDDR4.h"

using namespace tlm;
using json = nlohmann::json;

MemSpecLPDDR4::MemSpecLPDDR4(json &memspec)
    : MemSpec(memspec,
      parseUint(memspec["memarchitecturespec"]["nbrOfChannels"],"nbrOfChannels"),
      parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks"),
      1,
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks")
          * parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      1),
      tREFI   (tCK * parseUint(memspec["memtimingspec"]["REFI"], "REFI")),
      tREFIpb (tCK * parseUint(memspec["memtimingspec"]["REFIPB"], "REFIPB")),
      tRFCab  (tCK * parseUint(memspec["memtimingspec"]["RFCAB"], "RFCAB")),
      tRFCpb  (tCK * parseUint(memspec["memtimingspec"]["RFCPB"], "RFCPB")),
      tRPab   (tCK * parseUint(memspec["memtimingspec"]["RPAB"], "RPAB")),
      tRPpb   (tCK * parseUint(memspec["memtimingspec"]["RPPB"], "RPPB")),
      tRCab   (tCK * parseUint(memspec["memtimingspec"]["RCAB"], "RCAB")),
      tRCpb   (tCK * parseUint(memspec["memtimingspec"]["RCPB"], "RCPB")),
      tPPD    (tCK * parseUint(memspec["memtimingspec"]["PPD"], "PPD")),
      tRAS    (tCK * parseUint(memspec["memtimingspec"]["RAS"], "RAS")),
      tRCD    (tCK * parseUint(memspec["memtimingspec"]["RCD"], "RCD")),
      tFAW    (tCK * parseUint(memspec["memtimingspec"]["FAW"], "FAW")),
      tRRD    (tCK * parseUint(memspec["memtimingspec"]["RRD"], "RRD")),
      tCCD    (tCK * parseUint(memspec["memtimingspec"]["CCD"], "CCD")),
      tRL     (tCK * parseUint(memspec["memtimingspec"]["RL"], "RL")),
      tRPST   (tCK * parseUint(memspec["memtimingspec"]["RPST"], "RPST")),
      tDQSCK  (tCK * parseUint(memspec["memtimingspec"]["DQSCK"], "DQSCK")),
      tRTP    (tCK * parseUint(memspec["memtimingspec"]["RTP"], "RTP")),
      tWL     (tCK * parseUint(memspec["memtimingspec"]["WL"], "WL")),
      tDQSS   (tCK * parseUint(memspec["memtimingspec"]["DQSS"], "DQSS")),
      tDQS2DQ (tCK * parseUint(memspec["memtimingspec"]["DQS2DQ"], "DQS2DQ")),
      tWR     (tCK * parseUint(memspec["memtimingspec"]["WR"], "WR")),
      tWPRE   (tCK * parseUint(memspec["memtimingspec"]["WPRE"], "WPRE")),
      tWTR    (tCK * parseUint(memspec["memtimingspec"]["WTR"], "WTR")),
      tXP     (tCK * parseUint(memspec["memtimingspec"]["XP"], "XP")),
      tSR     (tCK * parseUint(memspec["memtimingspec"]["SR"], "SR")),
      tXSR    (tCK * parseUint(memspec["memtimingspec"]["XSR"], "XSR")),
      tESCKE  (tCK * parseUint(memspec["memtimingspec"]["ESCKE"], "ESCKE")),
      tCKE    (tCK * parseUint(memspec["memtimingspec"]["CKE"], "CKE")),
      tCMDCKE (tCK * parseUint(memspec["memtimingspec"]["CMDCKE"], "CMDCKE")),
      tRTRS   (tCK * parseUint(memspec["memtimingspec"]["RTRS"], "RTRS"))
{
    commandLengthInCycles[Command::ACT] = 4;
    commandLengthInCycles[Command::PRE] = 2;
    commandLengthInCycles[Command::PREA] = 2;
    commandLengthInCycles[Command::RD] = 4;
    commandLengthInCycles[Command::RDA] = 4;
    commandLengthInCycles[Command::WR] = 4;
    commandLengthInCycles[Command::WRA] = 4;
    commandLengthInCycles[Command::REFA] = 2;
    commandLengthInCycles[Command::REFB] = 2;
    commandLengthInCycles[Command::SREFEN] = 2;
    commandLengthInCycles[Command::SREFEX] = 2;
}

sc_time MemSpecLPDDR4::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecLPDDR4::getRefreshIntervalPB() const
{
    return tREFIpb;
}

sc_time MemSpecLPDDR4::getExecutionTime(Command command, const tlm_generic_payload &) const
{
    if (command == Command::PRE)
        return tRPpb + tCK;
    else if (command == Command::PREA)
        return tRPab + tCK;
    else if (command == Command::ACT)
        return tRCD + 3 * tCK;
    else if (command == Command::RD)
        return tRL + tDQSCK + burstDuration + 3 * tCK;
    else if (command == Command::RDA)
        return burstDuration + tRTP - 5 * tCK + tRPpb;
    else if (command == Command::WR)
        return tWL + tDQSS + tDQS2DQ + burstDuration + 3 * tCK;
    else if (command == Command::WRA)
        return tWL + 4 * tCK + burstDuration + tWR + tRPpb;
    else if (command == Command::REFA)
        return tRFCab + tCK;
    else if (command == Command::REFB)
        return tRFCpb + tCK;
    else
    {
        SC_REPORT_FATAL("getExecutionTime",
                        "command not known or command doesn't have a fixed execution time");
        return SC_ZERO_TIME;
    }
}

TimeInterval MemSpecLPDDR4::getIntervalOnDataStrobe(Command command) const
{
    if (command == Command::RD || command == Command::RDA)
        return TimeInterval(sc_time_stamp() + tRL + tDQSCK + 3 * tCK,
                            sc_time_stamp() + tRL + tDQSCK + burstDuration + 3 * tCK);
    else if (command == Command::WR || command == Command::WRA)
        return TimeInterval(sc_time_stamp() + tWL + tDQSS + tDQS2DQ + 3 * tCK,
                            sc_time_stamp() + tWL + tDQSS + tDQS2DQ + burstDuration + 3 * tCK);
    else
    {
        SC_REPORT_FATAL("MemSpecLPDDR4", "Method was called with invalid argument");
        return TimeInterval();
    }
}

