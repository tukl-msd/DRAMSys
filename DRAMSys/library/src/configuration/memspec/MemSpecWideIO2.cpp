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

#include "MemSpecWideIO2.h"

using namespace tlm;
using json = nlohmann::json;

MemSpecWideIO2::MemSpecWideIO2(json &memspec)
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
      tDQSCK  (tCK * parseUint(memspec["memtimingspec"]["DQSCK"], "DQSCK")),
      tDQSS   (tCK * parseUint(memspec["memtimingspec"]["DQSS"], "DQSS")),
      tCKE    (tCK * parseUint(memspec["memtimingspec"]["CKE"], "CKE")),
      tRL     (tCK * parseUint(memspec["memtimingspec"]["RL"], "RL")),
      tWL     (tCK * parseUint(memspec["memtimingspec"]["WL"], "WL")),
      tRCpb   (tCK * parseUint(memspec["memtimingspec"]["RCPB"], "RCPB")),
      tRCab   (tCK * parseUint(memspec["memtimingspec"]["RCAB"], "RCAB")),
      tCKESR  (tCK * parseUint(memspec["memtimingspec"]["CKESR"], "CKESR")),
      tXSR    (tCK * parseUint(memspec["memtimingspec"]["XSR"], "XSR")),
      tXP     (tCK * parseUint(memspec["memtimingspec"]["XP"], "XP")),
      tCCD    (tCK * parseUint(memspec["memtimingspec"]["CCD"], "CCD")),
      tRTP    (tCK * parseUint(memspec["memtimingspec"]["RTP"], "RTP")),
      tRCD    (tCK * parseUint(memspec["memtimingspec"]["RCD"], "RCD")),
      tRPpb   (tCK * parseUint(memspec["memtimingspec"]["RPPB"], "RPPB")),
      tRPab   (tCK * parseUint(memspec["memtimingspec"]["RPAB"], "RPAB")),
      tRAS    (tCK * parseUint(memspec["memtimingspec"]["RAS"], "RAS")),
      tWR     (tCK * parseUint(memspec["memtimingspec"]["WR"], "WR")),
      tWTR    (tCK * parseUint(memspec["memtimingspec"]["WTR"], "WTR")),
      tRRD    (tCK * parseUint(memspec["memtimingspec"]["RRD"], "RRD")),
      tFAW    (tCK * parseUint(memspec["memtimingspec"]["FAW"], "FAW")),
      tREFI   (tCK * (unsigned)(parseUint(memspec["memtimingspec"]["REFI"], "REFI")
              * parseUdouble(memspec["memtimingspec"]["REFM"], "REFM"))),
      tREFIpb (tCK * (unsigned)(parseUint(memspec["memtimingspec"]["REFIPB"], "REFIPB")
              * parseUdouble(memspec["memtimingspec"]["REFM"], "REFM"))),
      tRFCab  (tCK * parseUint(memspec["memtimingspec"]["RFCAB"], "RFCAB")),
      tRFCpb  (tCK * parseUint(memspec["memtimingspec"]["RFCPB"], "RFCPB")),
      tRTRS   (tCK * parseUint(memspec["memtimingspec"]["RTRS"], "RTRS"))
{}

sc_time MemSpecWideIO2::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecWideIO2::getRefreshIntervalPB() const
{
    return tREFIpb;
}

// Returns the execution time for commands that have a fixed execution time
sc_time MemSpecWideIO2::getExecutionTime(Command command, const tlm_generic_payload &) const
{
    if (command == Command::PRE)
        return tRPpb;
    else if (command == Command::PREA)
        return tRPab;
    else if (command == Command::ACT)
        return tRCD;
    else if (command == Command::RD)
        return tRL + tDQSCK + burstDuration;
    else if (command == Command::RDA)
        return burstDuration - 2 * tCK + tRTP + tRPpb;
    else if (command == Command::WR)
        return tWL + tDQSS + burstDuration;
    else if (command == Command::WRA)
        return tWL + burstDuration + tCK + tWR + tRPpb;
    else if (command == Command::REFA)
        return tRFCab;
    else if (command == Command::REFB)
        return tRFCpb;
    else
    {
        SC_REPORT_FATAL("MemSpecWideIO2::getExecutionTime",
                        "command not known or command doesn't have a fixed execution time");
        return SC_ZERO_TIME;
    }
}

TimeInterval MemSpecWideIO2::getIntervalOnDataStrobe(Command command) const
{
    if (command == Command::RD || command == Command::RDA)
        return TimeInterval(sc_time_stamp() + tRL + tDQSCK,
                sc_time_stamp() + tRL + tDQSCK + burstDuration);
    else if (command == Command::WR || command == Command::WRA)
        return TimeInterval(sc_time_stamp() + tWL + tDQSS,
                sc_time_stamp() + tWL + tDQSS + burstDuration);
    else
    {
        SC_REPORT_FATAL("MemSpec", "Method was called with invalid argument");
        return TimeInterval();
    }
}
