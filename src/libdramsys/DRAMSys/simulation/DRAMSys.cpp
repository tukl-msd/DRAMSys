/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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

#include "DRAMSys.h"

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/utils.h"
#include "DRAMSys/controller/Controller.h"
#include "DRAMSys/simulation/dram/DramDDR3.h"
#include "DRAMSys/simulation/dram/DramDDR4.h"
#include "DRAMSys/simulation/dram/DramGDDR5.h"
#include "DRAMSys/simulation/dram/DramGDDR5X.h"
#include "DRAMSys/simulation/dram/DramGDDR6.h"
#include "DRAMSys/simulation/dram/DramHBM2.h"
#include "DRAMSys/simulation/dram/DramLPDDR4.h"
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

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace DRAMSys
{

DRAMSys::DRAMSys(const sc_core::sc_module_name& name,
                 const ::DRAMSys::Config::Configuration& configLib) :
    DRAMSys(name, configLib, true)
{
}

DRAMSys::DRAMSys(const sc_core::sc_module_name& name,
                 const ::DRAMSys::Config::Configuration& configLib,
                 bool initAndBind) :
    sc_module(name),
    tSocket("DRAMSys_tSocket")
{
    logo();

    // Load configLib and initialize modules
    // Important: The memSpec needs to be the first configuration to be loaded!
    config.loadMemSpec(configLib.memspec);
    config.loadMCConfig(configLib.mcconfig);
    config.loadSimConfig(configLib.simconfig);

    // Setup the debug manager:
    setupDebugManager(config.simulationName);

    if (initAndBind)
    {
        // Instantiate all internal DRAMSys modules:
        instantiateModules(configLib.addressmapping);
        // Connect all internal DRAMSys modules:
        bindSockets();
        report(headline);
    }
}

bool DRAMSys::idle() const
{
    return std::all_of(controllers.cbegin(),
                       controllers.cend(),
                       [](auto const& controller) { return controller->idle(); });
}

void DRAMSys::registerIdleCallback(const std::function<void()>& idleCallback)
{
    for (auto& controller : controllers)
    {
        controller->registerIdleCallback(idleCallback);
    }
}

const Configuration& DRAMSys::getConfig() const
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
#define GREENTXT(s) std::string(("\u001b[38;5;28m" + std::string((s)) + "\033[0m"))
#define DGREENTXT(s) std::string(("\u001b[38;5;22m" + std::string((s)) + "\033[0m"))
#define LGREENTXT(s) std::string(("\u001b[38;5;82m" + std::string((s)) + "\033[0m"))
#define BLACKTXT(s) std::string(("\u001b[38;5;232m" + std::string((s)) + "\033[0m"))
#define BOLDTXT(s) std::string(("\033[1;37m" + std::string((s)) + "\033[0m"))
    cout << std::endl
         << BLACKTXT("■ ■ ") << DGREENTXT("■  ") << BOLDTXT("DRAMSys5.0, Copyright (c) 2023")
         << std::endl
         << BLACKTXT("■ ") << DGREENTXT("■ ") << GREENTXT("■  ") << "RPTU Kaiserslautern-Landau,"
         << std::endl
         << DGREENTXT("■ ") << GREENTXT("■ ") << LGREENTXT("■  ") << "Fraunhofer IESE" << std::endl
         << std::endl;
#undef GREENTXT
#undef DGREENTXT
#undef LGREENTXT
#undef BLACKTXT
#undef BOLDTXT
}

void DRAMSys::setupDebugManager([[maybe_unused]] const std::string& traceName) const
{
#ifndef NDEBUG
    auto& dbg = DebugManager::getInstance();
    bool debugEnabled = config.debug;
    bool writeToConsole = false;
    bool writeToFile = true;
    dbg.setup(debugEnabled, writeToConsole, writeToFile);
    if (debugEnabled && writeToFile)
        dbg.openDebugFile(traceName + ".txt");
#endif
}

void DRAMSys::instantiateModules(const ::DRAMSys::Config::AddressMapping& addressMapping)
{
    addressDecoder = std::make_unique<AddressDecoder>(addressMapping, *config.memSpec);
    addressDecoder->print();

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
        controllers.emplace_back(std::make_unique<Controller>(
            ("controller" + std::to_string(i)).c_str(), config, *addressDecoder));

        if (memoryType == MemSpec::MemoryType::DDR3)
            drams.emplace_back(
                std::make_unique<DramDDR3>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::DDR4)
            drams.emplace_back(
                std::make_unique<DramDDR4>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::WideIO)
            drams.emplace_back(
                std::make_unique<DramWideIO>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::LPDDR4)
            drams.emplace_back(
                std::make_unique<DramLPDDR4>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::WideIO2)
            drams.emplace_back(
                std::make_unique<DramWideIO2>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::HBM2)
            drams.emplace_back(
                std::make_unique<DramHBM2>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::GDDR5)
            drams.emplace_back(
                std::make_unique<DramGDDR5>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::GDDR5X)
            drams.emplace_back(
                std::make_unique<DramGDDR5X>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::GDDR6)
            drams.emplace_back(
                std::make_unique<DramGDDR6>(("dram" + std::to_string(i)).c_str(), config));
        else if (memoryType == MemSpec::MemoryType::STTMRAM)
            drams.emplace_back(
                std::make_unique<DramSTTMRAM>(("dram" + std::to_string(i)).c_str(), config));
#ifdef DDR5_SIM
        else if (memoryType == MemSpec::MemoryType::DDR5)
            drams.emplace_back(
                std::make_unique<DramDDR5>(("dram" + std::to_string(i)).c_str(), config));
#endif
#ifdef LPDDR5_SIM
        else if (memoryType == MemSpec::MemoryType::LPDDR5)
            drams.emplace_back(
                std::make_unique<DramLPDDR5>(("dram" + std::to_string(i)).c_str(), config));
#endif
#ifdef HBM3_SIM
        else if (memoryType == MemSpec::MemoryType::HBM3)
            drams.emplace_back(
                std::make_unique<DramHBM3>(("dram" + std::to_string(i)).c_str(), config));
#endif

        if (config.checkTLM2Protocol)
            controllersTlmCheckers.push_back(
                std::make_unique<tlm_utils::tlm2_base_protocol_checker<>>(
                    ("TlmCheckerController" + std::to_string(i)).c_str()));
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

void DRAMSys::report(std::string_view message)
{
    PRINTDEBUGMESSAGE(name(), message.data());
    std::cout << message << std::endl;
}

} // namespace DRAMSys
