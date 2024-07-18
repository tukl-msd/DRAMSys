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
 */

#include "MemSpecLPDDR4.h"

#include "DRAMSys/common/utils.h"

#include <iostream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

MemSpecLPDDR4::MemSpecLPDDR4(const Config::MemSpec& memSpec) :
    MemSpec(memSpec,
            memSpec.memarchitecturespec.entries.at("nbrOfChannels"),
            1,
            memSpec.memarchitecturespec.entries.at("nbrOfRanks"),
            memSpec.memarchitecturespec.entries.at("nbrOfBanks"),
            1,
            memSpec.memarchitecturespec.entries.at("nbrOfBanks"),
            memSpec.memarchitecturespec.entries.at("nbrOfBanks") *
                memSpec.memarchitecturespec.entries.at("nbrOfRanks"),
            memSpec.memarchitecturespec.entries.at("nbrOfRanks"),
            memSpec.memarchitecturespec.entries.at("nbrOfDevices")),
    tREFI(tCK * memSpec.memtimingspec.entries.at("REFI")),
    tREFIpb(tCK * memSpec.memtimingspec.entries.at("REFIPB")),
    tRFCab(tCK * memSpec.memtimingspec.entries.at("RFCAB")),
    tRFCpb(tCK * memSpec.memtimingspec.entries.at("RFCPB")),
    tRAS(tCK * memSpec.memtimingspec.entries.at("RAS")),
    tRPab(tCK * memSpec.memtimingspec.entries.at("RPAB")),
    tRPpb(tCK * memSpec.memtimingspec.entries.at("RPPB")),
    tRCpb(tCK * memSpec.memtimingspec.entries.at("RCPB")),
    tRCab(tCK * memSpec.memtimingspec.entries.at("RCAB")),
    tPPD(tCK * memSpec.memtimingspec.entries.at("PPD")),
    tRCD(tCK * memSpec.memtimingspec.entries.at("RCD")),
    tFAW(tCK * memSpec.memtimingspec.entries.at("FAW")),
    tRRD(tCK * memSpec.memtimingspec.entries.at("RRD")),
    tCCD(tCK * memSpec.memtimingspec.entries.at("CCD")),
    tCCDMW(tCK * memSpec.memtimingspec.entries.at("CCDMW")),
    tRL(tCK * memSpec.memtimingspec.entries.at("RL")),
    tRPST(tCK * memSpec.memtimingspec.entries.at("RPST")),
    tDQSCK(tCK * memSpec.memtimingspec.entries.at("DQSCK")),
    tRTP(tCK * memSpec.memtimingspec.entries.at("RTP")),
    tWL(tCK * memSpec.memtimingspec.entries.at("WL")),
    tDQSS(tCK * memSpec.memtimingspec.entries.at("DQSS")),
    tDQS2DQ(tCK * memSpec.memtimingspec.entries.at("DQS2DQ")),
    tWR(tCK * memSpec.memtimingspec.entries.at("WR")),
    tWPRE(tCK * memSpec.memtimingspec.entries.at("WPRE")),
    tWTR(tCK * memSpec.memtimingspec.entries.at("WTR")),
    tXP(tCK * memSpec.memtimingspec.entries.at("XP")),
    tSR(tCK * memSpec.memtimingspec.entries.at("SR")),
    tXSR(tCK * memSpec.memtimingspec.entries.at("XSR")),
    tESCKE(tCK * memSpec.memtimingspec.entries.at("ESCKE")),
    tCKE(tCK * memSpec.memtimingspec.entries.at("CKE")),
    tCMDCKE(tCK * memSpec.memtimingspec.entries.at("CMDCKE")),
    tRTRS(tCK * memSpec.memtimingspec.entries.at("RTRS"))
{
    commandLengthInCycles[Command::ACT] = 4;
    commandLengthInCycles[Command::PREPB] = 2;
    commandLengthInCycles[Command::PREAB] = 2;
    commandLengthInCycles[Command::RD] = 4;
    commandLengthInCycles[Command::RDA] = 4;
    commandLengthInCycles[Command::WR] = 4;
    commandLengthInCycles[Command::MWR] = 4;
    commandLengthInCycles[Command::WRA] = 4;
    commandLengthInCycles[Command::MWRA] = 4;
    commandLengthInCycles[Command::REFAB] = 2;
    commandLengthInCycles[Command::REFPB] = 2;
    commandLengthInCycles[Command::SREFEN] = 2;
    commandLengthInCycles[Command::SREFEX] = 2;

    uint64_t deviceSizeBits =
        static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * bitWidth;
    uint64_t deviceSizeBytes = deviceSizeBits / 8;
    memorySizeBytes = deviceSizeBytes * ranksPerChannel * numberOfChannels;

    std::cout << headline << std::endl;
    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:           "
              << "LPDDR4" << std::endl;
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

sc_time MemSpecLPDDR4::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecLPDDR4::getRefreshIntervalPB() const
{
    return tREFIpb;
}

sc_time MemSpecLPDDR4::getExecutionTime(Command command,
                                        [[maybe_unused]] const tlm_generic_payload& payload) const
{
    if (command == Command::PREPB)
        return tRPpb + tCK;

    if (command == Command::PREAB)
        return tRPab + tCK;

    if (command == Command::ACT)
        return tRCD + 3 * tCK;

    if (command == Command::RD)
        return tRL + tDQSCK + burstDuration + 3 * tCK;

    if (command == Command::RDA)
        return burstDuration + tRTP - 5 * tCK + tRPpb;

    if (command == Command::WR || command == Command::MWR)
        return tWL + tDQSS + tDQS2DQ + burstDuration + 3 * tCK;

    if (command == Command::WRA || command == Command::MWRA)
        return tWL + 4 * tCK + burstDuration + tWR + tRPpb;

    if (command == Command::REFAB)
        return tRFCab + tCK;

    if (command == Command::REFPB)
        return tRFCpb + tCK;

    SC_REPORT_FATAL("getExecutionTime",
                    "command not known or command doesn't have a fixed execution time");
    throw;
}

TimeInterval
MemSpecLPDDR4::getIntervalOnDataStrobe(Command command,
                                       [[maybe_unused]] const tlm_generic_payload& payload) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL + tDQSCK + 3 * tCK, tRL + tDQSCK + burstDuration + 3 * tCK};

    if (command == Command::WR || command == Command::WRA || command == Command::MWR ||
        command == Command::MWRA)
        return {tWL + tDQSS + tDQS2DQ + 3 * tCK, tWL + tDQSS + tDQS2DQ + burstDuration + 3 * tCK};

    SC_REPORT_FATAL("MemSpecLPDDR4", "Method was called with invalid argument");
    throw;
}

bool MemSpecLPDDR4::requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const
{
    return !allBytesEnabled(payload);
}

} // namespace DRAMSys
