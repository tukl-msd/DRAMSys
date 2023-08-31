/*
 * Copyright (c) 2020, RPTU Kaiserslautern-Landau
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

#include "DRAMSysRecordable.h"

#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/controller/ControllerRecordable.h"
#include "DRAMSys/simulation/dram/DramDDR3.h"
#include "DRAMSys/simulation/dram/DramDDR4.h"
#include "DRAMSys/simulation/dram/DramGDDR5.h"
#include "DRAMSys/simulation/dram/DramGDDR5X.h"
#include "DRAMSys/simulation/dram/DramGDDR6.h"
#include "DRAMSys/simulation/dram/DramHBM2.h"
#include "DRAMSys/simulation/dram/DramLPDDR4.h"
#include "DRAMSys/simulation/dram/DramRecordable.h"
#include "DRAMSys/simulation/dram/DramSTTMRAM.h"
#include "DRAMSys/simulation/dram/DramWideIO.h"
#include "DRAMSys/simulation/dram/DramWideIO2.h"

#ifdef DDR5_SIM
#include "DRAMSys/simulation/dram/DramDDR5.h"
#endif
#ifdef LPDDR5_SIM
#include "DRAMSys/simulation/dram/DramLPDDR5.h"
#endif
#ifdef HBM3_SIM
#include "DRAMSys/simulation/dram/DramHBM3.h"
#endif

#include <memory>

namespace DRAMSys
{

DRAMSysRecordable::DRAMSysRecordable(const sc_core::sc_module_name& name,
                                     const ::DRAMSys::Config::Configuration& configLib) :
    DRAMSys(name, configLib, false)
{
    // If a simulation file is passed as argument to DRAMSys the simulation ID
    // is prepended to the simulation name if found.
    std::string traceName;

    if (!configLib.simulationid.empty())
    {
        std::string sid = configLib.simulationid;
        traceName = sid + '_' + config.simulationName;
    }
    else
        traceName = config.simulationName;

    instantiateModules(traceName, configLib);
    bindSockets();
    report(headline);
}

void DRAMSysRecordable::end_of_simulation()
{
    // Report power before TLM recorders are finalized
    if (config.powerAnalysis)
    {
        for (auto& dram : drams)
            dram->reportPower();
    }

    for (auto& tlmRecorder : tlmRecorders)
        tlmRecorder.finalize();
}

void DRAMSysRecordable::setupTlmRecorders(const std::string& traceName,
                                          const ::DRAMSys::Config::Configuration& configLib)
{
    // Create TLM Recorders, one per channel.
    // Reserve is required because the recorders use double buffers that are accessed with pointers.
    // Without a reserve, the vector reallocates storage before inserting a second
    // element and the pointers are not valid anymore.
    tlmRecorders.reserve(config.memSpec->numberOfChannels);
    for (std::size_t i = 0; i < config.memSpec->numberOfChannels; i++)
    {
        std::string dbName =
            std::string(name()) + "_" + traceName + "_ch" + std::to_string(i) + ".tdb";
        std::string recorderName = "tlmRecorder" + std::to_string(i);

        nlohmann::json mcconfig;
        nlohmann::json memspec;
        mcconfig[Config::McConfig::KEY] = configLib.mcconfig;
        memspec[Config::MemSpec::KEY] = configLib.memspec;

        tlmRecorders.emplace_back(
            recorderName, config, dbName, mcconfig.dump(), memspec.dump(), config.simulationName);
    }
}

void DRAMSysRecordable::instantiateModules(const std::string& traceName,
                                           const ::DRAMSys::Config::Configuration& configLib)
{
    addressDecoder = std::make_unique<AddressDecoder>(configLib.addressmapping, *config.memSpec);
    addressDecoder->print();

    // Create and properly initialize TLM recorders.
    // They need to be ready before creating some modules.
    setupTlmRecorders(traceName, configLib);

    // Create arbiter
    if (config.arbiter == Configuration::Arbiter::Simple)
        arbiter = std::make_unique<ArbiterSimple>("arbiter", config, *addressDecoder);
    else if (config.arbiter == Configuration::Arbiter::Fifo)
        arbiter = std::make_unique<ArbiterFifo>("arbiter", config, *addressDecoder);
    else if (config.arbiter == Configuration::Arbiter::Reorder)
        arbiter = std::make_unique<ArbiterReorder>("arbiter", config, *addressDecoder);

    // Create controllers and DRAMs
    MemSpec::MemoryType memoryType = config.memSpec->memoryType;
    for (std::size_t i = 0; i < config.memSpec->numberOfChannels; i++)
    {
        controllers.emplace_back(std::make_unique<ControllerRecordable>(
            ("controller" + std::to_string(i)).c_str(), config, *addressDecoder, tlmRecorders[i]));

        if (memoryType == MemSpec::MemoryType::DDR3)
            drams.emplace_back(std::make_unique<DramRecordable<DramDDR3>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::DDR4)
            drams.emplace_back(std::make_unique<DramRecordable<DramDDR4>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::WideIO)
            drams.emplace_back(std::make_unique<DramRecordable<DramWideIO>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::LPDDR4)
            drams.emplace_back(std::make_unique<DramRecordable<DramLPDDR4>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::WideIO2)
            drams.emplace_back(std::make_unique<DramRecordable<DramWideIO2>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::HBM2)
            drams.emplace_back(std::make_unique<DramRecordable<DramHBM2>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::GDDR5)
            drams.emplace_back(std::make_unique<DramRecordable<DramGDDR5>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::GDDR5X)
            drams.emplace_back(std::make_unique<DramRecordable<DramGDDR5X>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::GDDR6)
            drams.emplace_back(std::make_unique<DramRecordable<DramGDDR6>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::STTMRAM)
            drams.emplace_back(std::make_unique<DramRecordable<DramSTTMRAM>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
#ifdef DDR5_SIM
        else if (memoryType == MemSpec::MemoryType::DDR5)
            drams.emplace_back(std::make_unique<DramRecordable<DramDDR5>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
#endif
#ifdef LPDDR5_SIM
        else if (memoryType == MemSpec::MemoryType::LPDDR5)
            drams.emplace_back(std::make_unique<DramRecordable<DramLPDDR5>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
#endif
#ifdef HBM3_SIM
        else if (memoryType == MemSpec::MemoryType::HBM3)
            drams.emplace_back(std::make_unique<DramRecordable<DramHBM3>>(
                ("dram" + std::to_string(i)).c_str(), config, tlmRecorders[i]));
#endif

        if (config.checkTLM2Protocol)
            controllersTlmCheckers.emplace_back(
                std::make_unique<tlm_utils::tlm2_base_protocol_checker<>>(
                    ("TLMCheckerController" + std::to_string(i)).c_str()));
    }
}

} // namespace DRAMSys
