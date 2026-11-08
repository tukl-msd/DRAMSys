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

#include "MemSpecLPDDR5.h"

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/common/utils.h"

#include <iostream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

namespace
{

MemSpecLPDDR5::BankArchitecture
bankArchitectureOf(const DRAMUtils::MemSpec::MemArchitectureSpecTypeLPDDR5& arch)
{
    if (arch.nbrOfBanks == 16 && arch.nbrOfBankGroups == 4)
        return MemSpecLPDDR5::BankArchitecture::BG;
    if (arch.nbrOfBanks == 16 && arch.nbrOfBankGroups <= 1)
        return MemSpecLPDDR5::BankArchitecture::B16;
    if (arch.nbrOfBanks == 8 && arch.nbrOfBankGroups <= 1)
        return MemSpecLPDDR5::BankArchitecture::B8;

    SC_REPORT_FATAL("MemSpecLPDDR5",
                    "Invalid bank architecture, valid banks|bank groups are 16|4 (BG mode), "
                    "16|1 (16B mode) or 8|1 (8B mode)");
    throw;
}

uint64_t groupsOf(const DRAMUtils::MemSpec::MemArchitectureSpecTypeLPDDR5& arch)
{
    return arch.nbrOfBankGroups == 0 ? 1 : arch.nbrOfBankGroups;
}

} // namespace

MemSpecLPDDR5::MemSpecLPDDR5(const DRAMUtils::MemSpec::MemSpecLPDDR5& memSpec) :
    MemSpec(memSpec,
            memSpec.memarchitecturespec.nbrOfChannels,
            memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfBanks,
            groupsOf(memSpec.memarchitecturespec),
            memSpec.memarchitecturespec.nbrOfBanks / groupsOf(memSpec.memarchitecturespec),
            memSpec.memarchitecturespec.nbrOfBanks * memSpec.memarchitecturespec.nbrOfRanks,
            groupsOf(memSpec.memarchitecturespec) * memSpec.memarchitecturespec.nbrOfRanks,
            memSpec.memarchitecturespec.nbrOfDevices),
    bankArchitecture(bankArchitectureOf(memSpec.memarchitecturespec)),
    wckToCkRatio(memSpec.memtimingspec.WCK2CK),
    per2BankOffset(static_cast<unsigned>(memSpec.memarchitecturespec.per2BankOffset)),
    tREFI(tCK * memSpec.memtimingspec.REFI),
    tREFIpb(tCK * memSpec.memtimingspec.REFIpb),
    tRFCab(tCK * memSpec.memtimingspec.RFCab),
    tRFCpb(tCK * memSpec.memtimingspec.RFCpb),
    tRAS(tCK * memSpec.memtimingspec.RAS),
    tRPab(tCK * memSpec.memtimingspec.RPab),
    tRPpb(tCK * memSpec.memtimingspec.RPpb),
    tRCpb(tCK * memSpec.memtimingspec.RCpb),
    tRCab(tCK * memSpec.memtimingspec.RCab),
    tPPD(tCK * memSpec.memtimingspec.PPD),
    tRCDRD(tCK * memSpec.memtimingspec.RCD_L),
    tRCDWR(tCK * memSpec.memtimingspec.RCD_S),
    tFAW(tCK * memSpec.memtimingspec.FAW),
    tRRD(tCK * memSpec.memtimingspec.RRD),
    tRL(tCK * memSpec.memtimingspec.RL),
    tRBTP(tCK * memSpec.memtimingspec.RBTP),
    tWL(tCK * memSpec.memtimingspec.WL),
    tWR(tCK * memSpec.memtimingspec.WR),
    tRTRS(tCK * memSpec.memtimingspec.RTRS),
    tBURST16(tCK * memSpec.memtimingspec.BL_n_min_16),
    tBURSTMAX16(tCK * memSpec.memtimingspec.BL_n_max_16),
    tCCD_L16(tCK * memSpec.memtimingspec.BL_n_L_16),
    tCCD_S16(tCK * memSpec.memtimingspec.BL_n_S_16),
    tBURST32(tCK * memSpec.memtimingspec.BL_n_min_32),
    tBURSTMAX32(tCK * memSpec.memtimingspec.BL_n_max_32),
    tCCD_L32(tCK * memSpec.memtimingspec.BL_n_L_32),
    tCCD_S32(tCK * memSpec.memtimingspec.BL_n_S_32),
    tWTR_L(tCK * memSpec.memtimingspec.WTR_L),
    tWTR_S(tCK * memSpec.memtimingspec.WTR_S),
    tRTW_L16(tCK * memSpec.memtimingspec.RTW_L_16),
    tRTW_S16(tCK * memSpec.memtimingspec.RTW_S_16),
    tRTW_L32(tCK * memSpec.memtimingspec.RTW_L_32),
    tRTW_S32(tCK * memSpec.memtimingspec.RTW_S_32),
    tWCK2DQO(tCK * memSpec.memtimingspec.WCK2DQO),
    tpbR2act(tCK * memSpec.memtimingspec.pbR2act),
    tpbR2pbR(tCK * memSpec.memtimingspec.pbR2pbR),
    tAAD(tCK * 8) // ACT-1 to ACT-2 maximum, fixed 8 nCK (not in the DRAMUtils LPDDR5 schema)
{
    // dataRate in the memspec is per WCK cycle; DRAMSys counts beats per CK cycle
    dataRate = memSpec.memarchitecturespec.dataRate * wckToCkRatio;
    burstDuration = tCK * (static_cast<double>(defaultBurstLength) / static_cast<double>(dataRate));

    // tRCD differs for READ and WRITE on LPDDR5X (JESD209-5C Table 396); LPDDR5 has one tRCD
    if (memSpec.memtimingspec.RCD_S > memSpec.memtimingspec.RCD_L)
        SC_REPORT_FATAL("MemSpecLPDDR5", "RCD_S (ACT-2 to WRITE) must not exceed RCD_L (ACT-2 to READ)");

    if (bankArchitecture == BankArchitecture::BG && maxBurstLength == 32)
        SC_REPORT_FATAL("MemSpecLPDDR5", "BL32 in BG mode is not supported");

    if (bankArchitecture == BankArchitecture::B8 &&
        (defaultBurstLength != 32 || maxBurstLength != 32))
        SC_REPORT_FATAL("MemSpecLPDDR5", "8B mode supports BL32 only");

    if (ranksPerChannel > 1 && memSpec.memtimingspec.RTRS == 0)
        SC_REPORT_FATAL("MemSpecLPDDR5", "Multi-rank configurations need RTRS > 0");

    if (!memSpec.memarchitecturespec.WCKalwaysOn)
        SC_REPORT_FATAL("MemSpecLPDDR5",
                        "Only WCK always-on mode is supported (no per-access CAS WCK2CK sync)");

    // ACT-1 occupies one CA cycle; CheckerLPDDR5 places ACT-2 in a free CA cycle within tAAD
    commandLengthInCycles[Command::ACT] = 1;

    deviceSizeBits =
        static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * bitWidth;
    deviceSizeBytes = deviceSizeBits / 8;
    memorySizeBytes = deviceSizeBytes * devicesPerRank * ranksPerChannel * numberOfChannels;
}

