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

#include "DramDDR3.h"

#include <memory>
#include "../../configuration/Configuration.h"
#include "../../configuration/memspec/MemSpecDDR3.h"

#ifdef DRAMPOWER
#include "../../common/third_party/DRAMPower/src/libdrampower/LibDRAMPower.h"
using namespace DRAMPower;
#endif

using namespace sc_core;

DramDDR3::DramDDR3(const sc_module_name& name, const Configuration& config,
                   TemperatureController& temperatureController)
    : Dram(name, config)
{
    if (storeMode == Configuration::StoreMode::ErrorModel)
        SC_REPORT_FATAL("DramDDR3", "Error Model not supported for DDR3");

    if (powerAnalysis)
    {
        const auto *memSpecDDR3 = dynamic_cast<const MemSpecDDR3*>(config.memSpec.get());
        if (memSpecDDR3 == nullptr)
            SC_REPORT_FATAL("DramDDR3", "Wrong MemSpec chosen");

#ifdef DRAMPOWER
        MemArchitectureSpec memArchSpec;
        memArchSpec.burstLength       = memSpecDDR3->defaultBurstLength;
        memArchSpec.dataRate          = memSpecDDR3->dataRate;
        memArchSpec.nbrOfRows         = memSpecDDR3->rowsPerBank;
        memArchSpec.nbrOfBanks        = memSpecDDR3->banksPerChannel;
        memArchSpec.nbrOfColumns      = memSpecDDR3->columnsPerRow;
        memArchSpec.nbrOfRanks        = memSpecDDR3->ranksPerChannel;
        memArchSpec.width             = memSpecDDR3->bitWidth;
        memArchSpec.nbrOfBankGroups   = memSpecDDR3->bankGroupsPerChannel;
        memArchSpec.twoVoltageDomains = false;
        memArchSpec.dll               = true;

        MemTimingSpec memTimingSpec;
        //FIXME: memTimingSpec.FAWB   = memSpecDDR3->tFAW / memSpecDDR3->tCK;
        //FIXME: memTimingSpec.RASB   = memSpecDDR3->tRAS / memSpecDDR3->tCK;
        //FIXME: memTimingSpec.RCB    = memSpecDDR3->tRC / memSpecDDR3->tCK;
        //FIXME: memTimingSpec.RPB    = memSpecDDR3->tRP / memSpecDDR3->tCK;
        //FIXME: memTimingSpec.RRDB   = memSpecDDR3->tRRD / memSpecDDR3->tCK;
        //FIXME: memTimingSpec.RRDB_L = memSpecDDR3->tRRD / memSpecDDR3->tCK;
        //FIXME: memTimingSpec.RRDB_S = memSpecDDR3->tRRD / memSpecDDR3->tCK;
        memTimingSpec.AL     = memSpecDDR3->tAL / memSpecDDR3->tCK;
        memTimingSpec.CCD    = memSpecDDR3->tCCD / memSpecDDR3->tCK;
        memTimingSpec.CCD_L  = memSpecDDR3->tCCD / memSpecDDR3->tCK;
        memTimingSpec.CCD_S  = memSpecDDR3->tCCD / memSpecDDR3->tCK;
        memTimingSpec.CKE    = memSpecDDR3->tCKE / memSpecDDR3->tCK;
        memTimingSpec.CKESR  = memSpecDDR3->tCKESR / memSpecDDR3->tCK;
        memTimingSpec.clkMhz = memSpecDDR3->fCKMHz;
        // See also MemTimingSpec.cc in DRAMPower
        memTimingSpec.clkPeriod = 1000.0 / memSpecDDR3->fCKMHz;
        memTimingSpec.DQSCK  = memSpecDDR3->tDQSCK / memSpecDDR3->tCK;
        memTimingSpec.FAW    = memSpecDDR3->tFAW / memSpecDDR3->tCK;
        memTimingSpec.RAS    = memSpecDDR3->tRAS / memSpecDDR3->tCK;
        memTimingSpec.RC     = memSpecDDR3->tRC / memSpecDDR3->tCK;
        memTimingSpec.RCD    = memSpecDDR3->tRCD / memSpecDDR3->tCK;
        memTimingSpec.REFI   = memSpecDDR3->tREFI / memSpecDDR3->tCK;
        memTimingSpec.RFC    = memSpecDDR3->tRFC / memSpecDDR3->tCK;
        memTimingSpec.RL     = memSpecDDR3->tRL / memSpecDDR3->tCK;
        memTimingSpec.RP     = memSpecDDR3->tRP / memSpecDDR3->tCK;
        memTimingSpec.RRD    = memSpecDDR3->tRRD / memSpecDDR3->tCK;
        memTimingSpec.RRD_L  = memSpecDDR3->tRRD / memSpecDDR3->tCK;
        memTimingSpec.RRD_S  = memSpecDDR3->tRRD / memSpecDDR3->tCK;
        memTimingSpec.RTP    = memSpecDDR3->tRTP / memSpecDDR3->tCK;
        memTimingSpec.TAW    = memSpecDDR3->tFAW / memSpecDDR3->tCK;
        memTimingSpec.WL     = memSpecDDR3->tWL / memSpecDDR3->tCK;
        memTimingSpec.WR     = memSpecDDR3->tWR / memSpecDDR3->tCK;
        memTimingSpec.WTR    = memSpecDDR3->tWTR / memSpecDDR3->tCK;
        memTimingSpec.WTR_L  = memSpecDDR3->tWTR / memSpecDDR3->tCK;
        memTimingSpec.WTR_S  = memSpecDDR3->tWTR / memSpecDDR3->tCK;
        memTimingSpec.XP     = memSpecDDR3->tXP / memSpecDDR3->tCK;
        memTimingSpec.XPDLL  = memSpecDDR3->tXPDLL / memSpecDDR3->tCK;
        memTimingSpec.XS     = memSpecDDR3->tXS / memSpecDDR3->tCK;
        memTimingSpec.XSDLL  = memSpecDDR3->tXSDLL / memSpecDDR3->tCK;

        MemPowerSpec  memPowerSpec;
        memPowerSpec.idd0    = memSpecDDR3->iDD0;
        memPowerSpec.idd02   = 0;
        memPowerSpec.idd2p0  = memSpecDDR3->iDD2P0;
        memPowerSpec.idd2p02 = 0;
        memPowerSpec.idd2p1  = memSpecDDR3->iDD2P1;
        memPowerSpec.idd2p12 = 0;
        memPowerSpec.idd2n   = memSpecDDR3->iDD2N;
        memPowerSpec.idd2n2  = 0;
        memPowerSpec.idd3p0  = memSpecDDR3->iDD3P0;
        memPowerSpec.idd3p02 = 0;
        memPowerSpec.idd3p1  = memSpecDDR3->iDD3P1;
        memPowerSpec.idd3p12 = 0;
        memPowerSpec.idd3n   = memSpecDDR3->iDD3N;
        memPowerSpec.idd3n2  = 0;
        memPowerSpec.idd4r   = memSpecDDR3->iDD4R;
        memPowerSpec.idd4r2  = 0;
        memPowerSpec.idd4w   = memSpecDDR3->iDD4W;
        memPowerSpec.idd4w2  = 0;
        memPowerSpec.idd5    = memSpecDDR3->iDD5;
        memPowerSpec.idd52   = 0;
        memPowerSpec.idd6    = memSpecDDR3->iDD6;
        memPowerSpec.idd62   = 0;
        memPowerSpec.vdd     = memSpecDDR3->vDD;
        memPowerSpec.vdd2    = 0;

        MemorySpecification powerSpec;
        powerSpec.id = memSpecDDR3->memoryId;
        powerSpec.memoryType = MemoryType::DDR3;
        powerSpec.memTimingSpec = memTimingSpec;
        powerSpec.memPowerSpec  = memPowerSpec;
        powerSpec.memArchSpec   = memArchSpec;

        DRAMPower = std::make_unique<libDRAMPower>(powerSpec, false);
#endif
    }
}
