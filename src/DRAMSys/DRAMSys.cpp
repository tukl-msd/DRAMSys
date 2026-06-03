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

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/DramATRecorder.h"
#include "DRAMSys/common/StandardMapping.h"
#include "DRAMSys/common/TlmATRecorder.h"
#include "DRAMSys/common/utils.h"
#include "DRAMSys/configuration/json/MemSpec.h"
#include "DRAMSys/controller/Controller.h"
#include "DRAMSys/controller/McConfig.h"
#include "DRAMSys/simulation/AddressDecoder.h"
#include "DRAMSys/simulation/Arbiter.h"
#include "DRAMSys/simulation/Dram.h"
#include "DRAMSys/simulation/SimConfig.h"

#include "DRAMSys/configuration/memspec/MemSpecDDR3.h"    // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecDDR4.h"    // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecGDDR5.h"   // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecGDDR5X.h"  // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecGDDR6.h"   // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecHBM2.h"    // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecLPDDR4.h"  // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecSTTMRAM.h" // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecWideIO.h"  // IWYU pragma: keep
#include "DRAMSys/configuration/memspec/MemSpecWideIO2.h" // IWYU pragma: keep
#ifdef DDR5_SIM
#include "DRAMSys/configuration/memspec/MemSpecDDR5.h" // IWYU pragma: keep
#endif
#ifdef LPDDR5_SIM
#include "DRAMSys/configuration/memspec/MemSpecLPDDR5.h" // IWYU pragma: keep
#endif
#ifdef LPDDR6_SIM
#include "DRAMSys/configuration/memspec/MemSpecLPDDR6.h" // IWYU pragma: keep
#endif
#ifdef HBM3_4_SIM
#include "DRAMSys/configuration/memspec/MemSpecHBM3_4.h" // IWYU pragma: keep
#endif

#ifdef USE_DRAMPOWER
#include "DRAMSys/power/DRAMPowerAdapter.h"
#endif

#include <DRAMUtils/memspec/MemSpec.h>

#include <fmt/base.h>

#include <cstdlib>
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
    arbiter(createArbiter(*simConfig, *mcConfig, *memSpec, *addressDecoder)),
    dram(std::make_unique<Dram>(memSpec->getSimMemSizeInBytes()))
{
    fmt::print(LOGO, DRAMSYS_VERSION, DRAMSYS_YEAR);
    fmt::println(headline);
    memSpec->print();
    fmt::println(headline);
    addressDecoder->print();
    fmt::println(headline);

    addressDecoder->plausibilityCheck(*memSpec);

    // Setup the debug manager:
    setupDebugManager(simConfig->simulationName);

#ifndef USE_DRAMPOWER
    if (simConfig->powerAnalysis)
        SC_REPORT_FATAL("DRAMSys", "Power Analysis only supported when building with DRAMPower!");
#endif

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
                                             config.memspec,
                                             *memSpec,
                                             *simConfig,
                                             *addressDecoder,
                                             &tlmRecorders[i]));

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
                                             config.memspec,
                                             *memSpec,
                                             *simConfig,
                                             *addressDecoder,
                                             nullptr));
        }
    }

    // Create DRAMPowerAdapters
#ifdef USE_DRAMPOWER
    if (simConfig->powerAnalysis)
    {
        createDRAMPowers(config.memspec);
    }
#endif

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

        auto traceCallback = [this, i](tlm::tlm_generic_payload const& trans,
                                       tlm::tlm_phase const& phase,
                                       sc_core::sc_time const& delay)
        {
            if (simConfig->databaseRecording)
                dramATRecorders[i]->record(trans, phase, delay);

#ifdef USE_DRAMPOWER
            if (simConfig->powerAnalysis)
                DRAMPowers[i]->handleTransaction(i, trans, phase, delay);
#endif
        };

        if (simConfig->databaseRecording || simConfig->powerAnalysis)
        {
            controllers[i]->registerTraceCallback(traceCallback);
        }

        if (simConfig->storageEnabled)
        {
            controllers[i]->registerAccessCallback([this](tlm::tlm_generic_payload& trans)
                                                   { dram->access(trans); });
        }
    }
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

