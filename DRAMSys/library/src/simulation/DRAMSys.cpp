/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Eder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 *    Derek Christ
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>

#include "DRAMSys.h"
#include "../common/DebugManager.h"
#include "../common/utils.h"
#include "../simulation/TemperatureController.h"
#include "../error/ecchamming.h"
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
#include "../controller/Controller.h"

DRAMSys::DRAMSys(const sc_core::sc_module_name &name,
                 const DRAMSysConfiguration::Configuration &configLib)
    : DRAMSys(name, configLib, true)
{}

DRAMSys::DRAMSys(const sc_core::sc_module_name &name,
                 const DRAMSysConfiguration::Configuration &configLib,
                 bool initAndBind)
    : sc_module(name), tSocket("DRAMSys_tSocket")
{
    logo();

    // Load configLib and initialize modules
    // Important: The memSpec needs to be the first configuration to be loaded!
    config.loadMemSpec(configLib.memSpec);
    config.loadMCConfig(configLib.mcConfig);
    config.loadSimConfig(configLib.simConfig);

    if (const auto &thermalConfig = configLib.thermalConfig)
        config.loadTemperatureSimConfig(*thermalConfig);

    // Setup the debug manager:
    setupDebugManager(config.simulationName);

    if (initAndBind)
    {
        // Instantiate all internal DRAMSys modules:
        instantiateModules(configLib.addressMapping);
        // Connect all internal DRAMSys modules:
        bindSockets();
        report(headline);
    }
}

const Configuration& DRAMSys::getConfig()
{
    return config;
}

void DRAMSys::end_of_simulation()
{
    if (config.powerAnalysis)
    {
        for (auto& dram : drams)
            dram->reportPower();
    }
}

void DRAMSys::logo()
{
#define GREENTXT(s)  std::string(("\u001b[38;5;28m"+std::string((s))+"\033[0m"))
#define DGREENTXT(s) std::string(("\u001b[38;5;22m"+std::string((s))+"\033[0m"))
#define LGREENTXT(s) std::string(("\u001b[38;5;82m"+std::string((s))+"\033[0m"))
#define BLACKTXT(s)  std::string(("\u001b[38;5;232m"+std::string((s))+"\033[0m"))
#define BOLDTXT(s)   std::string(("\033[1;37m"+std::string((s))+"\033[0m"))
    cout << std::endl
         << BLACKTXT("■ ■ ")<< DGREENTXT("■  ")
         << BOLDTXT("DRAMSys4.0, Copyright (c) 2020")
         << std::endl
         << BLACKTXT("■ ") << DGREENTXT("■ ") << GREENTXT("■  ")
         << "Technische Universitaet Kaiserslautern,"
         << std::endl
         << DGREENTXT("■ ") << GREENTXT("■ ") << LGREENTXT("■  " )
         << "Fraunhofer IESE"
         << std::endl
         << std::endl;
#undef GREENTXT
#undef DGREENTXT
#undef LGREENTXT
#undef BLACKTXT
#undef BOLDTXT
}

void DRAMSys::setupDebugManager(NDEBUG_UNUSED(const std::string &traceName))
{
#ifndef NDEBUG
    auto& dbg = DebugManager::getInstance();
    bool debugEnabled = config.debug;
    bool writeToConsole = false;
    bool writeToFile = true;
    dbg.setup(debugEnabled, writeToConsole, writeToFile);
    if (writeToFile)
        dbg.openDebugFile(traceName + ".txt");
#endif
}