void MemSpecLPDDR5::print() const
{
    std::string architecture = "BG";
    if (bankArchitecture == BankArchitecture::B16)
        architecture = "16B";
    else if (bankArchitecture == BankArchitecture::B8)
        architecture = "8B";

    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:           " << "LPDDR5" << std::endl;
    std::cout << " Bank architecture:     " << architecture << std::endl;
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
    std::cout << " WCK:CK ratio:          " << wckToCkRatio << std::endl;
    std::cout << std::endl;
}

sc_time MemSpecLPDDR5::getRefreshIntervalAB() const
{
    return tREFI;
}

sc_time MemSpecLPDDR5::getRefreshIntervalPB() const
{
    // 8B mode refreshes one bank per REFpb
    if (bankArchitecture != BankArchitecture::B8)
        SC_REPORT_FATAL("MemSpecLPDDR5", "BG and 16B modes refresh bank pairs, use RefreshPolicy Per2Bank");
    return tREFIpb;
}

sc_time MemSpecLPDDR5::getRefreshIntervalP2B() const
{
    // BG and 16B modes refresh a bank pair (bank and bank + per2BankOffset) per REFpb
    if (bankArchitecture == BankArchitecture::B8)
        SC_REPORT_FATAL("MemSpecLPDDR5", "8B mode refreshes single banks, use RefreshPolicy PerBank");
    return tREFIpb;
}

unsigned MemSpecLPDDR5::getPer2BankOffset() const
{
    return per2BankOffset;
}

sc_time MemSpecLPDDR5::getBurstDuration(const tlm_generic_payload& payload) const
{
    return ControllerExtension::getBurstLength(payload) == 32 ? tBURST32 : tBURST16;
}

sc_time MemSpecLPDDR5::getExecutionTime(Command command, const tlm_generic_payload& payload) const
{
    if (command == Command::PREPB)
        return tRPpb;

    if (command == Command::PREAB)
        return tRPab;

    if (command == Command::ACT)
        return tRCDRD;

    if (command == Command::RD)
        return tRL + tWCK2DQO + getBurstDuration(payload);

    if (command == Command::RDA)
        return getBurstDuration(payload) + tRBTP + tRPpb;

    if (command == Command::WR)
        return tWL + getBurstDuration(payload);

    if (command == Command::WRA)
        return tWL + getBurstDuration(payload) + tCK + tWR + tRPpb;

    if (command == Command::REFAB)
        return tRFCab;

    if (command == Command::REFPB || command == Command::REFP2B)
        return tRFCpb;

    SC_REPORT_FATAL("getExecutionTime",
                    "command not known or command doesn't have a fixed execution time");
    throw;
}

TimeInterval MemSpecLPDDR5::getIntervalOnDataStrobe(Command command,
                                                    const tlm_generic_payload& payload) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL + tWCK2DQO, tRL + tWCK2DQO + getBurstDuration(payload)};

    if (command == Command::WR || command == Command::WRA)
        return {tWL, tWL + getBurstDuration(payload)};

    SC_REPORT_FATAL("MemSpecLPDDR5", "Method was called with invalid argument");
    throw;
}

} // namespace DRAMSys
