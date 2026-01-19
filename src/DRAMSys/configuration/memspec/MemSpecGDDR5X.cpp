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
 * Authors:
 *    Lukas Steiner
 *    Derek Christ
 *    Marco Mörz
 */

#include "MemSpecGDDR5X.h"

#include "DRAMSys/common/utils.h"

#include <iostream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

MemSpecGDDR5X::MemSpecGDDR5X(const DRAMUtils::MemSpec::MemSpecGDDR5X& memSpec) :
    MemSpec(memSpec,
            memSpec.memarchitecturespec.nbrOfChannels,
            memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfBanks,
            memSpec.memarchitecturespec.nbrOfBankGroups,
            memSpec.memarchitecturespec.nbrOfBanks /
                memSpec.memarchitecturespec.nbrOfBankGroups,
            memSpec.memarchitecturespec.nbrOfBanks *
                memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfBankGroups *
                memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfDevices),
    tRP(tCK * memSpec.memtimingspec.RP),
    tRAS(tCK * memSpec.memtimingspec.RAS),
    tRC(tCK * memSpec.memtimingspec.RC),
    tRCDRD(tCK * memSpec.memtimingspec.RCDRD),
    tRCDWR(tCK * memSpec.memtimingspec.RCDWR),
    tRTP(tCK * memSpec.memtimingspec.RTP),
    tRRDS(tCK * memSpec.memtimingspec.RRDS),
    tRRDL(tCK * memSpec.memtimingspec.RRDL),
    tCCDS(tCK * memSpec.memtimingspec.CCDS),
    tCCDL(tCK * memSpec.memtimingspec.CCDL),
    tRL(tCK * memSpec.memtimingspec.CL),
    tWCK2CKPIN(tCK * memSpec.memtimingspec.WCK2CKPIN),
    tWCK2CK(tCK * memSpec.memtimingspec.WCK2CK),
    tWCK2DQO(tCK * memSpec.memtimingspec.WCK2DQO),
    tRTW(tCK * memSpec.memtimingspec.RTW),
    tWL(tCK * memSpec.memtimingspec.WL),
    tWCK2DQI(tCK * memSpec.memtimingspec.WCK2DQI),
    tWR(tCK * memSpec.memtimingspec.WR),
    tWTRS(tCK * memSpec.memtimingspec.WTRS),
    tWTRL(tCK * memSpec.memtimingspec.WTRL),
    tCKE(tCK * memSpec.memtimingspec.CKE),
    tPD(tCK * memSpec.memtimingspec.PD),
    tXP(tCK * memSpec.memtimingspec.XP),
    tREFI(tCK * memSpec.memtimingspec.REFI),
    tREFIPB(tCK * memSpec.memtimingspec.REFIPB),
    tRFC(tCK * memSpec.memtimingspec.RFC),
    tRFCPB(tCK * memSpec.memtimingspec.RFCPB),
    tRREFD(tCK * memSpec.memtimingspec.RREFD),
    tXS(tCK * memSpec.memtimingspec.XS),
    tFAW(tCK * memSpec.memtimingspec.FAW),
    t32AW(tCK * memSpec.memtimingspec._32AW), // TODO breaking change
    tPPD(tCK * memSpec.memtimingspec.PPD),
    tLK(tCK * memSpec.memtimingspec.LK),
    tRTRS(tCK * memSpec.memtimingspec.TRS)
{
    uint64_t deviceSizeBits =
        static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * bitWidth;
    uint64_t deviceSizeBytes = deviceSizeBits / 8;
    memorySizeBytes = deviceSizeBytes * ranksPerChannel * numberOfChannels;

    std::cout << headline << std::endl;
    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:           "
              << "GDDR5X" << std::endl;
    std::cout << " Memory size in bytes:  " << memorySizeBytes << std::endl;
    std::cout << " Channels:              " << numberOfChannels << std::endl;
    std::cout << " Ranks per channel:     " << ranksPerChannel << std::endl;
    std::cout << " Bank groups per rank:  " << groupsPerRank << std::endl;
    std::cout << " Banks per rank:        " << banksPerRank << std::endl;
    std::cout << " Rows per bank:         " << rowsPerBank << std::endl;
    std::cout << " Columns per row:       " << columnsPerRow << std::endl;
    std::cout << " Device width in bits:  " << bitWidth << std::endl;
    std::cout << " Device size in bits:   " << deviceSizeBits << std::endl;
    std::cout << " Device size in bytes:  " << deviceSizeBytes << std::endl;
    std::cout << " Devices per rank:      " << devicesPerRank << std::endl;
    std::cout << std::endl;
}

sc_time MemSpecGDDR5X::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecGDDR5X::getRefreshIntervalPB() const
{
    return tREFIPB;
}

sc_time MemSpecGDDR5X::getExecutionTime(Command command, const tlm_generic_payload& payload) const
{
    if (command == Command::PREPB || command == Command::PREAB)
        return tRP;

    if (command == Command::ACT)
    {
        if (payload.get_command() == TLM_READ_COMMAND)
            return tRCDRD;

        return tRCDWR;
    }

    if (command == Command::RD)
        return tRL + tWCK2CKPIN + tWCK2CK + tWCK2DQO + burstDuration;

    if (command == Command::RDA)
        return tRTP + tRP;

    if (command == Command::WR)
        return tWL + tWCK2CKPIN + tWCK2CK + tWCK2DQI + burstDuration;

    if (command == Command::WRA)
        return tWL + burstDuration + tWR + tRP;

    if (command == Command::REFAB)
        return tRFC;

    if (command == Command::REFPB)
        return tRFCPB;

    SC_REPORT_FATAL("getExecutionTime",
                    "command not known or command doesn't have a fixed execution time");
    throw;
}

TimeInterval
MemSpecGDDR5X::getIntervalOnDataStrobe(Command command,
                                       [[maybe_unused]] const tlm_generic_payload& payload) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL + tWCK2CKPIN + tWCK2CK + tWCK2DQO,
                tRL + tWCK2CKPIN + tWCK2CK + tWCK2DQO + burstDuration};

    if (command == Command::WR || command == Command::WRA)
        return {tWL + tWCK2CKPIN + tWCK2CK + tWCK2DQI,
                tWL + tWCK2CKPIN + tWCK2CK + tWCK2DQI + burstDuration};

    SC_REPORT_FATAL("MemSpecGDDR5X", "Method was called with invalid argument");
    throw;
}

} // namespace DRAMSys