void DRAMSys::instantiateModules(const DRAMSysConfiguration::AddressMapping &addressMapping)
{
    temperatureController = std::make_unique<TemperatureController>("TemperatureController", config);

    // Create arbiter
    if (config.arbiter == Configuration::Arbiter::Simple)
        arbiter = std::make_unique<ArbiterSimple>("arbiter", config, addressMapping);
    else if (config.arbiter == Configuration::Arbiter::Fifo)
        arbiter = std::make_unique<ArbiterFifo>("arbiter", config, addressMapping);
    else if (config.arbiter == Configuration::Arbiter::Reorder)
        arbiter = std::make_unique<ArbiterReorder>("arbiter", config, addressMapping);

    // Create controllers and DRAMs
    MemSpec::MemoryType memoryType = config.memSpec->memoryType;
    for (std::size_t i = 0; i < config.memSpec->numberOfChannels; i++)
    {
        controllers.emplace_back(std::make_unique<Controller>(("controller" + std::to_string(i)).c_str(), config));

        if (memoryType == MemSpec::MemoryType::DDR3)
            drams.emplace_back(std::make_unique<DramDDR3>(("dram" + std::to_string(i)).c_str(), config,
                                                          *temperatureController));
        else if (memoryType == MemSpec::MemoryType::DDR4)
            drams.emplace_back(std::make_unique<DramDDR4>(("dram" + std::to_string(i)).c_str(), config,
                                                          *temperatureController));
        else if (memoryType == MemSpec::MemoryType::DDR5)
            drams.emplace_back(std::make_unique<DramDDR5>(("dram" + std::to_string(i)).c_str(), config,
                                                          *temperatureController));
        else if (memoryType == MemSpec::MemoryType::WideIO)
            drams.emplace_back(std::make_unique<DramWideIO>(("dram" + std::to_string(i)).c_str(), config,
                                                            *temperatureController));
        else if (memoryType == MemSpec::MemoryType::LPDDR4)
            drams.emplace_back(std::make_unique<DramLPDDR4>(("dram" + std::to_string(i)).c_str(), config,
                                                            *temperatureController));
        else if (memoryType == MemSpec::MemoryType::LPDDR5)
            drams.emplace_back(std::make_unique<DramLPDDR5>(("dram" + std::to_string(i)).c_str(), config,
                                                            *temperatureController));
        else if (memoryType == MemSpec::MemoryType::WideIO2)
            drams.emplace_back(std::make_unique<DramWideIO2>(("dram" + std::to_string(i)).c_str(), config,
                                                             *temperatureController));
        else if (memoryType == MemSpec::MemoryType::HBM2)
            drams.emplace_back(std::make_unique<DramHBM2>(("dram" + std::to_string(i)).c_str(), config,
                                                          *temperatureController));
        else if (memoryType == MemSpec::MemoryType::GDDR5)
            drams.emplace_back(std::make_unique<DramGDDR5>(("dram" + std::to_string(i)).c_str(), config,
                                                           *temperatureController));
        else if (memoryType == MemSpec::MemoryType::GDDR5X)
            drams.emplace_back(std::make_unique<DramGDDR5X>(("dram" + std::to_string(i)).c_str(), config,
                                                            *temperatureController));
        else if (memoryType == MemSpec::MemoryType::GDDR6)
            drams.emplace_back(std::make_unique<DramGDDR6>(("dram" + std::to_string(i)).c_str(), config,
                                                           *temperatureController));
        else if (memoryType == MemSpec::MemoryType::STTMRAM)
            drams.emplace_back(std::make_unique<DramSTTMRAM>(("dram" + std::to_string(i)).c_str(), config,
                                                             *temperatureController));

        if (config.checkTLM2Protocol)
            controllersTlmCheckers.push_back(std::make_unique<tlm_utils::tlm2_base_protocol_checker<>>
                    (("TlmCheckerController" + std::to_string(i)).c_str()));
    }
}

void DRAMSys::bindSockets()
{
    tSocket.bind(arbiter->tSocket);

    for (unsigned i = 0; i < config.memSpec->numberOfChannels; i++)
    {
        if (config.checkTLM2Protocol)
        {
            arbiter->iSocket.bind(controllersTlmCheckers[i]->target_socket);
            controllersTlmCheckers[i]->initiator_socket.bind(controllers[i]->tSocket);
        }
        else
        {
            arbiter->iSocket.bind(controllers[i]->tSocket);
        }
        controllers[i]->iSocket.bind(drams[i]->tSocket);
    }
}

void DRAMSys::report(const std::string &message)
{
    PRINTDEBUGMESSAGE(name(), message);
    std::cout << message << std::endl;
}
