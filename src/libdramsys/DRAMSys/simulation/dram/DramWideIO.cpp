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
 */

#include "DramWideIO.h"

#include "DRAMSys/configuration/memspec/MemSpecWideIO.h"
#ifdef DRAMPOWER
#include "LibDRAMPower.h"
using namespace DRAMPower;
#endif

#include <memory>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

DramWideIO::DramWideIO(const sc_module_name& name, const Configuration& config) : Dram(name, config)
{
#ifdef DRAMPOWER
    if (powerAnalysis)
    {
        const auto* memSpecWideIO = dynamic_cast<const MemSpecWideIO*>(config.memSpec.get());
        if (memSpecWideIO == nullptr)
            SC_REPORT_FATAL("DramWideIO", "Wrong MemSpec chosen");

        MemArchitectureSpec memArchSpec;
        memArchSpec.burstLength = memSpecWideIO->defaultBurstLength;
        memArchSpec.dataRate = memSpecWideIO->dataRate;
        memArchSpec.nbrOfRows = memSpecWideIO->rowsPerBank;
        memArchSpec.nbrOfBanks = memSpecWideIO->banksPerChannel;
        memArchSpec.nbrOfColumns = memSpecWideIO->columnsPerRow;
        memArchSpec.nbrOfRanks = memSpecWideIO->ranksPerChannel;
        memArchSpec.width = memSpecWideIO->bitWidth;
        memArchSpec.nbrOfBankGroups = memSpecWideIO->bankGroupsPerChannel;
        memArchSpec.twoVoltageDomains = true;
        memArchSpec.dll = false;

        MemTimingSpec memTimingSpec;
        // FIXME: memTimingSpec.FAWB   = memSpecWideIO->tTAW / memSpecWideIO->tCK;
        // FIXME: memTimingSpec.RASB   = memSpecWideIO->tRAS / memSpecWideIO->tCK;
        // FIXME: memTimingSpec.RCB    = memSpecWideIO->tRC / memSpecWideIO->tCK;
        // FIXME: memTimingSpec.RPB    = memSpecWideIO->tRP / memSpecWideIO->tCK;
        // FIXME: memTimingSpec.RRDB   = memSpecWideIO->tRRD / memSpecWideIO->tCK;
        // FIXME: memTimingSpec.RRDB_L = memSpecWideIO->tRRD / memSpecWideIO->tCK;
        // FIXME: memTimingSpec.RRDB_S = memSpecWideIO->tRRD / memSpecWideIO->tCK;
        memTimingSpec.AL = 0;
        memTimingSpec.CCD = memSpecWideIO->defaultBurstLength;
        memTimingSpec.CCD_L = memSpecWideIO->defaultBurstLength;
        memTimingSpec.CCD_S = memSpecWideIO->defaultBurstLength;
        memTimingSpec.CKE = memSpecWideIO->tCKE / memSpecWideIO->tCK;
        memTimingSpec.CKESR = memSpecWideIO->tCKESR / memSpecWideIO->tCK;
        memTimingSpec.clkMhz = memSpecWideIO->fCKMHz;
        // See also MemTimingSpec.cc in DRAMPower
        memTimingSpec.clkPeriod = 1000.0 / memSpecWideIO->fCKMHz;
        memTimingSpec.DQSCK = memSpecWideIO->tDQSCK / memSpecWideIO->tCK;
        memTimingSpec.FAW = memSpecWideIO->tTAW / memSpecWideIO->tCK;
        memTimingSpec.RAS = memSpecWideIO->tRAS / memSpecWideIO->tCK;
        memTimingSpec.RC = memSpecWideIO->tRC / memSpecWideIO->tCK;
        memTimingSpec.RCD = memSpecWideIO->tRCD / memSpecWideIO->tCK;
        memTimingSpec.REFI = memSpecWideIO->tREFI / memSpecWideIO->tCK;
        memTimingSpec.RFC = memSpecWideIO->tRFC / memSpecWideIO->tCK;
        memTimingSpec.RL = memSpecWideIO->tRL / memSpecWideIO->tCK;
        memTimingSpec.RP = memSpecWideIO->tRP / memSpecWideIO->tCK;
        memTimingSpec.RRD = memSpecWideIO->tRRD / memSpecWideIO->tCK;
        memTimingSpec.RRD_L = memSpecWideIO->tRRD / memSpecWideIO->tCK;
        memTimingSpec.RRD_S = memSpecWideIO->tRRD / memSpecWideIO->tCK;
        memTimingSpec.RTP = memSpecWideIO->defaultBurstLength;
        memTimingSpec.TAW = memSpecWideIO->tTAW / memSpecWideIO->tCK;
        memTimingSpec.WL = memSpecWideIO->tWL / memSpecWideIO->tCK;
        memTimingSpec.WR = memSpecWideIO->tWR / memSpecWideIO->tCK;
        memTimingSpec.WTR = memSpecWideIO->tWTR / memSpecWideIO->tCK;
        memTimingSpec.WTR_L = memSpecWideIO->tWTR / memSpecWideIO->tCK;
        memTimingSpec.WTR_S = memSpecWideIO->tWTR / memSpecWideIO->tCK;
        memTimingSpec.XP = memSpecWideIO->tXP / memSpecWideIO->tCK;
        memTimingSpec.XPDLL = memSpecWideIO->tXP / memSpecWideIO->tCK;
        memTimingSpec.XS = memSpecWideIO->tXSR / memSpecWideIO->tCK;
        memTimingSpec.XSDLL = memSpecWideIO->tXSR / memSpecWideIO->tCK;

        MemPowerSpec memPowerSpec;
        memPowerSpec.idd0 = memSpecWideIO->iDD0;
        memPowerSpec.idd02 = memSpecWideIO->iDD02;
        memPowerSpec.idd2p0 = memSpecWideIO->iDD2P0;
        memPowerSpec.idd2p02 = memSpecWideIO->iDD2P02;
        memPowerSpec.idd2p1 = memSpecWideIO->iDD2P1;
        memPowerSpec.idd2p12 = memSpecWideIO->iDD2P12;
        memPowerSpec.idd2n = memSpecWideIO->iDD2N;
        memPowerSpec.idd2n2 = memSpecWideIO->iDD2N2;
        memPowerSpec.idd3p0 = memSpecWideIO->iDD3P0;
        memPowerSpec.idd3p02 = memSpecWideIO->iDD3P02;
        memPowerSpec.idd3p1 = memSpecWideIO->iDD3P1;
        memPowerSpec.idd3p12 = memSpecWideIO->iDD3P12;
        memPowerSpec.idd3n = memSpecWideIO->iDD3N;
        memPowerSpec.idd3n2 = memSpecWideIO->iDD3N2;
        memPowerSpec.idd4r = memSpecWideIO->iDD4R;
        memPowerSpec.idd4r2 = memSpecWideIO->iDD4R2;
        memPowerSpec.idd4w = memSpecWideIO->iDD4W;
        memPowerSpec.idd4w2 = memSpecWideIO->iDD4W2;
        memPowerSpec.idd5 = memSpecWideIO->iDD5;
        memPowerSpec.idd52 = memSpecWideIO->iDD52;
        memPowerSpec.idd6 = memSpecWideIO->iDD6;
        memPowerSpec.idd62 = memSpecWideIO->iDD62;
        memPowerSpec.vdd = memSpecWideIO->vDD;
        memPowerSpec.vdd2 = memSpecWideIO->vDD2;

        MemorySpecification powerSpec;
        powerSpec.id = memSpecWideIO->memoryId;
        powerSpec.memoryType = MemoryType::WIDEIO_SDR;
        powerSpec.memTimingSpec = memTimingSpec;
        powerSpec.memPowerSpec = memPowerSpec;
        powerSpec.memArchSpec = memArchSpec;

        DRAMPower = std::make_unique<libDRAMPower>(powerSpec, false);
    }
#endif
}

} // namespace DRAMSys
