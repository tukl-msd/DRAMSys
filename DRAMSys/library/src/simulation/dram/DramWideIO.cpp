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

#include "DramWideIO.h"

#include <systemc.h>
#include <tlm.h>
#include "Dram.h"
#include "../../configuration/Configuration.h"
#include "../../error/errormodel.h"
#include "../../common/third_party/DRAMPower/src/libdrampower/LibDRAMPower.h"
#include "../../configuration/memspec/MemSpecWideIO.h"

using namespace tlm;
using namespace DRAMPower;

DramWideIO::DramWideIO(sc_module_name name) : Dram(name)
{
    if (Configuration::getInstance().powerAnalysis)
    {
        MemSpecWideIO *memSpec = dynamic_cast<MemSpecWideIO *>(this->memSpec);
        if (memSpec == nullptr)
            SC_REPORT_FATAL("DramWideIO", "Wrong MemSpec chosen");

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
        memArchSpec.dll               = false;

        MemTimingSpec memTimingSpec;
        //FIXME: memTimingSpec.FAWB   = memSpec->tTAW / memSpec->tCK;
        //FIXME: memTimingSpec.RASB   = memSpec->tRAS / memSpec->tCK;
        //FIXME: memTimingSpec.RCB    = memSpec->tRC / memSpec->tCK;
        //FIXME: memTimingSpec.RPB    = memSpec->tRP / memSpec->tCK;
        //FIXME: memTimingSpec.RRDB   = memSpec->tRRD / memSpec->tCK;
        //FIXME: memTimingSpec.RRDB_L = memSpec->tRRD / memSpec->tCK;
        //FIXME: memTimingSpec.RRDB_S = memSpec->tRRD / memSpec->tCK;
        memTimingSpec.AL     = 0;
        memTimingSpec.CCD    = memSpec->burstLength;
        memTimingSpec.CCD_L  = memSpec->burstLength;
        memTimingSpec.CCD_S  = memSpec->burstLength;
        memTimingSpec.CKE    = memSpec->tCKE / memSpec->tCK;
        memTimingSpec.CKESR  = memSpec->tCKESR / memSpec->tCK;
        memTimingSpec.clkMhz = memSpec->fCKMHz;
        // See also MemTimingSpec.cc in DRAMPower
        memTimingSpec.clkPeriod = 1000.0 / memSpec->fCKMHz;
        memTimingSpec.DQSCK  = memSpec->tDQSCK / memSpec->tCK;
        memTimingSpec.FAW    = memSpec->tTAW / memSpec->tCK;
        memTimingSpec.RAS    = memSpec->tRAS / memSpec->tCK;
        memTimingSpec.RC     = memSpec->tRC / memSpec->tCK;
        memTimingSpec.RCD    = memSpec->tRCD / memSpec->tCK;
        memTimingSpec.REFI   = memSpec->tREFI / memSpec->tCK;
        memTimingSpec.RFC    = memSpec->tRFC / memSpec->tCK;
        memTimingSpec.RL     = memSpec->tRL / memSpec->tCK;
        memTimingSpec.RP     = memSpec->tRP / memSpec->tCK;
        memTimingSpec.RRD    = memSpec->tRRD / memSpec->tCK;
        memTimingSpec.RRD_L  = memSpec->tRRD / memSpec->tCK;
        memTimingSpec.RRD_S  = memSpec->tRRD / memSpec->tCK;
        memTimingSpec.RTP    = memSpec->burstLength;
        memTimingSpec.TAW    = memSpec->tTAW / memSpec->tCK;
        memTimingSpec.WL     = memSpec->tWL / memSpec->tCK;
        memTimingSpec.WR     = memSpec->tWR / memSpec->tCK;
        memTimingSpec.WTR    = memSpec->tWTR / memSpec->tCK;
        memTimingSpec.WTR_L  = memSpec->tWTR / memSpec->tCK;
        memTimingSpec.WTR_S  = memSpec->tWTR / memSpec->tCK;
        memTimingSpec.XP     = memSpec->tXP / memSpec->tCK;
        memTimingSpec.XPDLL  = memSpec->tXP / memSpec->tCK;
        memTimingSpec.XS     = memSpec->tXSR / memSpec->tCK;
        memTimingSpec.XSDLL  = memSpec->tXSR / memSpec->tCK;

        MemPowerSpec  memPowerSpec;
        memPowerSpec.idd0    = memSpec->iDD0;
        memPowerSpec.idd02   = memSpec->iDD02;
        memPowerSpec.idd2p0  = memSpec->iDD2P0;
        memPowerSpec.idd2p02 = memSpec->iDD2P02;
        memPowerSpec.idd2p1  = memSpec->iDD2P1;
        memPowerSpec.idd2p12 = memSpec->iDD2P12;
        memPowerSpec.idd2n   = memSpec->iDD2N;
        memPowerSpec.idd2n2  = memSpec->iDD2N2;
        memPowerSpec.idd3p0  = memSpec->iDD3P0;
        memPowerSpec.idd3p02 = memSpec->iDD3P02;
        memPowerSpec.idd3p1  = memSpec->iDD3P1;
        memPowerSpec.idd3p12 = memSpec->iDD3P12;
        memPowerSpec.idd3n   = memSpec->iDD3N;
        memPowerSpec.idd3n2  = memSpec->iDD3N2;
        memPowerSpec.idd4r   = memSpec->iDD4R;
        memPowerSpec.idd4r2  = memSpec->iDD4R2;
        memPowerSpec.idd4w   = memSpec->iDD4W;
        memPowerSpec.idd4w2  = memSpec->iDD4W2;
        memPowerSpec.idd5    = memSpec->iDD5;
        memPowerSpec.idd52   = memSpec->iDD52;
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

        // For each bank in a channel a error Model is created:
        if (storeMode == StorageMode::ErrorModel)
        {
            for (unsigned i = 0; i < memSpec->numberOfBanks; i++)
            {
                errorModel *em;
                std::string errorModelStr = "errorModel_bank" + std::to_string(i);
                em = new errorModel(errorModelStr.c_str(), DRAMPower);
                ememory.push_back(em);
            }
        }
    }
    else
    {
        if (storeMode == StorageMode::ErrorModel)
        {
            for (unsigned i = 0; i < memSpec->numberOfBanks; i++)
            {
                errorModel *em;
                std::string errorModelStr = "errorModel_bank" + std::to_string(i);
                em = new errorModel(errorModelStr.c_str());
                ememory.push_back(em);
            }
        }
    }
}

