/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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

#include "MemSpecSTTMRAM.h"

#include "DRAMSys/common/utils.h"

#include <iostream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

MemSpecSTTMRAM::MemSpecSTTMRAM(const DRAMUtils::MemSpec::MemSpecSTTMRAM& memSpec) :
    MemSpec(memSpec,
            memSpec.memarchitecturespec.nbrOfChannels,
            memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfBanks,
            1,
            memSpec.memarchitecturespec.nbrOfBanks,
            memSpec.memarchitecturespec.nbrOfBanks *
                memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfDevices),
    tCKE(tCK * memSpec.memtimingspec.CKE),
    tPD(tCKE),
    tCKESR(tCK * memSpec.memtimingspec.CKESR),
    tRAS(tCK * memSpec.memtimingspec.RAS),
    tRC(tCK * memSpec.memtimingspec.RC),
    tRCD(tCK * memSpec.memtimingspec.RCD),
    tRL(tCK * memSpec.memtimingspec.RL),
    tRTP(tCK * memSpec.memtimingspec.RTP),
    tWL(tCK * memSpec.memtimingspec.WL),
    tWR(tCK * memSpec.memtimingspec.WR),
    tXP(tCK * memSpec.memtimingspec.XP),
    tXS(tCK * memSpec.memtimingspec.XS),
    tRP(tCK * memSpec.memtimingspec.RP),
    tDQSCK(tCK * memSpec.memtimingspec.DQSCK),
    tCCD(tCK * memSpec.memtimingspec.CCD),
    tFAW(tCK * memSpec.memtimingspec.FAW),
    tRRD(tCK * memSpec.memtimingspec.RRD),
    tWTR(tCK * memSpec.memtimingspec.WTR),
    tXPDLL(tCK * memSpec.memtimingspec.XPDLL),
    tXSDLL(tCK * memSpec.memtimingspec.XSDLL),
    tAL(tCK * memSpec.memtimingspec.AL),
    tACTPDEN(tCK * memSpec.memtimingspec.ACTPDEN),
    tPRPDEN(tCK * memSpec.memtimingspec.PRPDEN),
    tRTRS(tCK * memSpec.memtimingspec.RTRS)
{
    uint64_t deviceSizeBits =
        static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * bitWidth;
    uint64_t deviceSizeBytes = deviceSizeBits / 8;
    memorySizeBytes = deviceSizeBytes * devicesPerRank * ranksPerChannel * numberOfChannels;

    std::cout << headline << std::endl;
    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:           "
              << "STT-MRAM" << std::endl;
    std::cout << " Memory size in bytes:  " << memorySizeBytes << std::endl;
    std::cout << " Channels:              " << numberOfChannels << std::endl;
    std::cout << " Ranks per channel:     " << ranksPerChannel << std::endl;
    std::cout << " Banks per rank:        " << banksPerRank << std::endl;
    std::cout << " Rows per bank:         " << rowsPerBank << std::endl;
    std::cout << " Columns per row:       " << columnsPerRow << std::endl;
    std::cout << " Device width in bits:  " << bitWidth << std::endl;
    std::cout << " Device size in bits:   " << deviceSizeBits << std::endl;
    std::cout << " Device size in bytes:  " << deviceSizeBytes << std::endl;
    std::cout << " Devices per rank:      " << devicesPerRank << std::endl;
    std::cout << std::endl;
}

// Returns the execution time for commands that have a fixed execution time
sc_time MemSpecSTTMRAM::getExecutionTime(Command command,
                                         [[maybe_unused]] const tlm_generic_payload& payload) const
{
    if (command == Command::PREPB || command == Command::PREAB)
        return tRP;

    if (command == Command::ACT)
        return tRCD;

    if (command == Command::RD)
        return tRL + burstDuration;

    if (command == Command::RDA)
        return tRTP + tRP;

    if (command == Command::WR || command == Command::MWR)
        return tWL + burstDuration;

    if (command == Command::WRA || command == Command::MWRA)
        return tWL + burstDuration + tWR + tRP;

    SC_REPORT_FATAL("getExecutionTime",
                    "command not known or command doesn't have a fixed execution time");
    return SC_ZERO_TIME;
}

TimeInterval MemSpecSTTMRAM::getIntervalOnDataStrobe(
    Command command, [[maybe_unused]] const tlm::tlm_generic_payload& payload) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL, tRL + burstDuration};

    if (command == Command::WR || command == Command::WRA || command == Command::MWR ||
        command == Command::MWRA)
        return {tWL, tWL + burstDuration};

    SC_REPORT_FATAL("MemSpec", "Method was called with invalid argument");
    throw;
}

bool MemSpecSTTMRAM::requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const
{
    return !allBytesEnabled(payload);
}

} // namespace DRAMSys
