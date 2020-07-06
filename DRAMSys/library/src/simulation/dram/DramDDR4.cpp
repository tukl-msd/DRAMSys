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

#include "Dram.h"
#include "../../configuration/Configuration.h"
#include "../../common/third_party/DRAMPower/src/libdrampower/LibDRAMPower.h"
#include "../../configuration/memspec/MemSpecDDR4.h"

using namespace DRAMPower;

DramDDR4::DramDDR4(sc_module_name name) : Dram(name)
{
    if (storeMode == StorageMode::ErrorModel)
        SC_REPORT_FATAL("DramDDR4", "Error Model not supported for DDR4");

    if (Configuration::getInstance().powerAnalysis)
    {
        MemSpecDDR4 *memSpec = dynamic_cast<MemSpecDDR4 *>(this->memSpec);
        if (memSpec == nullptr)
            SC_REPORT_FATAL("DramDDR4", "Wrong MemSpec chosen");

        MemArchitectureSpec memArchSpec;
        memArchSpec.burstLength       = memSpec->burstLength;
        memArchSpec.dataRate          = memSpec->dataRate;
        memArchSpec.nbrOfRows         = memSpec->numberOfRows;
        memArchSpec.nbrOfBanks        = memSpec->numberOfBanks;
        memArchSpec.nbrOfColumns      = memSpec->numberOfColumns;
        memArchSpec.nbrOfRanks        = memSpec->numberOfRanks;
        memArchSpec.width             = memSpec->bitWidth;
        memArchSpec.nbrOfBankGroups   = memSpec->numberOfBankGroups;
        memArchSpec.twoVoltageDomains = true;
        memArchSpec.dll               = true;

        MemTimingSpec memTimingSpec;
        //FIXME: memTimingSpec.FAWB   = memSpec->tFAW / memSpec->tCK;
        //FIXME: memTimingSpec.RASB   = memSpec->tRAS / memSpec->tCK;
        //FIXME: memTimingSpec.RCB    = memSpec->tRC / memSpec->tCK;
        //FIXME: memTimingSpec.RPB    = memSpec->tRP / memSpec->tCK;
        //FIXME: memTimingSpec.RRDB   = memSpec->tRRD_S / memSpec->tCK;
        //FIXME: memTimingSpec.RRDB_L = memSpec->tRRD_L / memSpec->tCK;
        //FIXME: memTimingSpec.RRDB_S = memSpec->tRRD_S / memSpec->tCK;
        memTimingSpec.AL     = memSpec->tAL / memSpec->tCK;
        memTimingSpec.CCD    = memSpec->tCCD_S / memSpec->tCK;
        memTimingSpec.CCD_L  = memSpec->tCCD_L / memSpec->tCK;
        memTimingSpec.CCD_S  = memSpec->tCCD_S / memSpec->tCK;
        memTimingSpec.CKE    = memSpec->tCKE / memSpec->tCK;
        memTimingSpec.CKESR  = memSpec->tCKESR / memSpec->tCK;
        memTimingSpec.clkMhz = memSpec->fCKMHz;
        // See also MemTimingSpec.cc in DRAMPower
        memTimingSpec.clkPeriod = 1000.0 / memSpec->fCKMHz;
        memTimingSpec.DQSCK  = memSpec->tDQSCK / memSpec->tCK;
        memTimingSpec.FAW    = memSpec->tFAW / memSpec->tCK;
        memTimingSpec.RAS    = memSpec->tRAS / memSpec->tCK;
        memTimingSpec.RC     = memSpec->tRC / memSpec->tCK;
        memTimingSpec.RCD    = memSpec->tRCD / memSpec->tCK;
        memTimingSpec.REFI   = memSpec->tREFI / memSpec->tCK;
        memTimingSpec.RFC    = memSpec->tRFC / memSpec->tCK;
        memTimingSpec.RL     = memSpec->tRL / memSpec->tCK;
        memTimingSpec.RP     = memSpec->tRP / memSpec->tCK;
        memTimingSpec.RRD    = memSpec->tRRD_S / memSpec->tCK;
        memTimingSpec.RRD_L  = memSpec->tRRD_L / memSpec->tCK;
        memTimingSpec.RRD_S  = memSpec->tRRD_S / memSpec->tCK;
        memTimingSpec.RTP    = memSpec->tRTP / memSpec->tCK;
        memTimingSpec.TAW    = memSpec->tFAW / memSpec->tCK;
        memTimingSpec.WL     = memSpec->tWL / memSpec->tCK;
        memTimingSpec.WR     = memSpec->tWR / memSpec->tCK;
        memTimingSpec.WTR    = memSpec->tWTR_S / memSpec->tCK;
        memTimingSpec.WTR_L  = memSpec->tWTR_L / memSpec->tCK;
        memTimingSpec.WTR_S  = memSpec->tWTR_S / memSpec->tCK;
        memTimingSpec.XP     = memSpec->tXP / memSpec->tCK;
        memTimingSpec.XPDLL  = memSpec->tXPDLL / memSpec->tCK;
        memTimingSpec.XS     = memSpec->tXS / memSpec->tCK;
        memTimingSpec.XSDLL  = memSpec->tXSDLL / memSpec->tCK;

        MemPowerSpec  memPowerSpec;
        memPowerSpec.idd0    = memSpec->iDD0;
        memPowerSpec.idd02   = memSpec->iDD02;
        memPowerSpec.idd2p0  = memSpec->iDD2P0;
        memPowerSpec.idd2p02 = 0;
        memPowerSpec.idd2p1  = memSpec->iDD2P1;
        memPowerSpec.idd2p12 = 0;
        memPowerSpec.idd2n   = memSpec->iDD2N;
        memPowerSpec.idd2n2  = 0;
        memPowerSpec.idd3p0  = memSpec->iDD3P0;
        memPowerSpec.idd3p02 = 0;
        memPowerSpec.idd3p1  = memSpec->iDD3P1;
        memPowerSpec.idd3p12 = 0;
        memPowerSpec.idd3n   = memSpec->iDD3N;
        memPowerSpec.idd3n2  = 0;
        memPowerSpec.idd4r   = memSpec->iDD4R;
        memPowerSpec.idd4r2  = 0;
        memPowerSpec.idd4w   = memSpec->iDD4W;
        memPowerSpec.idd4w2  = 0;
        memPowerSpec.idd5    = memSpec->iDD5;
        memPowerSpec.idd52   = 0;
        memPowerSpec.idd6    = memSpec->iDD6;
        memPowerSpec.idd62   = memSpec->iDD62;
        memPowerSpec.vdd     = memSpec->vDD;
        memPowerSpec.vdd2    = memSpec->vDD2;

        MemorySpecification powerSpec;
        powerSpec.id = memSpec->memoryId;
        powerSpec.memoryType = memSpec->memoryType;
        powerSpec.memTimingSpec = memTimingSpec;
        powerSpec.memPowerSpec  = memPowerSpec;
        powerSpec.memArchSpec   = memArchSpec;

        DRAMPower = new libDRAMPower(powerSpec, 0);
    }
}
