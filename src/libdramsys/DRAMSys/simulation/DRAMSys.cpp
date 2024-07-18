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

#include "DRAMSys/configuration/memspec/MemSpecDDR3.h"
#include "DRAMSys/configuration/memspec/MemSpecDDR4.h"
#include "DRAMSys/configuration/memspec/MemSpecGDDR5.h"
#include "DRAMSys/configuration/memspec/MemSpecGDDR5X.h"
#include "DRAMSys/configuration/memspec/MemSpecGDDR6.h"
#include "DRAMSys/configuration/memspec/MemSpecHBM2.h"
#include "DRAMSys/configuration/memspec/MemSpecLPDDR4.h"
#include "DRAMSys/configuration/memspec/MemSpecSTTMRAM.h"
#include "DRAMSys/configuration/memspec/MemSpecWideIO.h"
#include "DRAMSys/configuration/memspec/MemSpecWideIO2.h"

#ifdef DDR5_SIM
#include "DRAMSys/configuration/memspec/MemSpecDDR5.h"
#endif
#ifdef LPDDR5_SIM
#include "DRAMSys/configuration/memspec/MemSpecLPDDR5.h"
#endif
#ifdef HBM3_SIM
#include "DRAMSys/configuration/memspec/MemSpecHBM3.h"
#endif

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace DRAMSys
{

DRAMSys::DRAMSys(const sc_core::sc_module_name& name, const Config::Configuration& config) :
    sc_module(name),
    memSpec(createMemSpec(config.memspec)),
    simConfig(config.simconfig),
    mcConfig(config.mcconfig, *memSpec),
    addressDecoder(std::make_unique<AddressDecoder>(config.addressmapping)),
    arbiter(createArbiter(simConfig, mcConfig, *memSpec, *addressDecoder))
{
    logo();
    addressDecoder->plausibilityCheck(*memSpec);
    addressDecoder->print();

    // Setup the debug manager:
    setupDebugManager(simConfig.simulationName);

    // Instantiate all internal DRAMSys modules:
    if (simConfig.databaseRecording)
    {
        std::string traceName = simConfig.simulationName;

        if (!config.simulationid.empty())
            traceName = config.simulationid + '_' + traceName;

        // Create and properly initialize TLM recorders.
        // They need to be ready before creating some modules.
        setupTlmRecorders(traceName, config);

        // Create controllers and DRAMs
        for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
        {
            controllers.emplace_back(
                std::make_unique<ControllerRecordable>(("controller" + std::to_string(i)).c_str(),
                                                       mcConfig,
                                                       simConfig,
                                                       *memSpec,
                                                       *addressDecoder,
                                                       tlmRecorders[i]));

            drams.emplace_back(std::make_unique<DramRecordable>(
                ("dram" + std::to_string(i)).c_str(), simConfig, *memSpec, tlmRecorders[i]));

            if (simConfig.checkTLM2Protocol)
                controllersTlmCheckers.emplace_back(
                    std::make_unique<tlm_utils::tlm2_base_protocol_checker<>>(
                        ("TLMCheckerController" + std::to_string(i)).c_str()));
        }
    }
    else
    {
        for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
        {
            controllers.emplace_back(std::make_unique<Controller>(
                ("controller" + std::to_string(i)).c_str(), mcConfig, *memSpec, *addressDecoder));

            drams.emplace_back(
                std::make_unique<Dram>(("dram" + std::to_string(i)).c_str(), simConfig, *memSpec));

            if (simConfig.checkTLM2Protocol)
            {
                controllersTlmCheckers.push_back(
                    std::make_unique<tlm_utils::tlm2_base_protocol_checker<>>(
                        ("TlmCheckerController" + std::to_string(i)).c_str()));
            }
        }
    }

    // Connect all internal DRAMSys modules:
    tSocket.bind(arbiter->tSocket);
    for (unsigned i = 0; i < memSpec->numberOfChannels; i++)
    {
        if (simConfig.checkTLM2Protocol)
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

    report();
}

void DRAMSys::setupTlmRecorders(const std::string& traceName, const Config::Configuration& config)
{
    // Create TLM Recorders, one per channel.
    // Reserve is required because the recorders use double buffers that are accessed with pointers.
    // Without a reserve, the vector reallocates storage before inserting a second
    // element and the pointers are not valid anymore.
    tlmRecorders.reserve(memSpec->numberOfChannels);
    for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
    {
        std::string dbName =
            std::string(name()) + "_" + traceName + "_ch" + std::to_string(i) + ".tdb";
        std::string recorderName = "tlmRecorder" + std::to_string(i);

        nlohmann::json mcconfig;
        nlohmann::json memspec;
        mcconfig[Config::McConfig::KEY] = config.mcconfig;
        memspec[Config::MemSpec::KEY] = config.memspec;

        tlmRecorders.emplace_back(recorderName,
                                  simConfig,
                                  mcConfig,
                                  *memSpec,
                                  dbName,
                                  mcconfig.dump(),
                                  memspec.dump(),
                                  simConfig.simulationName);
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

void DRAMSys::end_of_simulation()
{
    if (simConfig.powerAnalysis)
    {
        for (auto& dram : drams)
            dram->reportPower();
    }

    for (auto& tlmRecorder : tlmRecorders)
        tlmRecorder.finalize();
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
    bool debugEnabled = simConfig.debug;
    bool writeToConsole = false;
    bool writeToFile = true;
    dbg.setup(debugEnabled, writeToConsole, writeToFile);
    if (debugEnabled && writeToFile)
        dbg.openDebugFile(traceName + ".txt");
#endif
}

void DRAMSys::report()
{
    PRINTDEBUGMESSAGE(name(), headline.data());
    std::cout << headline << std::endl;
}

std::unique_ptr<const MemSpec> DRAMSys::createMemSpec(const Config::MemSpec& memSpec)
{
    auto memoryType = memSpec.memoryType;

    if (memoryType == Config::MemoryType::DDR3)
        return std::make_unique<const MemSpecDDR3>(memSpec);

    if (memoryType == Config::MemoryType::DDR4)
        return std::make_unique<const MemSpecDDR4>(memSpec);

    if (memoryType == Config::MemoryType::LPDDR4)
        return std::make_unique<const MemSpecLPDDR4>(memSpec);

    if (memoryType == Config::MemoryType::WideIO)
        return std::make_unique<const MemSpecWideIO>(memSpec);

    if (memoryType == Config::MemoryType::WideIO2)
        return std::make_unique<const MemSpecWideIO2>(memSpec);

    if (memoryType == Config::MemoryType::HBM2)
        return std::make_unique<const MemSpecHBM2>(memSpec);

    if (memoryType == Config::MemoryType::GDDR5)
        return std::make_unique<const MemSpecGDDR5>(memSpec);

    if (memoryType == Config::MemoryType::GDDR5X)
        return std::make_unique<const MemSpecGDDR5X>(memSpec);

    if (memoryType == Config::MemoryType::GDDR6)
        return std::make_unique<const MemSpecGDDR6>(memSpec);

    if (memoryType == Config::MemoryType::STTMRAM)
        return std::make_unique<const MemSpecSTTMRAM>(memSpec);

#ifdef DDR5_SIM
    if (memoryType == Config::MemoryType::DDR5)
        return std::make_unique<const MemSpecDDR5>(memSpec);
#endif

#ifdef LPDDR5_SIM
    if (memoryType == Config::MemoryType::LPDDR5)
        return std::make_unique<const MemSpecLPDDR5>(memSpec);
#endif

#ifdef HBM3_SIM
    if (memoryType == Config::MemoryType::HBM3)
        return std::make_unique<const MemSpecHBM3>(memSpec);
#endif

    SC_REPORT_FATAL("Configuration", "Unsupported DRAM type");
    return {};
}

std::unique_ptr<Arbiter> DRAMSys::createArbiter(const SimConfig& simConfig,
                                                const McConfig& mcConfig,
                                                const MemSpec& memSpec,
                                                const AddressDecoder& addressDecoder)
{
    if (mcConfig.arbiter == Config::ArbiterType::Simple)
        return std::make_unique<ArbiterSimple>(
            "arbiter", simConfig, mcConfig, memSpec, addressDecoder);

    if (mcConfig.arbiter == Config::ArbiterType::Fifo)
        return std::make_unique<ArbiterFifo>(
            "arbiter", simConfig, mcConfig, memSpec, addressDecoder);

    if (mcConfig.arbiter == Config::ArbiterType::Reorder)
        return std::make_unique<ArbiterReorder>(
            "arbiter", simConfig, mcConfig, memSpec, addressDecoder);

    SC_REPORT_FATAL("DRAMSys", "Invalid Arbiter");
    return {};
}

} // namespace DRAMSys
