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
 *    Derek Christ
 */

#include <iostream>

#include "../../common/utils.h"
#include "MemSpecHBM2.h"

using namespace sc_core;
using namespace tlm;

MemSpecHBM2::MemSpecHBM2(const DRAMSysConfiguration::MemSpec &memSpec)
    : MemSpec(memSpec, MemoryType::HBM2,
      memSpec.memArchitectureSpec.entries.at("nbrOfChannels"),
      memSpec.memArchitectureSpec.entries.at("nbrOfPseudoChannels"),
      memSpec.memArchitectureSpec.entries.at("nbrOfPseudoChannels"),
      memSpec.memArchitectureSpec.entries.at("nbrOfBanks"),
      memSpec.memArchitectureSpec.entries.at("nbrOfBankGroups"),
      memSpec.memArchitectureSpec.entries.at("nbrOfBanks")
          / memSpec.memArchitectureSpec.entries.at("nbrOfBankGroups"),
      memSpec.memArchitectureSpec.entries.at("nbrOfBanks")
          * memSpec.memArchitectureSpec.entries.at("nbrOfPseudoChannels"),
      memSpec.memArchitectureSpec.entries.at("nbrOfBankGroups")
          * memSpec.memArchitectureSpec.entries.at("nbrOfPseudoChannels"),
      memSpec.memArchitectureSpec.entries.at("nbrOfDevices")),
      tDQSCK  (tCK * memSpec.memTimingSpec.entries.at("DQSCK")),
      tRC     (tCK * memSpec.memTimingSpec.entries.at("RC")),
      tRAS    (tCK * memSpec.memTimingSpec.entries.at("RAS")),
      tRCDRD  (tCK * memSpec.memTimingSpec.entries.at("RCDRD")),
      tRCDWR  (tCK * memSpec.memTimingSpec.entries.at("RCDWR")),
      tRRDL   (tCK * memSpec.memTimingSpec.entries.at("RRDL")),
      tRRDS   (tCK * memSpec.memTimingSpec.entries.at("RRDS")),
      tFAW    (tCK * memSpec.memTimingSpec.entries.at("FAW")),
      tRTP    (tCK * memSpec.memTimingSpec.entries.at("RTP")),
      tRP     (tCK * memSpec.memTimingSpec.entries.at("RP")),
      tRL     (tCK * memSpec.memTimingSpec.entries.at("RL")),
      tWL     (tCK * memSpec.memTimingSpec.entries.at("WL")),
      tPL     (tCK * memSpec.memTimingSpec.entries.at("PL")),
      tWR     (tCK * memSpec.memTimingSpec.entries.at("WR")),
      tCCDL   (tCK * memSpec.memTimingSpec.entries.at("CCDL")),
      tCCDS   (tCK * memSpec.memTimingSpec.entries.at("CCDS")),
      tWTRL   (tCK * memSpec.memTimingSpec.entries.at("WTRL")),
      tWTRS   (tCK * memSpec.memTimingSpec.entries.at("WTRS")),
      tRTW    (tCK * memSpec.memTimingSpec.entries.at("RTW")),
      tXP     (tCK * memSpec.memTimingSpec.entries.at("XP")),
      tCKE    (tCK * memSpec.memTimingSpec.entries.at("CKE")),
      tPD     (tCKE),
      tCKESR  (tCKE + tCK),
      tXS     (tCK * memSpec.memTimingSpec.entries.at("XS")),
      tRFC    (tCK * memSpec.memTimingSpec.entries.at("RFC")),
      tRFCSB  (tCK * memSpec.memTimingSpec.entries.at("RFCSB")),
      tRREFD  (tCK * memSpec.memTimingSpec.entries.at("RREFD")),
      tREFI   (tCK * memSpec.memTimingSpec.entries.at("REFI")),
      tREFISB (tCK * memSpec.memTimingSpec.entries.at("REFISB"))
{
    commandLengthInCycles[Command::ACT] = 2;

    uint64_t deviceSizeBits = static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * bitWidth;
    uint64_t deviceSizeBytes = deviceSizeBits / 8;
    memorySizeBytes = deviceSizeBytes * ranksPerChannel * numberOfChannels;

    std::cout << headline << std::endl;
    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:                    " << "HBM2"           << std::endl;
    std::cout << " Memory size in bytes:           " << memorySizeBytes  << std::endl;
    std::cout << " Channels:                       " << numberOfChannels << std::endl;
    std::cout << " Pseudo channels per channel:    " << ranksPerChannel  << std::endl;
    std::cout << " Bank groups per pseudo channel: " << groupsPerRank    << std::endl;
    std::cout << " Banks per pseudo channel:       " << banksPerRank     << std::endl;
    std::cout << " Rows per bank:                  " << rowsPerBank      << std::endl;
    std::cout << " Columns per row:                " << columnsPerRow    << std::endl;
    std::cout << " Pseudo channel width in bits:   " << bitWidth         << std::endl;
    std::cout << " Pseudo channel size in bits:    " << deviceSizeBits   << std::endl;
    std::cout << " Pseudo channel size in bytes:   " << deviceSizeBytes  << std::endl;
    std::cout << std::endl;
}

sc_time MemSpecHBM2::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecHBM2::getRefreshIntervalPB() const
{
    return tREFISB;
}

bool MemSpecHBM2::hasRasAndCasBus() const
{
    return true;
}

sc_time MemSpecHBM2::getExecutionTime(Command command, const tlm_generic_payload &payload) const
{
    if (command == Command::PREPB || command == Command::PREAB)
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
    else if (command == Command::REFAB)
        return tRFC;
    else if (command == Command::REFPB)
        return tRFCSB;
    else
    {
        SC_REPORT_FATAL("getExecutionTime",
                        "command not known or command doesn't have a fixed execution time");
        return SC_ZERO_TIME;
    }
}

TimeInterval MemSpecHBM2::getIntervalOnDataStrobe(Command command, const tlm_generic_payload &) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL + tDQSCK, tRL + tDQSCK + burstDuration};
    else if (command == Command::WR || command == Command::WRA)
        return {tWL, tWL + burstDuration};
    else
    {
        SC_REPORT_FATAL("MemSpecHBM2", "Method was called with invalid argument");
        return {};
    }
}
