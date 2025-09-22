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
 *    Marco Mörz
 */

#include "DRAMSys.h"

#include "DRAMSys/controller/Controller.h"
#include "DRAMSys/controller/McConfig.h"
#include "DRAMSys/simulation/AddressDecoder.h"
#include "DRAMSys/simulation/Arbiter.h"
#include "DRAMSys/simulation/SimConfig.h"
#include <DRAMUtils/memspec/MemSpec.h>

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/DramATRecorder.h"
#include "DRAMSys/common/TlmATRecorder.h"
#include "DRAMSys/common/utils.h"
#include "DRAMSys/configuration/json/MemSpec.h"
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
#include "DRAMSys/simulation/Dram.h"

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
#include <type_traits>
#include <variant>
#include <vector>

namespace DRAMSys
{

DRAMSys::DRAMSys(const sc_core::sc_module_name& name, const Config::Configuration& config) :
    sc_module(name),
    memSpec(createMemSpec(config.memspec)),
    simConfig(std::make_unique<SimConfig>(config.simconfig)),
    mcConfig(std::make_unique<McConfig>(config.mcconfig, *memSpec)),
    addressDecoder(std::make_unique<AddressDecoder>(config.addressmapping)),
    arbiter(createArbiter(*simConfig, *mcConfig, *memSpec, *addressDecoder))
{
    pim_vm::init_logger();
    logo();
    addressDecoder->plausibilityCheck(*memSpec);
    addressDecoder->print();

    // Setup the debug manager:
    setupDebugManager(simConfig->simulationName);

    // Instantiate all internal DRAMSys modules:
    if (simConfig->databaseRecording)
    {
        std::string traceName = simConfig->simulationName;
        if (!config.simulationid.empty())
            traceName = config.simulationid + '_' + traceName;

        // Create and properly initialize TLM recorders.
        // They need to be ready before creating some modules.
        setupTlmRecorders(traceName, config);

        // Create controllers and DRAMs
        for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
        {
            controllers.emplace_back(
                std::make_unique<Controller>(("controller" + std::to_string(i)).c_str(),
                                             *mcConfig,
                                             *memSpec,
                                             *simConfig,
                                             *addressDecoder,
                                             &tlmRecorders[i]));

            drams.emplace_back(std::make_unique<Dram>(
                ("dram" + std::to_string(i)).c_str(), *simConfig, *memSpec, *addressDecoder, &tlmRecorders[i]));

            // Not recording bandwidth between Arbiter - Controller
            tlmATRecorders.emplace_back(
                std::make_unique<TlmATRecorder>(("TlmATRecorder" + std::to_string(i)).c_str(),
                                                *simConfig,
                                                *memSpec,
                                                tlmRecorders[i],
                                                false));

            // Recording bandwidth between Controller - DRAM
            dramATRecorders.emplace_back(
                std::make_unique<DramATRecorder>(("DramATRecorder" + std::to_string(i)).c_str(),
                                                 *simConfig,
                                                 *memSpec,
                                                 tlmRecorders[i],
                                                 true));
        }
    }
    else
    {
        for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
        {
            controllers.emplace_back(
                std::make_unique<Controller>(("controller" + std::to_string(i)).c_str(),
                                             *mcConfig,
                                             *memSpec,
                                             *simConfig,
                                             *addressDecoder,
                                             nullptr));

            drams.emplace_back(std::make_unique<Dram>(
                ("dram" + std::to_string(i)).c_str(), *simConfig, *memSpec, *addressDecoder, nullptr));
        }
    }

    // Connect all internal DRAMSys modules:
    // If database recording is enabled, then the tlmRecorders are placed
    // on the bus between the modules
    tSocket.bind(arbiter->tSocket);
    for (unsigned i = 0; i < memSpec->numberOfChannels; i++)
    {
        if (simConfig->databaseRecording)
        {
            arbiter->iSocket.bind(tlmATRecorders[i]->tSocket);
            tlmATRecorders[i]->iSocket.bind(controllers[i]->tSocket);
        }
        else
        {
            arbiter->iSocket.bind(controllers[i]->tSocket);
        }

        if (simConfig->databaseRecording)
        {
            // Controller <--> tlmRecorder <--> Dram
            controllers[i]->iSocket.bind(dramATRecorders[i]->tSocket);
            dramATRecorders[i]->iSocket.bind(drams[i]->tSocket);
        }
        else
        {
            // Controller <--> Dram
            controllers[i]->iSocket.bind(drams[i]->tSocket);
        }
    }

    report();
}

DRAMSys::~DRAMSys() = default;

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
        memspec[Config::MemSpecConstants::KEY] = config.memspec;

