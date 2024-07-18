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

#include "MemSpecWideIO.h"

#include "DRAMSys/common/utils.h"

#include <iostream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

MemSpecWideIO::MemSpecWideIO(const Config::MemSpec& memSpec) :
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
    tCKE(tCK * memSpec.memtimingspec.entries.at("CKE")),
    tCKESR(tCK * memSpec.memtimingspec.entries.at("CKESR")),
    tRAS(tCK * memSpec.memtimingspec.entries.at("RAS")),
    tRC(tCK * memSpec.memtimingspec.entries.at("RC")),
    tRCD(tCK * memSpec.memtimingspec.entries.at("RCD")),
    tRL(tCK * memSpec.memtimingspec.entries.at("RL")),
    tWL(tCK * memSpec.memtimingspec.entries.at("WL")),
    tWR(tCK * memSpec.memtimingspec.entries.at("WR")),
    tXP(tCK * memSpec.memtimingspec.entries.at("XP")),
    tXSR(tCK * memSpec.memtimingspec.entries.at("XSR")),
    tREFI(tCK * memSpec.memtimingspec.entries.at("REFI")),
    tRFC(tCK * memSpec.memtimingspec.entries.at("RFC")),
    tRP(tCK * memSpec.memtimingspec.entries.at("RP")),
    tDQSCK(tCK * memSpec.memtimingspec.entries.at("DQSCK")),
    tAC(tCK * memSpec.memtimingspec.entries.at("AC")),
    tCCD_R(tCK * memSpec.memtimingspec.entries.at("CCD_R")),
    tCCD_W(tCK * memSpec.memtimingspec.entries.at("CCD_W")),
    tRRD(tCK * memSpec.memtimingspec.entries.at("RRD")),
    tTAW(tCK * memSpec.memtimingspec.entries.at("TAW")),
    tWTR(tCK * memSpec.memtimingspec.entries.at("WTR")),
    tRTRS(tCK * memSpec.memtimingspec.entries.at("RTRS")),
    iDD0(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd0") : 0),
    iDD2N(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd2n") : 0),
    iDD3N(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd3n") : 0),
    iDD4R(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd4r") : 0),
    iDD4W(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd4w") : 0),
    iDD5(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd5") : 0),
    iDD6(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd6") : 0),
    vDD(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("vdd") : 0),
    iDD02(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd02") : 0),
    iDD2P0(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd2p0")
                                            : 0),
    iDD2P02(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd2p02")
                                             : 0),
    iDD2P1(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd2p1")
                                            : 0),
    iDD2P12(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd2p12")
                                             : 0),
    iDD2N2(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd2n2")
                                            : 0),
    iDD3P0(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd3p0")
                                            : 0),
    iDD3P02(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd3p02")
                                             : 0),
    iDD3P1(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd3p1")
                                            : 0),
    iDD3P12(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd3p12")
                                             : 0),
    iDD3N2(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd3n2")
                                            : 0),
    iDD4R2(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd4r2")
                                            : 0),
    iDD4W2(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd4w2")
                                            : 0),
    iDD52(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd52") : 0),
    iDD62(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("idd62") : 0),
    vDD2(memSpec.mempowerspec.has_value() ? memSpec.mempowerspec.value().entries.at("vdd2") : 0)
{
    uint64_t deviceSizeBits =
        static_cast<uint64_t>(banksPerRank) * rowsPerBank * columnsPerRow * bitWidth;
    uint64_t deviceSizeBytes = deviceSizeBits / 8;
    memorySizeBytes = deviceSizeBytes * ranksPerChannel * numberOfChannels;

    if (!memSpec.mempowerspec.has_value())
        SC_REPORT_FATAL("MemSpec", "No power spec defined!");

    std::cout << headline << std::endl;
    std::cout << "Memory Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:           "
              << "Wide I/O" << std::endl;
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

sc_time MemSpecWideIO::getRefreshIntervalAB() const
{
    return tREFI;
}

// Returns the execution time for commands that have a fixed execution time
sc_time MemSpecWideIO::getExecutionTime(Command command,
                                        [[maybe_unused]] const tlm_generic_payload& payload) const
{
    if (command == Command::PREPB || command == Command::PREAB)
        return tRP;

    if (command == Command::ACT)
        return tRCD;

    if (command == Command::RD)
        return tRL + tAC + burstDuration;

    if (command == Command::RDA)
        return burstDuration + tRP;

    if (command == Command::WR || command == Command::MWR)
        return tWL + burstDuration;

    if (command == Command::WRA || command == Command::MWRA)
        return tWL + burstDuration - tCK + tWR + tRP;

    if (command == Command::REFAB)
        return tRFC;

    SC_REPORT_FATAL("getExecutionTime",
                    "command not known or command doesn't have a fixed execution time");
    throw;
}

TimeInterval
MemSpecWideIO::getIntervalOnDataStrobe(Command command,
                                       [[maybe_unused]] const tlm_generic_payload& payload) const
{
    if (command == Command::RD || command == Command::RDA)
        return {tRL + tAC, tRL + tAC + burstDuration};

    if (command == Command::WR || command == Command::WRA || command == Command::MWR ||
        command == Command::MWRA)
        return {tWL, tWL + burstDuration};

    SC_REPORT_FATAL("MemSpec", "Method was called with invalid argument");
    throw;
}

bool MemSpecWideIO::requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const
{
    return !allBytesEnabled(payload);
}

#ifdef DRAMPOWER
DRAMPower::MemorySpecification MemSpecWideIO::toDramPowerMemSpec() const
{
    DRAMPower::MemArchitectureSpec memArchSpec;
    memArchSpec.burstLength = defaultBurstLength;
    memArchSpec.dataRate = dataRate;
    memArchSpec.nbrOfRows = rowsPerBank;
    memArchSpec.nbrOfBanks = banksPerChannel;
    memArchSpec.nbrOfColumns = columnsPerRow;
    memArchSpec.nbrOfRanks = ranksPerChannel;
    memArchSpec.width = bitWidth;
    memArchSpec.nbrOfBankGroups = bankGroupsPerChannel;
    memArchSpec.twoVoltageDomains = true;
    memArchSpec.dll = false;

    DRAMPower::MemTimingSpec memTimingSpec;
    // FIXME: memTimingSpec.FAWB   = tTAW / tCK;
    // FIXME: memTimingSpec.RASB   = tRAS / tCK;
    // FIXME: memTimingSpec.RCB    = tRC / tCK;
    // FIXME: memTimingSpec.RPB    = tRP / tCK;
    // FIXME: memTimingSpec.RRDB   = tRRD / tCK;
    // FIXME: memTimingSpec.RRDB_L = tRRD / tCK;
    // FIXME: memTimingSpec.RRDB_S = tRRD / tCK;
    memTimingSpec.AL = 0;
    memTimingSpec.CCD = defaultBurstLength;
    memTimingSpec.CCD_L = defaultBurstLength;
    memTimingSpec.CCD_S = defaultBurstLength;
    memTimingSpec.CKE = tCKE / tCK;
    memTimingSpec.CKESR = tCKESR / tCK;
    // See also MemTimingSpec.cc in DRAMPower
    memTimingSpec.clkMhz = 1 / (tCK.to_seconds() * 1'000'000);
    memTimingSpec.clkPeriod = tCK.to_seconds() * 1'000'000'000;
    memTimingSpec.DQSCK = tDQSCK / tCK;
    memTimingSpec.FAW = tTAW / tCK;
    memTimingSpec.RAS = tRAS / tCK;
    memTimingSpec.RC = tRC / tCK;
    memTimingSpec.RCD = tRCD / tCK;
    memTimingSpec.REFI = tREFI / tCK;
    memTimingSpec.RFC = tRFC / tCK;
    memTimingSpec.RL = tRL / tCK;
    memTimingSpec.RP = tRP / tCK;
    memTimingSpec.RRD = tRRD / tCK;
    memTimingSpec.RRD_L = tRRD / tCK;
    memTimingSpec.RRD_S = tRRD / tCK;
    memTimingSpec.RTP = defaultBurstLength;
    memTimingSpec.TAW = tTAW / tCK;
    memTimingSpec.WL = tWL / tCK;
    memTimingSpec.WR = tWR / tCK;
    memTimingSpec.WTR = tWTR / tCK;
    memTimingSpec.WTR_L = tWTR / tCK;
    memTimingSpec.WTR_S = tWTR / tCK;
    memTimingSpec.XP = tXP / tCK;
    memTimingSpec.XPDLL = tXP / tCK;
    memTimingSpec.XS = tXSR / tCK;
    memTimingSpec.XSDLL = tXSR / tCK;

    DRAMPower::MemPowerSpec memPowerSpec;
    memPowerSpec.idd0 = iDD0;
    memPowerSpec.idd02 = iDD02;
    memPowerSpec.idd2p0 = iDD2P0;
    memPowerSpec.idd2p02 = iDD2P02;
    memPowerSpec.idd2p1 = iDD2P1;
    memPowerSpec.idd2p12 = iDD2P12;
    memPowerSpec.idd2n = iDD2N;
    memPowerSpec.idd2n2 = iDD2N2;
    memPowerSpec.idd3p0 = iDD3P0;
    memPowerSpec.idd3p02 = iDD3P02;
    memPowerSpec.idd3p1 = iDD3P1;
    memPowerSpec.idd3p12 = iDD3P12;
    memPowerSpec.idd3n = iDD3N;
    memPowerSpec.idd3n2 = iDD3N2;
    memPowerSpec.idd4r = iDD4R;
    memPowerSpec.idd4r2 = iDD4R2;
    memPowerSpec.idd4w = iDD4W;
    memPowerSpec.idd4w2 = iDD4W2;
    memPowerSpec.idd5 = iDD5;
    memPowerSpec.idd52 = iDD52;
    memPowerSpec.idd6 = iDD6;
    memPowerSpec.idd62 = iDD62;
    memPowerSpec.vdd = vDD;
    memPowerSpec.vdd2 = vDD2;

    DRAMPower::MemorySpecification powerSpec;
    powerSpec.id = memoryId;
    powerSpec.memoryType = DRAMPower::MemoryType::WIDEIO_SDR;
    powerSpec.memTimingSpec = memTimingSpec;
    powerSpec.memPowerSpec = memPowerSpec;
    powerSpec.memArchSpec = memArchSpec;

    return powerSpec;
}
#endif

} // namespace DRAMSys
