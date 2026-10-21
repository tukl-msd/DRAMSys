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
 *    Thomas Zimmermann
 */

#include "DRAMSys.h"

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/DramATRecorder.h"
#include "DRAMSys/common/StandardMapping.h"
#include "DRAMSys/common/TlmATRecorder.h"
#include "DRAMSys/common/utils.h"
#include "DRAMSys/controller/Controller.h"
#include "DRAMSys/controller/McConfig.h"
#include "DRAMSys/controller/checker/CheckerDDR3.h"    // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerDDR4.h"    // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerGDDR5.h"   // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerGDDR5X.h"  // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerGDDR6.h"   // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerHBM2.h"    // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerLPDDR4.h"  // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerSTTMRAM.h" // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerWideIO.h"  // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerWideIO2.h" // IWYU pragma: keep
#include "DRAMSys/simulation/AddressDecoder.h"
#include "DRAMSys/simulation/AddressMapping.h"
#include "DRAMSys/simulation/Arbiter.h"
#include "DRAMSys/simulation/Dram.h"
#include "DRAMSys/simulation/Phy.h"
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
#include "DRAMSys/controller/checker/CheckerDDR5.h"    // IWYU pragma: keep
#endif
#ifdef LPDDR5_SIM
#include "DRAMSys/configuration/memspec/MemSpecLPDDR5.h" // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerLPDDR5.h"    // IWYU pragma: keep
#endif
#ifdef LPDDR6_SIM
#include "DRAMSys/configuration/memspec/MemSpecLPDDR6.h" // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerLPDDR6.h"    // IWYU pragma: keep
#endif
#ifdef HBM3_4_SIM
#include "DRAMSys/configuration/memspec/MemSpecHBM3_4.h" // IWYU pragma: keep
#include "DRAMSys/controller/checker/CheckerHBM3_4.h"    // IWYU pragma: keep
#endif

#ifdef USE_DRAMPOWER
#include "DRAMSys/power/DRAMPowerAdapter.h"
#else
namespace DRAMSys
{
class DRAMPowerAdapter
{
};
} // namespace DRAMSys
#endif

#include <DRAMUtils/memspec/MemSpec.h>

#include <fmt/base.h>
#include <fmt/format.h>

#include <cstdlib>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

namespace DRAMSys
{

DRAMSys::DRAMSys(const sc_core::sc_module_name& name, const Config::Configuration& config) :
    DRAMSys(name,
            config.simulationid,
            SimConfig(config.simconfig),
            McConfig(config.mcconfig, createMemSpec(config.memspec)->tCK),
            config.memspec,
            config.addressmapping)
{
    for (auto& recorder : tlmRecorders)
    {
        nlohmann::json mcconfig_json;
        nlohmann::json memspec_json;
        mcconfig_json["mcconfig"] = config.mcconfig;
        memspec_json["memspec"] = config.memspec;
        recorder.insertGeneralInfo(
            this->simConfig->simulationName, mcconfig_json.dump(), memspec_json.dump());
    }
}

DRAMSys::DRAMSys(const sc_core::sc_module_name& name,
                 std::string simid,
                 SimConfig const& simconfig,
                 McConfig const& mcconfig,
                 DRAMUtils::MemSpec::MemSpecVariant const& memspec,
                 AddressMapping const& addressmapping) :
    sc_module(name),
    memSpec(createMemSpec(memspec)),
    simConfig(std::make_unique<SimConfig>(simconfig)),
    mcConfig(std::make_unique<McConfig>(mcconfig)),
    addressDecoder(std::make_unique<AddressDecoder>(addressmapping)),
    arbiter(createArbiter(*simConfig, *mcConfig, *memSpec, *addressDecoder)),
    dram(std::make_unique<Dram>("Dram",
                                &backingStore,
                                memSpec.get(),
                                simconfig.databaseRecording,
                                &dramATRecorders,
                                simconfig.powerAnalysis,
                                &DRAMPowerMappings)),
    checker(createChecker(*memSpec)),
    phy(std::make_unique<Phy>("Phy", &phyATRecorders, simconfig.databaseRecording, simconfig.efficiencyMode)),
    stats(*this)
{
    printLogo();
    fmt::println(headline);
    memSpec->print();
    fmt::println(headline);
    addressDecoder->print();
    fmt::println(headline);

    if (!addressDecoder->plausibilityCheck(*memSpec)) {
        SC_REPORT_FATAL("DRAMSys", "AddressDecoder configuration not plausible!");
    }

    // Setup the debug manager:
    setupDebugManager(simConfig->simulationName);

#ifndef USE_DRAMPOWER
    if (simConfig->powerAnalysis)
        SC_REPORT_FATAL("DRAMSys", "Power Analysis only supported when building with DRAMPower!");
#endif

    // Prepare TimingChecker for controllers

    // Instantiate all internal DRAMSys modules:
    if (simConfig->databaseRecording)
    {
        std::string traceName = simConfig->simulationName;
        traceName = simid + '_' + traceName;

        // Create and properly initialize TLM recorders.
        // They need to be ready before creating some modules.
        setupTlmRecorders(traceName);

        // Create controllers and DRAM
        for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
        {
            controllers.emplace_back(
                std::make_unique<Controller>(("controller" + std::to_string(i)).c_str(),
                                             static_cast<int>(i),
                                             *mcConfig,
                                             *memSpec,
                                             *simConfig,
                                             *addressDecoder,
                                             *checker,
                                             &tlmRecorders[i])
            );

            // Not recording bandwidth between Arbiter - Controller
            tlmATRecorders.emplace_back(
                std::make_unique<TlmATRecorder>(("TlmATRecorder" + std::to_string(i)).c_str(),
                                                *simConfig,
                                                *memSpec,
                                                tlmRecorders[i],
                                                false));

            // Recording bandwidth between Controller - PHY
            phyATRecorders.emplace_back(
                std::make_unique<DramATRecorder>(("phyATRecorder" + std::to_string(i)).c_str(),
                                                 *simConfig,
                                                 *memSpec,
                                                 tlmRecorders[i],
                                                 true,
                                                 false));

            // Recording bandwidth between PHY - DRAM
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
                                             static_cast<int>(i),
                                             *mcConfig,
                                             *memSpec,
                                             *simConfig,
                                             *addressDecoder,
                                             *checker,
                                             nullptr)
            );
        }
    }

    // Create DRAMPowerAdapters
