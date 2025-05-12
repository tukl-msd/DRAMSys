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

#include "MemSpecDDR4.h"

#include "DRAMSys/common/utils.h"

#include <DRAMPower/standards/ddr4/DDR4.h>

#include <iostream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

using DRAMUtils::MemSpec::RefModeTypeDDR4;

MemSpecDDR4::MemSpecDDR4(const DRAMUtils::MemSpec::MemSpecDDR4& memSpec) :
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
    memSpec(memSpec),
    tCKE(tCK * memSpec.memtimingspec.CKE),
    tPD(tCKE),
    tCKESR(tCK * memSpec.memtimingspec.CKESR),
    tRAS(tCK * memSpec.memtimingspec.RAS),
    tRC(tCK * memSpec.memtimingspec.RC),
    tRCD(tCK * memSpec.memtimingspec.RCD),
    tRL(tCK * memSpec.memtimingspec.RL),
    tRPRE(tCK * memSpec.memtimingspec.RPRE),
    tRTP(tCK * memSpec.memtimingspec.RTP),
    tWL(tCK * memSpec.memtimingspec.WL),
    tWPRE(tCK * memSpec.memtimingspec.WPRE),
    tWR(tCK * memSpec.memtimingspec.WR),
    tXP(tCK * memSpec.memtimingspec.XP),
    tXS(tCK * memSpec.memtimingspec.XS),
    tREFI((memSpec.memarchitecturespec.RefMode == RefModeTypeDDR4::REF_MODE_4) ?
        (tCK * (static_cast<double>(memSpec.memtimingspec.REFI) / 4))
            : ((memSpec.memarchitecturespec.RefMode == RefModeTypeDDR4::REF_MODE_2) ?
        (tCK * (static_cast<double>(memSpec.memtimingspec.REFI) / 2))
            // RefModeTypeDDR4::REF_MODE_1 || RefModeTypeDDR4::INVALID
            : (tCK * memSpec.memtimingspec.REFI))),
    tRFC((memSpec.memarchitecturespec.RefMode == RefModeTypeDDR4::REF_MODE_4) ?
        (tCK * memSpec.memtimingspec.RFC4)
            : ((memSpec.memarchitecturespec.RefMode == RefModeTypeDDR4::REF_MODE_2) ?
        (tCK * memSpec.memtimingspec.RFC2)
            // RefModeTypeDDR4::REF_MODE_1 || RefModeTypeDDR4::INVALID
            : (tCK * memSpec.memtimingspec.RFC1))),
    tRP(tCK * memSpec.memtimingspec.RP),
    tDQSCK(tCK * memSpec.memtimingspec.DQSCK),
    tCCD_S(tCK * memSpec.memtimingspec.CCD_S),
    tCCD_L(tCK * memSpec.memtimingspec.CCD_L),
    tFAW(tCK * memSpec.memtimingspec.FAW),
    tRRD_S(tCK * memSpec.memtimingspec.RRD_S),
    tRRD_L(tCK * memSpec.memtimingspec.RRD_L),
    tWTR_S(tCK * memSpec.memtimingspec.WTR_S),
    tWTR_L(tCK * memSpec.memtimingspec.WTR_L),
    tAL(tCK * memSpec.memtimingspec.AL),
    tXPDLL(tCK * memSpec.memtimingspec.XPDLL),
    tXSDLL(tCK * memSpec.memtimingspec.XSDLL),
    tACTPDEN(tCK * memSpec.memtimingspec.ACTPDEN),
    tPRPDEN(tCK * memSpec.memtimingspec.PRPDEN),
    tREFPDEN(tCK * memSpec.memtimingspec.REFPDEN),
    tRTRS(tCK * memSpec.memtimingspec.RTRS)
{
    if (RefModeTypeDDR4::INVALID == memSpec.memarchitecturespec.RefMode)
        SC_REPORT_FATAL("MemSpecDDR4",
                        "Invalid refresh mode! "
                        "Set 1 for normal (fixed 1x), 2 for fixed 2x or 4 for fixed 4x refresh mode.");

    uint64_t deviceSizeBits =
        static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * bitWidth;
    uint64_t deviceSizeBytes = deviceSizeBits / 8;
    memorySizeBytes = deviceSizeBytes * devicesPerRank * ranksPerChannel * numberOfChannels;

    std::cout << headline << std::endl;
    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:           "
              << "DDR4" << std::endl;
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

sc_time MemSpecDDR4::getRefreshIntervalAB() const
{
    return tREFI;
}

// Returns the execution time for commands that have a fixed execution time
sc_time MemSpecDDR4::getExecutionTime(Command command,
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

    if (command == Command::REFAB)
        return tRFC;

    SC_REPORT_FATAL("getExecutionTime",
                    "command not known or command doesn't have a fixed execution time");
    throw;
}

TimeInterval
MemSpecDDR4::getIntervalOnDataStrobe(Command command,
                                     [[maybe_unused]] const tlm::tlm_generic_payload& payload) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL, tRL + burstDuration};

    if (command == Command::WR || command == Command::WRA || command == Command::MWR ||
        command == Command::MWRA)
        return {tWL, tWL + burstDuration};

    SC_REPORT_FATAL("MemSpec", "Method was called with invalid argument");
    throw;
}

bool MemSpecDDR4::requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const
{
    return !allBytesEnabled(payload);
}

std::unique_ptr<DRAMPower::dram_base<DRAMPower::CmdType>> MemSpecDDR4::toDramPowerObject() const
{
    return std::make_unique<DRAMPower::DDR4>(DRAMPower::MemSpecDDR4(memSpec));
}

} // namespace DRAMSys
