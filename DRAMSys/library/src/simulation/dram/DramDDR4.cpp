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

#include "DramDDR4.h"

#include <memory>
#include "../../configuration/Configuration.h"
#include "../../configuration/memspec/MemSpecDDR4.h"

#ifdef DRAMPOWER
#include "../../common/third_party/DRAMPower/src/libdrampower/LibDRAMPower.h"
using namespace DRAMPower;
#endif

using namespace sc_core;

DramDDR4::DramDDR4(const sc_module_name& name, const Configuration& config,
                   TemperatureController& temperatureController)
    : Dram(name, config)
{
    if (storeMode == Configuration::StoreMode::ErrorModel)
        SC_REPORT_FATAL("DramDDR4", "Error Model not supported for DDR4");

    if (powerAnalysis)
    {
        const auto *memSpecDDR4 = dynamic_cast<const MemSpecDDR4*>(config.memSpec.get());
        if (memSpecDDR4 == nullptr)
            SC_REPORT_FATAL("DramDDR4", "Wrong MemSpec chosen");

#ifdef DRAMPOWER
        MemArchitectureSpec memArchSpec;
        memArchSpec.burstLength       = memSpecDDR4->defaultBurstLength;
        memArchSpec.dataRate          = memSpecDDR4->dataRate;
        memArchSpec.nbrOfRows         = memSpecDDR4->rowsPerBank;
        memArchSpec.nbrOfBanks        = memSpecDDR4->banksPerChannel;
        memArchSpec.nbrOfColumns      = memSpecDDR4->columnsPerRow;
        memArchSpec.nbrOfRanks        = memSpecDDR4->ranksPerChannel;
        memArchSpec.width             = memSpecDDR4->bitWidth;
        memArchSpec.nbrOfBankGroups   = memSpecDDR4->bankGroupsPerChannel;
        memArchSpec.twoVoltageDomains = true;
        memArchSpec.dll               = true;

        MemTimingSpec memTimingSpec;
        //FIXME: memTimingSpec.FAWB   = memSpecDDR4->tFAW / memSpecDDR4->tCK;
        //FIXME: memTimingSpec.RASB   = memSpecDDR4->tRAS / memSpecDDR4->tCK;
        //FIXME: memTimingSpec.RCB    = memSpecDDR4->tRC / memSpecDDR4->tCK;
        //FIXME: memTimingSpec.RPB    = memSpecDDR4->tRP / memSpecDDR4->tCK;
        //FIXME: memTimingSpec.RRDB   = memSpecDDR4->tRRD_S / memSpecDDR4->tCK;
        //FIXME: memTimingSpec.RRDB_L = memSpecDDR4->tRRD_L / memSpecDDR4->tCK;
        //FIXME: memTimingSpec.RRDB_S = memSpecDDR4->tRRD_S / memSpecDDR4->tCK;
        memTimingSpec.AL     = memSpecDDR4->tAL / memSpecDDR4->tCK;
        memTimingSpec.CCD    = memSpecDDR4->tCCD_S / memSpecDDR4->tCK;
        memTimingSpec.CCD_L  = memSpecDDR4->tCCD_L / memSpecDDR4->tCK;
        memTimingSpec.CCD_S  = memSpecDDR4->tCCD_S / memSpecDDR4->tCK;
        memTimingSpec.CKE    = memSpecDDR4->tCKE / memSpecDDR4->tCK;
        memTimingSpec.CKESR  = memSpecDDR4->tCKESR / memSpecDDR4->tCK;
        memTimingSpec.clkMhz = memSpecDDR4->fCKMHz;
        // See also MemTimingSpec.cc in DRAMPower
        memTimingSpec.clkPeriod = 1000.0 / memSpecDDR4->fCKMHz;
        memTimingSpec.DQSCK  = memSpecDDR4->tDQSCK / memSpecDDR4->tCK;
        memTimingSpec.FAW    = memSpecDDR4->tFAW / memSpecDDR4->tCK;
        memTimingSpec.RAS    = memSpecDDR4->tRAS / memSpecDDR4->tCK;
        memTimingSpec.RC     = memSpecDDR4->tRC / memSpecDDR4->tCK;
        memTimingSpec.RCD    = memSpecDDR4->tRCD / memSpecDDR4->tCK;
        memTimingSpec.REFI   = memSpecDDR4->tREFI / memSpecDDR4->tCK;
        memTimingSpec.RFC    = memSpecDDR4->tRFC / memSpecDDR4->tCK;
        memTimingSpec.RL     = memSpecDDR4->tRL / memSpecDDR4->tCK;
        memTimingSpec.RP     = memSpecDDR4->tRP / memSpecDDR4->tCK;
        memTimingSpec.RRD    = memSpecDDR4->tRRD_S / memSpecDDR4->tCK;
        memTimingSpec.RRD_L  = memSpecDDR4->tRRD_L / memSpecDDR4->tCK;
        memTimingSpec.RRD_S  = memSpecDDR4->tRRD_S / memSpecDDR4->tCK;
        memTimingSpec.RTP    = memSpecDDR4->tRTP / memSpecDDR4->tCK;
        memTimingSpec.TAW    = memSpecDDR4->tFAW / memSpecDDR4->tCK;
        memTimingSpec.WL     = memSpecDDR4->tWL / memSpecDDR4->tCK;
        memTimingSpec.WR     = memSpecDDR4->tWR / memSpecDDR4->tCK;
        memTimingSpec.WTR    = memSpecDDR4->tWTR_S / memSpecDDR4->tCK;
        memTimingSpec.WTR_L  = memSpecDDR4->tWTR_L / memSpecDDR4->tCK;
        memTimingSpec.WTR_S  = memSpecDDR4->tWTR_S / memSpecDDR4->tCK;
        memTimingSpec.XP     = memSpecDDR4->tXP / memSpecDDR4->tCK;
        memTimingSpec.XPDLL  = memSpecDDR4->tXPDLL / memSpecDDR4->tCK;
        memTimingSpec.XS     = memSpecDDR4->tXS / memSpecDDR4->tCK;
        memTimingSpec.XSDLL  = memSpecDDR4->tXSDLL / memSpecDDR4->tCK;

        MemPowerSpec  memPowerSpec;
        memPowerSpec.idd0    = memSpecDDR4->iDD0;
        memPowerSpec.idd02   = memSpecDDR4->iDD02;
        memPowerSpec.idd2p0  = memSpecDDR4->iDD2P0;
        memPowerSpec.idd2p02 = 0;
        memPowerSpec.idd2p1  = memSpecDDR4->iDD2P1;
        memPowerSpec.idd2p12 = 0;
        memPowerSpec.idd2n   = memSpecDDR4->iDD2N;
        memPowerSpec.idd2n2  = 0;
        memPowerSpec.idd3p0  = memSpecDDR4->iDD3P0;
        memPowerSpec.idd3p02 = 0;
        memPowerSpec.idd3p1  = memSpecDDR4->iDD3P1;
        memPowerSpec.idd3p12 = 0;
        memPowerSpec.idd3n   = memSpecDDR4->iDD3N;
        memPowerSpec.idd3n2  = 0;
        memPowerSpec.idd4r   = memSpecDDR4->iDD4R;
        memPowerSpec.idd4r2  = 0;
        memPowerSpec.idd4w   = memSpecDDR4->iDD4W;
        memPowerSpec.idd4w2  = 0;
        memPowerSpec.idd5    = memSpecDDR4->iDD5;
        memPowerSpec.idd52   = 0;
        memPowerSpec.idd6    = memSpecDDR4->iDD6;
        memPowerSpec.idd62   = memSpecDDR4->iDD62;
        memPowerSpec.vdd     = memSpecDDR4->vDD;
        memPowerSpec.vdd2    = memSpecDDR4->vDD2;

        MemorySpecification powerSpec;
        powerSpec.id = memSpecDDR4->memoryId;
        powerSpec.memoryType = MemoryType::DDR4;
        powerSpec.memTimingSpec = memTimingSpec;
        powerSpec.memPowerSpec  = memPowerSpec;
        powerSpec.memArchSpec   = memArchSpec;

        DRAMPower = std::make_unique<libDRAMPower>(powerSpec, false);
#endif
    }
}