#ifdef USE_DRAMPOWER
    if (simConfig->powerAnalysis)
    {
        createDRAMPowers(memspec);
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

        controllers[i]->iSocket.bind(phy->tSocket);
        phy->iSocket.bind(dram->tSocket);
        
    }
}

DRAMSys::~DRAMSys() = default;

void DRAMSys::setupTlmRecorders(const std::string& traceName)
{
    // Create TLM Recorders, one per channel.
    // Reserve is required because the recorders use double buffers that are accessed with pointers.
    // Without a reserve, the vector reallocates storage before inserting a second
    // element and the pointers are not valid anymore.
    tlmRecorders.reserve(memSpec->numberOfChannels);
    for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
    {
        std::string dbName =
            std::string("DRAMSys") + "_" + traceName + "_ch" + std::to_string(i) + ".tdb";
        std::string recorderName = "tlmRecorder" + std::to_string(i);

        auto& recorder =
            tlmRecorders.emplace_back(recorderName, *simConfig, *mcConfig, *memSpec, dbName);
        recorder.insertGeneralInfo(this->simConfig->simulationName);
    }
}

void DRAMSys::setBackingStore(unsigned char* backingStore)
{
    this->backingStore = backingStore;
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

void DRAMSys::createDRAMPowers(
    [[maybe_unused]] const DRAMUtils::MemSpec::MemSpecVariant& memSpecVar)
{
#ifdef USE_DRAMPOWER
    // DRAMPower SimConfig
    DRAMPower::config::SimConfig drampowerSimConfig;
    drampowerSimConfig.toggleRateDefinition =
        simConfig->storageEnabled ? std::nullopt : simConfig->togglingRate;

    // DRAMPowerAdapter generator
    auto generator = [this, &memSpecVar, &drampowerSimConfig](
                         std::size_t channel,
                         TlmRecorder* recorder) -> std::unique_ptr<DRAMPowerAdapter>
    {
        std::unique_ptr<DRAMPowerAdapter> adapter = std::visit(
            [this, &drampowerSimConfig, recorder, channel](
                const auto& var) -> std::unique_ptr<DRAMPowerAdapter>
            {
                using T = std::decay_t<decltype(var)>;
                if constexpr (StandardMapping::has_PowerType_v<T>)
                {

                    using Impl = typename StandardMapping::Mapping<T>::PowerType;
                    return std::make_unique<DRAMPowerAdapter>(
                        ("drampoweradapter" + std::to_string(channel)).c_str(),
                        Impl(var, drampowerSimConfig),
                        *simConfig,
                        *memSpec,
                        recorder);
                }
                SC_REPORT_FATAL("DRAMSys", "Standard does not support the power analysis");
                return nullptr;
            },
            memSpecVar.getVariant());
        return adapter;
    };

    // Create DRAMPowerAdapters
    DRAMPowerAdapter *lastDRAMPower = nullptr;
    for (std::size_t i = 0; i < memSpec->numberOfChannels; ++i)
    {
        auto* recorder = simConfig->databaseRecording ? &tlmRecorders[i] : nullptr;
        if (auto drampower = generator(i, recorder)) {
            lastDRAMPower = drampower.get();
            DRAMPowers.emplace_back(std::move(drampower));
        }
        assert((!(0 == i) || nullptr != lastDRAMPower) && "The first channel must produce a DRAMPowerAdapter object");
        DRAMPowerMappings.emplace_back(lastDRAMPower);
    }
#endif
}

std::unique_ptr<Arbiter> DRAMSys::createArbiter(const SimConfig& simConfig,
                                                const McConfig& mcConfig,
                                                const MemSpec& memSpec,
                                                const AddressDecoder& addressDecoder)
{
    if (mcConfig.arbiter == McConfig::Arbiter::Simple)
        return std::make_unique<ArbiterSimple>(
            "arbiter", simConfig, mcConfig, memSpec, addressDecoder);

    if (mcConfig.arbiter == McConfig::Arbiter::Fifo)
        return std::make_unique<ArbiterFifo>(
            "arbiter", simConfig, mcConfig, memSpec, addressDecoder);

    if (mcConfig.arbiter == McConfig::Arbiter::Reorder)
        return std::make_unique<ArbiterReorder>(
            "arbiter", simConfig, mcConfig, memSpec, addressDecoder);

    SC_REPORT_FATAL("DRAMSys", "Invalid Arbiter");
    return {};
}

std::unique_ptr<CheckerIF> DRAMSys::createChecker(MemSpec const& memSpec)
{
    std::unique_ptr<CheckerIF> checker;

    // instantiate timing checker
    if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecDDR3::id)
    {
        checker = std::make_unique<CheckerDDR3>(dynamic_cast<const MemSpecDDR3&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecDDR4::id)
    {
        checker = std::make_unique<CheckerDDR4>(dynamic_cast<const MemSpecDDR4&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecWideIO::id)
    {
        checker = std::make_unique<CheckerWideIO>(dynamic_cast<const MemSpecWideIO&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecLPDDR4::id)
    {
        checker = std::make_unique<CheckerLPDDR4>(dynamic_cast<const MemSpecLPDDR4&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecWideIO2::id)
    {
        checker = std::make_unique<CheckerWideIO2>(dynamic_cast<const MemSpecWideIO2&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecHBM2::id)
    {
        checker = std::make_unique<CheckerHBM2>(dynamic_cast<const MemSpecHBM2&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecGDDR5::id)
    {
        checker = std::make_unique<CheckerGDDR5>(dynamic_cast<const MemSpecGDDR5&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecGDDR5X::id)
    {
        checker = std::make_unique<CheckerGDDR5X>(dynamic_cast<const MemSpecGDDR5X&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecGDDR6::id)
    {
        checker = std::make_unique<CheckerGDDR6>(dynamic_cast<const MemSpecGDDR6&>(memSpec));
    }
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecSTTMRAM::id)
    {
        checker = std::make_unique<CheckerSTTMRAM>(dynamic_cast<const MemSpecSTTMRAM&>(memSpec));
    }
#ifdef DDR5_SIM
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecDDR5::id)
    {
        checker = std::make_unique<CheckerDDR5>(dynamic_cast<const MemSpecDDR5&>(memSpec));
    }
#endif
#ifdef LPDDR5_SIM
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecLPDDR5::id)
    {
        checker = std::make_unique<CheckerLPDDR5>(dynamic_cast<const MemSpecLPDDR5&>(memSpec));
    }
#endif
#ifdef LPDDR6_SIM
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecLPDDR6::id)
    {
        checker = std::make_unique<CheckerLPDDR6>(dynamic_cast<const MemSpecLPDDR6&>(memSpec));
    }
#endif
#ifdef HBM3_4_SIM
    else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecHBM3::id ||
             memSpec.memoryType == DRAMUtils::MemSpec::MemSpecHBM4::id)
    {
        checker = std::make_unique<CheckerHBM3_4>(dynamic_cast<const MemSpecHBM3_4&>(memSpec));
    }
#endif
    else
    {
        SC_REPORT_FATAL("DRAMSys", "Wrong MemSpec chosen");
        return nullptr;
    }

    return checker;
}

void DRAMSys::serialize(std::filesystem::path const& checkpointPath) const
{
    std::ofstream stream(checkpointPath / name(), std::ios::binary);

    for (auto const& controller : controllers)
    {
        controller->serialize(stream);
    }
}

void DRAMSys::deserialize(std::filesystem::path const& checkpointPath)
{
    std::ifstream stream(checkpointPath / name(), std::ios::binary);

    for (auto const& controller : controllers)
    {
        controller->deserialize(stream);
    }
}

} // namespace DRAMSys
