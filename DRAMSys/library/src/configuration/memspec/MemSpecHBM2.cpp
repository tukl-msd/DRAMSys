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

#include "MemSpecHBM2.h"

using namespace tlm;
using json = nlohmann::json;

MemSpecHBM2::MemSpecHBM2(json &memspec)
    : MemSpec(memspec,
      parseUint(memspec["memarchitecturespec"]["nbrOfChannels"],"nbrOfChannels"),
      parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"], "nbrOfBankGroups"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks")
          / parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"], "nbrOfBankGroups"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks")
          * parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"], "nbrOfBankGroups")
          * parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      1),
      tDQSCK  (tCK * parseUint(memspec["memtimingspec"]["DQSCK"], "DQSCK")),
      tRC     (tCK * parseUint(memspec["memtimingspec"]["RC"], "RC")),
      tRAS    (tCK * parseUint(memspec["memtimingspec"]["RAS"], "RAS")),
      tRCDRD  (tCK * parseUint(memspec["memtimingspec"]["RCDRD"], "RCDRD")),
      tRCDWR  (tCK * parseUint(memspec["memtimingspec"]["RCDWR"], "RCDWR")),
      tRRDL   (tCK * parseUint(memspec["memtimingspec"]["RRDL"], "RRDL")),
      tRRDS   (tCK * parseUint(memspec["memtimingspec"]["RRDS"], "RRDS")),
      tFAW    (tCK * parseUint(memspec["memtimingspec"]["FAW"], "FAW")),
      tRTP    (tCK * parseUint(memspec["memtimingspec"]["RTP"], "RTP")),
      tRP     (tCK * parseUint(memspec["memtimingspec"]["RP"], "RP")),
      tRL     (tCK * parseUint(memspec["memtimingspec"]["RL"], "RL")),
      tWL     (tCK * parseUint(memspec["memtimingspec"]["WL"], "WL")),
      tPL     (tCK * parseUint(memspec["memtimingspec"]["PL"], "PL")),
      tWR     (tCK * parseUint(memspec["memtimingspec"]["WR"], "WR")),
      tCCDL   (tCK * parseUint(memspec["memtimingspec"]["CCDL"], "CCDL")),
      tCCDS   (tCK * parseUint(memspec["memtimingspec"]["CCDS"], "CCDS")),
      tWTRL   (tCK * parseUint(memspec["memtimingspec"]["WTRL"], "WTRL")),
      tWTRS   (tCK * parseUint(memspec["memtimingspec"]["WTRS"], "WTRS")),
      tRTW    (tCK * parseUint(memspec["memtimingspec"]["RTW"], "RTW")),
      tXP     (tCK * parseUint(memspec["memtimingspec"]["XP"], "XP")),
      tCKE    (tCK * parseUint(memspec["memtimingspec"]["CKE"], "CKE")),
      tPD     (tCKE),
      tCKESR  (tCKE + tCK),
      tXS     (tCK * parseUint(memspec["memtimingspec"]["XS"], "XS")),
      tRFC    (tCK * parseUint(memspec["memtimingspec"]["RFC"], "RFC")),
      tRFCSB  (tCK * parseUint(memspec["memtimingspec"]["RFCSB"], "RFCSB")),
      tRREFD  (tCK * parseUint(memspec["memtimingspec"]["RREFD"], "RREFD")),
      tREFI   (tCK * parseUint(memspec["memtimingspec"]["REFI"], "REFI")),
      tREFISB (tCK * parseUint(memspec["memtimingspec"]["REFISB"], "REFISB"))
{
    commandLengthInCycles[Command::ACT] = 2;
}

sc_time MemSpecHBM2::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecHBM2::getRefreshIntervalPB() const
{
    return tREFISB;
}

sc_time MemSpecHBM2::getExecutionTime(Command command, const tlm_generic_payload &payload) const
{
    if (command == Command::PRE || command == Command::PREA)
        return tRP;
    else if (command == Command::ACT)
    {
        if (payload.get_command() == TLM_READ_COMMAND)
            return tRCDRD + tCK;
        else
            return tRCDWR + tCK;
    }
    else if (command == Command::RD)
        return tRL + tDQSCK + burstDuration;
    else if (command == Command::RDA)
        return tRTP + tRP;
    else if (command == Command::WR)
        return tWL + burstDuration;
    else if (command == Command::WRA)
        return tWL + burstDuration + tWR + tRP;
    else if (command == Command::REFA)
        return tRFC;
    else if (command == Command::REFB)
        return tRFCSB;
    else
    {
        SC_REPORT_FATAL("getExecutionTime",
                        "command not known or command doesn't have a fixed execution time");
        return SC_ZERO_TIME;
    }
}

TimeInterval MemSpecHBM2::getIntervalOnDataStrobe(Command command) const
{
    if (command == Command::RD || command == Command::RDA)
        return TimeInterval(sc_time_stamp() + tRL + tDQSCK,
                            sc_time_stamp() + tRL + tDQSCK + burstDuration);
    else if (command == Command::WR || command == Command::WRA)
        return TimeInterval(sc_time_stamp() + tWL,
                            sc_time_stamp() + tWL + burstDuration);
    else
    {
        SC_REPORT_FATAL("MemSpecHBM2", "Method was called with invalid argument");
        return TimeInterval();
    }
}