std::uint64_t DRAMSys::memorySize() const
{
    return memSpec->getSimMemSizeInBytes();
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
#ifdef USE_DRAMPOWER
    if (simConfig->powerAnalysis)
    {
        for (auto& DRAMPower : DRAMPowers)
            DRAMPower->reportPower();
    }
#endif

    if (simConfig->databaseRecording)
    {
        for (auto& tlmRecorder : tlmRecorders)
            tlmRecorder.finalize();
    }
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

std::unique_ptr<const MemSpec>
DRAMSys::createMemSpec(const DRAMUtils::MemSpec::MemSpecVariant& memSpec)
{
    return std::visit(
        [](const auto& v) -> std::unique_ptr<const MemSpec>
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (StandardMapping::has_MemSpecType_v<T>) {
                using Impl = typename StandardMapping::Mapping<T>::MemSpecType;
                return std::make_unique<const Impl>(v);
            }
            SC_REPORT_FATAL("Configuration", ("Unsupported DRAM type: " + std::string(T::id)).c_str());
            return nullptr;
        },
        memSpec.getVariant());
}

#ifdef USE_DRAMPOWER
void DRAMSys::createDRAMPowers(const DRAMUtils::MemSpec::MemSpecVariant& memSpecVar)
{
    // DRAMPower SimConfig
    DRAMPower::config::SimConfig drampowerSimConfig;
    drampowerSimConfig.toggleRateDefinition =
        simConfig->storageEnabled ? std::nullopt : simConfig->togglingRate;

    // DRAMPowerAdapter generator
    auto generator = [this, &memSpecVar, &drampowerSimConfig] (std::size_t channel, TlmRecorder* recorder) -> std::unique_ptr<DRAMPowerAdapter> {
        std::unique_ptr<DRAMPowerAdapter> adapter = std::visit(
            [this, &drampowerSimConfig, recorder, channel](const auto& var) -> std::unique_ptr<DRAMPowerAdapter> {
            using T = std::decay_t<decltype(var)>;
            if constexpr (StandardMapping::has_PowerType_v<T>) {
                using Impl = typename StandardMapping::Mapping<T>::PowerType;
                return std::make_unique<DRAMPowerAdapter>(
                    ("drampoweradapter" + std::to_string(channel)).c_str(), Impl(var, drampowerSimConfig),
                    *simConfig, *memSpec, recorder
                );
            }
            SC_REPORT_FATAL("DRAMSys", "Standard does not support the power analysis");
            return nullptr;
        }, memSpecVar.getVariant());
        return adapter;
    };

    // Create DRAMPowerAdapters
    for (std::size_t i = 0; i < memSpec->numberOfChannels; ++i)
    {
        auto* recorder = simConfig->databaseRecording ? &tlmRecorders[i] : nullptr;
        DRAMPowers.emplace_back(generator(i, recorder));
    }
}
#endif

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

void DRAMSys::serialize(std::filesystem::path const& checkpointPath) const
{
    std::function<void(sc_core::sc_object const*)> serialize;
    serialize = [&serialize, &checkpointPath](sc_core::sc_object const* object)
    {
        auto const* serializableObject = dynamic_cast<::DRAMSys::Serialize const*>(object);

        if (serializableObject != nullptr)
        {
            std::string dumpFileName(object->name());
            std::ofstream stream(checkpointPath / dumpFileName, std::ios::binary);
            serializableObject->serialize(stream);
        }

        for (auto const* childObject : object->get_child_objects())
        {
            serialize(childObject);
        }
    };

    serialize(this);
}

void DRAMSys::deserialize(std::filesystem::path const& checkpointPath)
{
    std::function<void(sc_core::sc_object*)> deserialize;
    deserialize = [&deserialize, &checkpointPath](sc_core::sc_object* object)
    {
        auto* deserializableObject = dynamic_cast<::DRAMSys::Deserialize*>(object);

        if (deserializableObject != nullptr)
        {
            std::string dumpFileName(object->name());
            std::ifstream stream(checkpointPath / dumpFileName, std::ios::binary);
            deserializableObject->deserialize(stream);
        }

        for (auto* childObject : object->get_child_objects())
        {
            deserialize(childObject);
        }
    };

    deserialize(this);
}

} // namespace DRAMSys
