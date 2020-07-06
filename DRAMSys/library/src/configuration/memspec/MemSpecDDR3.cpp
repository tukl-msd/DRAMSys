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

#include "MemSpecDDR3.h"

using namespace tlm;
using json = nlohmann::json;

MemSpecDDR3::MemSpecDDR3(json &memspec)
    : MemSpec(memspec,
      parseUint(memspec["memarchitecturespec"]["nbrOfChannels"],"nbrOfChannels"),
      parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks"),
      1,
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks")
          * parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks"),
      parseUint(memspec["memarchitecturespec"]["nbrOfDevicesOnDIMM"],"nbrOfDevicesOnDIMM")),
      tCKE     (tCK * parseUint(memspec["memtimingspec"]["CKE"], "CKE")),
      tPD      (tCKE),
      tCKESR   (tCK * parseUint(memspec["memtimingspec"]["CKESR"], "CKESR")),
      tDQSCK   (tCK * parseUint(memspec["memtimingspec"]["DQSCK"], "DQSCK")),
      tRAS     (tCK * parseUint(memspec["memtimingspec"]["RAS"], "RAS")),
      tRC      (tCK * parseUint(memspec["memtimingspec"]["RC"], "RC")),
      tRCD     (tCK * parseUint(memspec["memtimingspec"]["RCD"], "RCD")),
      tRL      (tCK * parseUint(memspec["memtimingspec"]["RL"], "RL")),
      tRTP     (tCK * parseUint(memspec["memtimingspec"]["RTP"], "RTP")),
      tWL      (tCK * parseUint(memspec["memtimingspec"]["WL"], "WL")),
      tWR      (tCK * parseUint(memspec["memtimingspec"]["WR"], "WR")),
      tXP      (tCK * parseUint(memspec["memtimingspec"]["XP"], "XP")),
      tXS      (tCK * parseUint(memspec["memtimingspec"]["XS"], "XS")),
      tCCD     (tCK * parseUint(memspec["memtimingspec"]["CCD"], "CCD")),
      tFAW     (tCK * parseUint(memspec["memtimingspec"]["FAW"], "FAW")),
      tREFI    (tCK * parseUint(memspec["memtimingspec"]["REFI"], "REFI")),
      tRFC     (tCK * parseUint(memspec["memtimingspec"]["RFC"], "RFC")),
      tRP      (tCK * parseUint(memspec["memtimingspec"]["RP"], "RP")),
      tRRD     (tCK * parseUint(memspec["memtimingspec"]["RRD"], "RRD")),
      tWTR     (tCK * parseUint(memspec["memtimingspec"]["WTR"], "WTR")),
      tAL      (tCK * parseUint(memspec["memtimingspec"]["AL"], "AL")),
      tXPDLL   (tCK * parseUint(memspec["memtimingspec"]["XPDLL"], "XPDLL")),
      tXSDLL   (tCK * parseUint(memspec["memtimingspec"]["XSDLL"], "XSDLL")),
      tACTPDEN (tCK * parseUint(memspec["memtimingspec"]["ACTPDEN"], "ACTPDEN")),
      tPRPDEN  (tCK * parseUint(memspec["memtimingspec"]["PRPDEN"], "PRPDEN")),
      tREFPDEN (tCK * parseUint(memspec["memtimingspec"]["REFPDEN"], "REFPDEN")),
      tRTRS    (tCK * parseUint(memspec["memtimingspec"]["RTRS"], "RTRS")),
      iDD0     (parseUdouble(memspec["mempowerspec"]["idd0"], "idd0")),
      iDD2N    (parseUdouble(memspec["mempowerspec"]["idd2n"], "idd2n")),
      iDD3N    (parseUdouble(memspec["mempowerspec"]["idd3n"], "idd3n")),
      iDD4R    (parseUdouble(memspec["mempowerspec"]["idd4r"], "idd4r")),
      iDD4W    (parseUdouble(memspec["mempowerspec"]["idd4w"], "idd4w")),
      iDD5     (parseUdouble(memspec["mempowerspec"]["idd5"], "idd5")),
      iDD6     (parseUdouble(memspec["mempowerspec"]["idd6"], "idd6")),
      vDD      (parseUdouble(memspec["mempowerspec"]["vdd"], "vdd")),
      iDD2P0   (parseUdouble(memspec["mempowerspec"]["idd2p0"], "idd2p0")),
      iDD2P1   (parseUdouble(memspec["mempowerspec"]["idd2p1"], "idd2p1")),
      iDD3P0   (parseUdouble(memspec["mempowerspec"]["idd3p0"], "idd3p0")),
      iDD3P1   (parseUdouble(memspec["mempowerspec"]["idd3p1"], "idd3p1"))
{}

sc_time MemSpecDDR3::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecDDR3::getRefreshIntervalPB() const
{
    SC_REPORT_FATAL("MemSpecDDR3", "Per bank refresh not supported");
    return SC_ZERO_TIME;
}

// Returns the execution time for commands that have a fixed execution time
sc_time MemSpecDDR3::getExecutionTime(Command command, const tlm_generic_payload &) const
{
    if (command == Command::PRE || command == Command::PREA)
        return tRP;
    else if (command == Command::ACT)
        return tRCD;
    else if (command == Command::RD)
        return tRL + burstDuration;
    else if (command == Command::RDA)
        return tRTP + tRP;
    else if (command == Command::WR)
        return tWL + burstDuration;
    else if (command == Command::WRA)
        return tWL + burstDuration + tWR + tRP;
    else if (command == Command::REFA)
        return tRFC;
    else
    {
        SC_REPORT_FATAL("getExecutionTime",
                        "command not known or command doesn't have a fixed execution time");
        return SC_ZERO_TIME;
    }
}

TimeInterval MemSpecDDR3::getIntervalOnDataStrobe(Command command) const
{
    if (command == Command::RD || command == Command::RDA)
        return TimeInterval(sc_time_stamp() + tRL, sc_time_stamp() + tRL + burstDuration);
    else if (command == Command::WR || command == Command::WRA)
        return TimeInterval(sc_time_stamp() + tWL, sc_time_stamp() + tWL + burstDuration);
    else
    {
        SC_REPORT_FATAL("MemSpec", "Method was called with invalid argument");
        return TimeInterval();
    }
}