DramWideIO::~DramWideIO()
{
    // Clean up:
    for (auto e : ememory)
        delete e;
}

tlm_sync_enum DramWideIO::nb_transport_fw(tlm_generic_payload &payload,
                                           tlm_phase &phase, sc_time &)
{
    assert(phase >= 5 && phase <= 19);

    if (Configuration::getInstance().powerAnalysis)
    {
        unsigned bank = DramExtension::getExtension(payload).getBank().ID();
        unsigned long long cycle = sc_time_stamp() / memSpec->tCK;
        DRAMPower->doCommand(phaseToDRAMPowerCommand(phase), bank, cycle);
    }

    if (storeMode == StorageMode::Store)
    {
        if (phase == BEGIN_RD || phase == BEGIN_RDA)
        {
            unsigned char *phyAddr = memory + payload.get_address();
            memcpy(payload.get_data_ptr(), phyAddr, payload.get_data_length());
        }
        else if (phase == BEGIN_WR || phase == BEGIN_WRA)
        {
            unsigned char *phyAddr = memory + payload.get_address();
            memcpy(phyAddr, payload.get_data_ptr(), payload.get_data_length());
        }
    }
    else if (storeMode == StorageMode::ErrorModel)
    {
        unsigned bank = DramExtension::getExtension(payload).getBank().ID();

        if (phase == BEGIN_ACT)
            ememory[bank]->activate(DramExtension::getExtension(payload).getRow().ID());
        else if (phase == BEGIN_RD || phase == BEGIN_RDA)
            ememory[bank]->load(payload);
        else if (phase == BEGIN_WR || phase == BEGIN_WRA)
            ememory[bank]->store(payload);
        else if (phase == BEGIN_REFA)
            ememory[bank]->refresh(DramExtension::getExtension(payload).getRow().ID());
    }

    return TLM_ACCEPTED;
}