        tlmRecorders.emplace_back(recorderName,
                                  *simConfig,
                                  *mcConfig,
                                  *memSpec,
                                  dbName,
                                  mcconfig.dump(),
                                  memspec.dump(),
                                  simConfig->simulationName);
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
    if (simConfig->powerAnalysis)
    {
        for (auto& dram : drams)
            dram->reportPower();
    }

    if (simConfig->databaseRecording)
    {
        for (auto& tlmRecorder : tlmRecorders)
            tlmRecorder.finalize();
    }
}

void DRAMSys::logo()
{
#define GREENTXT(s) std::string(("\u001b[38;5;28m" + std::string((s)) + "\033[0m"))
#define DGREENTXT(s) std::string(("\u001b[38;5;22m" + std::string((s)) + "\033[0m"))
#define LGREENTXT(s) std::string(("\u001b[38;5;82m" + std::string((s)) + "\033[0m"))
#define BLACKTXT(s) std::string(("\u001b[38;5;232m" + std::string((s)) + "\033[0m"))
#define BOLDTXT(s) std::string(("\033[1;37m" + std::string((s)) + "\033[0m"))
    std::cout << std::endl
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
    bool debugEnabled = simConfig->debug;
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

std::unique_ptr<const MemSpec>
DRAMSys::createMemSpec(const DRAMUtils::MemSpec::MemSpecVariant& memSpec)
{
    return std::visit(
        [](const auto& v) -> std::unique_ptr<const MemSpec>
        {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecDDR3>)
                return std::make_unique<const MemSpecDDR3>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecDDR4>)
                return std::make_unique<const MemSpecDDR4>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecLPDDR4>)
                return std::make_unique<const MemSpecLPDDR4>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecWideIO>)
                return std::make_unique<const MemSpecWideIO>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecWideIO2>)
                return std::make_unique<const MemSpecWideIO2>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecHBM2>)
                return std::make_unique<const MemSpecHBM2>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecGDDR5>)
                return std::make_unique<const MemSpecGDDR5>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecGDDR5X>)
                return std::make_unique<const MemSpecGDDR5X>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecGDDR6>)
                return std::make_unique<const MemSpecGDDR6>(v);
            else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecSTTMRAM>)
                return std::make_unique<const MemSpecSTTMRAM>(v);
#ifdef DDR5_SIM
            else if constexpr ((std::is_same_v<T, DRAMUtils::MemSpec::MemSpecDDR5>))
                return std::make_unique<const MemSpecDDR5>(v);
#endif
#ifdef LPDDR5_SIM
            else if constexpr ((std::is_same_v<T, DRAMUtils::MemSpec::MemSpecLPDDR5>))
                return std::make_unique<const MemSpecLPDDR5>(v);
#endif
#ifdef HBM3_SIM
            else if constexpr ((std::is_same_v<T, DRAMUtils::MemSpec::MemSpecHBM3>))
                return std::make_unique<const MemSpecHBM3>(v);
#endif
            else
            {
                SC_REPORT_FATAL("Configuration", "Unsupported DRAM type");
                return nullptr;
            }
        },
        memSpec.getVariant());
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
