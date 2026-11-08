/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 *    Martinius
 */

#include "MemSpecHBM3_4.h"

#include "DRAMSys/common/utils.h"

#include <iostream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

MemSpecHBM3_4::MemSpecHBM3_4(const DRAMUtils::MemSpec::MemSpecHBM3& memSpec) :
    MemSpecHBM3_4(memSpec, DRAMUtils::MemSpec::MemSpecHBM3::id)
{
}

MemSpecHBM3_4::MemSpecHBM3_4(const DRAMUtils::MemSpec::MemSpecHBM4& memSpec) :
    MemSpecHBM3_4(memSpec, DRAMUtils::MemSpec::MemSpecHBM4::id)
{
}

template <typename MemSpecType>
MemSpecHBM3_4::MemSpecHBM3_4(const MemSpecType& memSpec, std::string_view memoryType) :
    MemSpec(memSpec,
            memSpec.memarchitecturespec.nbrOfChannels,
            memSpec.memarchitecturespec.nbrOfPseudoChannels,
            memSpec.memarchitecturespec.nbrOfBanks,
            memSpec.memarchitecturespec.nbrOfBankGroups,
            memSpec.memarchitecturespec.nbrOfBanks /
                memSpec.memarchitecturespec.nbrOfBankGroups,
            memSpec.memarchitecturespec.nbrOfBanks * memSpec.memarchitecturespec.nbrOfPseudoChannels,
            memSpec.memarchitecturespec.nbrOfBankGroups * memSpec.memarchitecturespec.nbrOfPseudoChannels,
            memSpec.memarchitecturespec.nbrOfDevices),
    memoryType(memoryType),
    banksPerStack(memSpec.memarchitecturespec.nbrOfBanks / memSpec.memarchitecturespec.nbrOfStacks),
    tDQSCK(tCK * memSpec.memtimingspec.DQSCK),
    tRC(tCK * memSpec.memtimingspec.RC),
    tRAS(tCK * memSpec.memtimingspec.RAS),
    tRCDRD(tCK * memSpec.memtimingspec.RCDRD),
    tRCDWR(tCK * memSpec.memtimingspec.RCDWR),
    tRRDL(tCK * memSpec.memtimingspec.RRDL),
    tRRDS(tCK * memSpec.memtimingspec.RRDS),
    tFAW(tCK * memSpec.memtimingspec.FAW),
    tRTP(tCK * memSpec.memtimingspec.RTP),
    tRP(tCK * memSpec.memtimingspec.RP),
    tRL(tCK * memSpec.memtimingspec.RL),
    tWL(tCK * memSpec.memtimingspec.WL),
    tPL(tCK * memSpec.memtimingspec.PL),
    tWR(tCK * memSpec.memtimingspec.WR),
    tCCDL(tCK * memSpec.memtimingspec.CCDL),
    tCCDS(tCK * memSpec.memtimingspec.CCDS),
    tCCDR(tCK * memSpec.memtimingspec.CCDR),
    tWTRL(tCK * memSpec.memtimingspec.WTRL),
    tWTRS(tCK * memSpec.memtimingspec.WTRS),
    tRTW(tCK * memSpec.memtimingspec.RTW),
    tXP(tCK * memSpec.memtimingspec.XP),
    tCKE(tCK * memSpec.memtimingspec.CKE),
    tPD(tCKE),
    tCKESR(tCKE),
    tXS(tCK * memSpec.memtimingspec.XS),
    tRFC(tCK * memSpec.memtimingspec.RFC),
    tRFCPB(tCK * memSpec.memtimingspec.RFCPB),
    tRREFD(tCK * memSpec.memtimingspec.RREFD),
    tREFI(tCK * memSpec.memtimingspec.REFI),
    tREFIPB(tCK * memSpec.memtimingspec.REFIPB),
    tPPD(tCK * memSpec.memtimingspec.PPD)
{
    stacksPerChannel = memSpec.memarchitecturespec.nbrOfStacks;

    if (memSpec.memarchitecturespec.nbrOfBanks % memSpec.memarchitecturespec.nbrOfStacks != 0 ||
        memSpec.memarchitecturespec.nbrOfBankGroups % memSpec.memarchitecturespec.nbrOfStacks != 0)
        SC_REPORT_FATAL("MemSpecHBM3_4", "Banks and bank groups must divide evenly into stack IDs");

    // Row command bus slots in CK cycles (row commands truth table): ACT takes R, F, R; PREpb, PREab,
    // REFpb, REFab and PDX/SRX one edge; PDE and SRE R, F. CheckerHBM3_4 lets only PREpb/PREab start
    // on a falling edge.
    commandLengthInCycles[Command::ACT] = 1.5;
    commandLengthInCycles[Command::PREPB] = 0.5;
    commandLengthInCycles[Command::PREAB] = 0.5;
    commandLengthInCycles[Command::REFPB] = 0.5;
    commandLengthInCycles[Command::REFAB] = 0.5;
    commandLengthInCycles[Command::PDXA] = 0.5;
    commandLengthInCycles[Command::PDXP] = 0.5;
    commandLengthInCycles[Command::SREFEX] = 0.5;

    deviceSizeBytes =
        static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * defaultDataBytesPerBurst;
    deviceSizeBits = deviceSizeBytes * 8;
    memorySizeBytes = deviceSizeBytes * ranksPerChannel * numberOfChannels;
}

