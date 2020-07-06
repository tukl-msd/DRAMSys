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

#include "MemSpecGDDR5X.h"

using namespace tlm;
using json = nlohmann::json;

MemSpecGDDR5X::MemSpecGDDR5X(json &memspec)
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
      tRP        (tCK * parseUint(memspec["memtimingspec"]["RP"], "RP")),
      tRAS       (tCK * parseUint(memspec["memtimingspec"]["RAS"], "RAS")),
      tRC        (tCK * parseUint(memspec["memtimingspec"]["RC"], "RC")),
      tRCDRD     (tCK * parseUint(memspec["memtimingspec"]["RCDRD"], "RCDRD")),
      tRCDWR     (tCK * parseUint(memspec["memtimingspec"]["RCDWR"], "RCDWR")),
      tRTP       (tCK * parseUint(memspec["memtimingspec"]["RTP"], "RTP")),
      tRRDS      (tCK * parseUint(memspec["memtimingspec"]["RRDS"], "RRDS")),
      tRRDL      (tCK * parseUint(memspec["memtimingspec"]["RRDL"], "RRDL")),
      tCCDS      (tCK * parseUint(memspec["memtimingspec"]["CCDS"], "CCDS")),
      tCCDL      (tCK * parseUint(memspec["memtimingspec"]["CCDL"], "CCDL")),
      tRL        (tCK * parseUint(memspec["memtimingspec"]["CL"], "CL")),
      tWCK2CKPIN (tCK * parseUint(memspec["memtimingspec"]["WCK2CKPIN"], "WCK2CKPIN")),
      tWCK2CK    (tCK * parseUint(memspec["memtimingspec"]["WCK2CK"], "WCK2CK")),
      tWCK2DQO   (tCK * parseUint(memspec["memtimingspec"]["WCK2DQO"], "WCK2DQO")),
      tRTW       (tCK * parseUint(memspec["memtimingspec"]["RTW"], "RTW")),
      tWL        (tCK * parseUint(memspec["memtimingspec"]["WL"], "WL")),
      tWCK2DQI   (tCK * parseUint(memspec["memtimingspec"]["WCK2DQI"], "WCK2DQI")),
      tWR        (tCK * parseUint(memspec["memtimingspec"]["WR"], "WR")),
      tWTRS      (tCK * parseUint(memspec["memtimingspec"]["WTRS"], "WTRS")),
      tWTRL      (tCK * parseUint(memspec["memtimingspec"]["WTRL"], "WTRL")),
      tCKE       (tCK * parseUint(memspec["memtimingspec"]["CKE"], "CKE")),
      tPD        (tCK * parseUint(memspec["memtimingspec"]["PD"], "PD")),
      tXP        (tCK * parseUint(memspec["memtimingspec"]["XP"], "XP")),
      tREFI      (tCK * parseUint(memspec["memtimingspec"]["REFI"], "REFI")),
      tREFIPB    (tCK * parseUint(memspec["memtimingspec"]["REFIPB"], "REFIPB")),
      tRFC       (tCK * parseUint(memspec["memtimingspec"]["RFC"], "RFC")),
      tRFCPB     (tCK * parseUint(memspec["memtimingspec"]["RFCPB"], "RFCPB")),
      tRREFD     (tCK * parseUint(memspec["memtimingspec"]["RREFD"], "RREFD")),
      tXS        (tCK * parseUint(memspec["memtimingspec"]["XS"], "XS")),
      tFAW       (tCK * parseUint(memspec["memtimingspec"]["FAW"], "FAW")),
      t32AW      (tCK * parseUint(memspec["memtimingspec"]["32AW"], "32AW")),
      tPPD       (tCK * parseUint(memspec["memtimingspec"]["PPD"], "PPD")),
      tLK        (tCK * parseUint(memspec["memtimingspec"]["LK"], "LK")),
      tRTRS      (tCK * parseUint(memspec["memtimingspec"]["RTRS"], "RTRS"))
{}

sc_time MemSpecGDDR5X::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecGDDR5X::getRefreshIntervalPB() const
{
    return tREFIPB;
}

sc_time MemSpecGDDR5X::getExecutionTime(Command command, const tlm_generic_payload &payload) const
{
    if (command == Command::PRE || command == Command::PREA)
        return tRP;
    else if (command == Command::ACT)
    {
        if (payload.get_command() == TLM_READ_COMMAND)
            return tRCDRD;
        else
            return tRCDWR;
    }
    else if (command == Command::RD)
        return tRL + tWCK2CKPIN + tWCK2CK + tWCK2DQO + burstDuration;
    else if (command == Command::RDA)
        return tRTP + tRP;
    else if (command == Command::WR)
        return tWL + tWCK2CKPIN + tWCK2CK + tWCK2DQI + burstDuration;
    else if (command == Command::WRA)
        return tWL + burstDuration + tWR + tRP;
    else if (command == Command::REFA)
        return tRFC;
    else if (command == Command::REFB)
        return tRFCPB;
    else
    {
        SC_REPORT_FATAL("getExecutionTime",
                        "command not known or command doesn't have a fixed execution time");
        return SC_ZERO_TIME;
    }
}

TimeInterval MemSpecGDDR5X::getIntervalOnDataStrobe(Command command) const
{
    if (command == Command::RD || command == Command::RDA)
        return TimeInterval(sc_time_stamp() + tRL + tWCK2CKPIN + tWCK2CK + tWCK2DQO,
                sc_time_stamp() + tRL + tWCK2CKPIN + tWCK2CK + tWCK2DQO + burstDuration);
    else if (command == Command::WR || command == Command::WRA)
        return TimeInterval(sc_time_stamp() + tWL + tWCK2CKPIN + tWCK2CK + tWCK2DQI,
                sc_time_stamp() + tWL + tWCK2CKPIN + tWCK2CK + tWCK2DQI + burstDuration);
    else
    {
        SC_REPORT_FATAL("MemSpecGDDR5X", "Method was called with invalid argument");
        return TimeInterval();
    }
}
