/*
 * Copyright (c) 2020, Technische Universität Kaiserslautern
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

#include <memory>

#include "DRAMSysRecordable.h"
#include "../controller/ControllerRecordable.h"
#include "dram/DramRecordable.h"
#include "dram/DramDDR3.h"
#include "dram/DramDDR4.h"
#include "dram/DramDDR5.h"
#include "dram/DramWideIO.h"
#include "dram/DramLPDDR4.h"
#include "dram/DramLPDDR5.h"
#include "dram/DramWideIO2.h"
#include "dram/DramHBM2.h"
#include "dram/DramGDDR5.h"
#include "dram/DramGDDR5X.h"
#include "dram/DramGDDR6.h"
#include "dram/DramSTTMRAM.h"
#include "../common/TlmRecorder.h"
#include "../simulation/TemperatureController.h"
#include "../error/ecchamming.h"

using namespace sc_core;

DRAMSysRecordable::DRAMSysRecordable(const sc_module_name& name, const DRAMSysConfiguration::Configuration& configLib)
    : DRAMSys(name, configLib, false)
{
    // If a simulation file is passed as argument to DRAMSys the simulation ID
    // is prepended to the simulation name if found.
    std::string traceName;

    if (!configLib.simulationId.empty())
    {
        std::string sid = configLib.simulationId;
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

void DRAMSysRecordable::setupTlmRecorders(const std::string& traceName, const DRAMSysConfiguration::Configuration& configLib)
{
    // Create TLM Recorders, one per channel.
    for (std::size_t i = 0; i < config.memSpec->numberOfChannels; i++)
    {
        std::string dbName = traceName + std::string("_ch") + std::to_string(i) + ".tdb";
        std::string recorderName = "tlmRecorder" + std::to_string(i);

        tlmRecorders.emplace_back(recorderName, config, dbName);
        tlmRecorders.back().recordMcConfig(DRAMSysConfiguration::dump(configLib.mcConfig));
        tlmRecorders.back().recordMemspec(DRAMSysConfiguration::dump(configLib.memSpec));
        tlmRecorders.back().recordTraceNames(config.simulationName);
    }
}

void DRAMSysRecordable::instantiateModules(const std::string &traceName,
                                           const DRAMSysConfiguration::Configuration &configLib)
{
    temperatureController = std::make_unique<TemperatureController>("TemperatureController", config);

    // Create and properly initialize TLM recorders.
    // They need to be ready before creating some modules.
    setupTlmRecorders(traceName, configLib);

    // Create arbiter
    if (config.arbiter == Configuration::Arbiter::Simple)
        arbiter = std::make_unique<ArbiterSimple>("arbiter", config, configLib.addressMapping);
    else if (config.arbiter == Configuration::Arbiter::Fifo)
        arbiter = std::make_unique<ArbiterFifo>("arbiter", config, configLib.addressMapping);
    else if (config.arbiter == Configuration::Arbiter::Reorder)
        arbiter = std::make_unique<ArbiterReorder>("arbiter", config, configLib.addressMapping);

    // Create controllers and DRAMs
    MemSpec::MemoryType memoryType = config.memSpec->memoryType;
    for (std::size_t i = 0; i < config.memSpec->numberOfChannels; i++)
    {
        controllers.emplace_back(std::make_unique<ControllerRecordable>(("controller" + std::to_string(i)).c_str(),
                                                                        config, tlmRecorders[i]));

        if (memoryType == MemSpec::MemoryType::DDR3)
            drams.emplace_back(std::make_unique<DramRecordable<DramDDR3>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::DDR4)
            drams.emplace_back(std::make_unique<DramRecordable<DramDDR4>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::DDR5)
            drams.emplace_back(std::make_unique<DramRecordable<DramDDR5>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::WideIO)
            drams.emplace_back(std::make_unique<DramRecordable<DramWideIO>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::LPDDR4)
            drams.emplace_back(std::make_unique<DramRecordable<DramLPDDR4>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::LPDDR5)
            drams.emplace_back(std::make_unique<DramRecordable<DramLPDDR5>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::WideIO2)
            drams.emplace_back(std::make_unique<DramRecordable<DramWideIO2>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::HBM2)
            drams.emplace_back(std::make_unique<DramRecordable<DramHBM2>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::GDDR5)
            drams.emplace_back(std::make_unique<DramRecordable<DramGDDR5>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::GDDR5X)
            drams.emplace_back(std::make_unique<DramRecordable<DramGDDR5X>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::GDDR6)
            drams.emplace_back(std::make_unique<DramRecordable<DramGDDR6>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));
        else if (memoryType == MemSpec::MemoryType::STTMRAM)
            drams.emplace_back(std::make_unique<DramRecordable<DramSTTMRAM>>(("dram" + std::to_string(i)).c_str(),
                    config, *temperatureController, tlmRecorders[i]));

        if (config.checkTLM2Protocol)
            controllersTlmCheckers.emplace_back(std::make_unique<tlm_utils::tlm2_base_protocol_checker<>>
                    (("TLMCheckerController" + std::to_string(i)).c_str()));
    }
}