void MemSpecHBM3_4::print() const
{
    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:                    " << memoryType << std::endl;
    std::cout << " Memory size in bytes:           " << memorySizeBytes << std::endl;
    std::cout << " Channels:                       " << numberOfChannels << std::endl;
    std::cout << " Pseudo channels per channel:    " << ranksPerChannel << std::endl;
    std::cout << " Total bank groups per channel:  " << bankGroupsPerChannel << std::endl;
    std::cout << " Total banks per channel:        " << banksPerChannel << std::endl;
    std::cout << " Stack IDs (SID):                " << stacksPerChannel << std::endl;
    std::cout << " Bank groups per pseudo channel: " << groupsPerRank << std::endl;
    std::cout << " Banks per pseudo channel:       " << banksPerRank << std::endl;
    std::cout << " Rows per bank:                  " << rowsPerBank << std::endl;
    std::cout << " Columns per row:                " << columnsPerRow << std::endl;
    std::cout << " Pseudo channel width in bits:   " << bitWidth << std::endl;
    std::cout << " Pseudo channel size in bits:    " << deviceSizeBits << std::endl;
    std::cout << " Pseudo channel size in bytes:   " << deviceSizeBytes << std::endl;
    std::cout << std::endl;
}

sc_time MemSpecHBM3_4::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecHBM3_4::getRefreshIntervalPB() const
{
    return tREFIPB;
}

bool MemSpecHBM3_4::hasRasAndCasBus() const
{
    return true;
}

bool MemSpecHBM3_4::pseudoChannelMode() const
{
    return ranksPerChannel != 1;
}

sc_time MemSpecHBM3_4::getExecutionTime(Command command, const tlm_generic_payload& payload) const
{
    if (command == Command::PREPB || command == Command::PREAB)
        return tRP;

    if (command == Command::ACT)
    {
        if (payload.get_command() == TLM_READ_COMMAND)
            return tRCDRD + tCK;

        return tRCDWR + tCK;
    }

    if (command == Command::RD)
        return tRL + tPL + tDQSCK + burstDuration;

    if (command == Command::RDA)
        return tRTP + tRP;

    if (command == Command::WR)
        return tWL + tPL + burstDuration;

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
MemSpecHBM3_4::getIntervalOnDataStrobe(Command command,
                                       [[maybe_unused]] const tlm_generic_payload& payload) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL + tPL + tDQSCK, tRL + tPL + tDQSCK + burstDuration};

    if (command == Command::WR || command == Command::WRA)
        return {tWL + tPL, tWL + tPL + burstDuration};

    SC_REPORT_FATAL("MemSpecHBM3_4", "Method was called with invalid argument");
    throw;
}

} // namespace DRAMSys
