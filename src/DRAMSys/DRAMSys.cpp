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
#include "DRAMSys/common/StandardMapping.h"
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
    nlohmann::json mcconfigJson;
    mcconfigJson["mcconfig"] = config.mcconfig;
    databaseRecorder.record(Recording::GeneralInfoRecord{"MCconfig", mcconfigJson.dump()});

    nlohmann::json memspecJson;
    memspecJson["memspec"] = config.memspec;
    databaseRecorder.record(Recording::GeneralInfoRecord{"Memspec", memspecJson.dump()});

    if (config.simconfig.WindowSize.has_value())
    {
        databaseRecorder.record(
            Recording::GeneralInfoRecord{"WindowSize", config.simconfig.WindowSize.value()});
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
    arbiter(createArbiter()),
    dram(std::make_unique<Dram>(
        "Dram", &backingStore, memSpec.get(), simconfig.powerAnalysis, &DRAMPowerMappings)),
    checker(createChecker(*memSpec)),
    phy(std::make_unique<Phy>("Phy", simconfig.efficiencyMode)),
    database(simid + ".tdb"),
    databaseRecorder(database),
    stats(*this)
{
    printLogo();
    fmt::println(headline);
    memSpec->print();
    fmt::println(headline);
    addressDecoder->print();
    fmt::println(headline);

    if (!addressDecoder->plausibilityCheck(*memSpec))
    {
        SC_REPORT_FATAL("DRAMSys", "AddressDecoder configuration not plausible!");
    }

    tlm::tlm_phase my_phase(tlm::tlm_phase_enum(0));

    // Setup the debug manager:
    setupDebugManager(simConfig->simulationName);

#ifndef USE_DRAMPOWER
    if (simConfig->powerAnalysis)
        SC_REPORT_FATAL("DRAMSys", "Power Analysis only supported when building with DRAMPower!");
#endif

    // Prepare TimingChecker for controllers

    // Instantiate all internal DRAMSys modules:
    for (std::size_t i = 0; i < memSpec->numberOfChannels; i++)
    {
        controllers.emplace_back(
            std::make_unique<Controller>(("controller" + std::to_string(i)).c_str(),
                                         i,
                                         *mcConfig,
                                         *memSpec,
                                         *simConfig,
                                         *addressDecoder,
                                         *checker));
    }

    // Create DRAMPowerAdapters
#ifdef USE_DRAMPOWER
    if (simConfig->powerAnalysis)
    {
        createDRAMPowers(memspec);
    }
#endif

    // Connect all internal DRAMSys modules
    tSocket.bind(arbiter->tSocket);
    for (unsigned i = 0; i < memSpec->numberOfChannels; i++)
    {
        // Arbiter <--> Controller
        auto controller_bus_record = Recording::SignalRecord{
            fmt::format("{}.channel{}", this->name(), i), Recording::SignalType::TLMBus, i};
        unsigned tlm_bus_id = databaseRecorder.record(controller_bus_record);
        auto monitor = std::make_unique<Recording::TlmMonitor<Recording::DatabaseRecorder>>(
            ("monitor" + std::to_string(i)).c_str(), tlm_bus_id, &databaseRecorder);

        arbiter->iSocket.bind(monitor->targetSocket());
        monitor->initiatorSocket().bind(controllers[i]->tSocket);
        monitors.emplace_back(std::move(monitor));

        auto command_bus_record = Recording::SignalRecord{
            fmt::format("{}.controller{}.cmd", this->name(), i),
            Recording::SignalType::CommandBus,
            i,
        };
        unsigned command_bus_id = databaseRecorder.record(command_bus_record);

        auto data_bus_record = Recording::SignalRecord{
            fmt::format("{}.controller{}.data", this->name(), i),
            Recording::SignalType::DataBus,
            i,
        };
        unsigned data_bus_id = databaseRecorder.record(data_bus_record);

        // Controller <--> Dram
        auto dram_monitor =
            std::make_unique<Recording::DramMonitor>(("dramMonitor" + std::to_string(i)).c_str(),
                                                     command_bus_id,
                                                     data_bus_id,
                                                     simconfig.windowSize * memSpec->tCK,
                                                     memSpec.get(),
                                                     &databaseRecorder);
        controllers[i]->iSocket.bind(phy->tSocket);
        phy->iSocket.bind(dram_monitor->targetSocket());
        dram_monitor->initiatorSocket().bind(dram->tSocket);
        dramMonitors.emplace_back(std::move(dram_monitor));
    }

    // GeneralInfo Table
    databaseRecorder.record(Recording::GeneralInfoRecord{"UnitOfTime", "PS"});
    databaseRecorder.record(Recording::GeneralInfoRecord{
        "Clk", static_cast<std::uint64_t>(memSpec->tCK.to_seconds() * 1e12 + 0.5)});
    databaseRecorder.record(
        Recording::GeneralInfoRecord{"NumberOfRanks", memSpec->ranksPerChannel});
    databaseRecorder.record(
        Recording::GeneralInfoRecord{"NumberOfBankgroups", memSpec->groupsPerRank * memSpec->ranksPerChannel});
    databaseRecorder.record(Recording::GeneralInfoRecord{"NumberOfBanks", memSpec->banksPerRank * memSpec->ranksPerChannel});
    databaseRecorder.record(
        Recording::GeneralInfoRecord{"Per2BankOffset", memSpec->getPer2BankOffset()});
    databaseRecorder.record(
        Recording::GeneralInfoRecord{"RowColumnCommandBus", memSpec->hasRasAndCasBus()});
    databaseRecorder.record(
        Recording::GeneralInfoRecord{"PseudoChannelMode", memSpec->pseudoChannelMode()});

    // Command Table
    for (std::size_t i = 0; i < Command::END_ENUM; ++i)
    {
        Command command(static_cast<Command::Type>(i));
        auto commandName = command.toString();
        auto commandLength = memSpec->getCommandLength(command);
        Recording::CommandRecord commandRecord{command, commandName, commandLength.value()};
        databaseRecorder.record(commandRecord);
    }

    // Phases Table
    databaseRecorder.record(Recording::PhaseRecord{tlm::BEGIN_REQ, "BEGIN_REQ"});
    databaseRecorder.record(Recording::PhaseRecord{tlm::END_REQ, "END_REQ"});
    databaseRecorder.record(Recording::PhaseRecord{tlm::BEGIN_RESP, "BEGIN_RESP"});
    databaseRecorder.record(Recording::PhaseRecord{tlm::END_RESP, "END_RESP"});
}

DRAMSys::~DRAMSys() = default;

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
            if constexpr (StandardMapping::has_MemSpecType_v<T>)
            {
                using Impl = typename StandardMapping::Mapping<T>::MemSpecType;
                return std::make_unique<const Impl>(v);
            }
            SC_REPORT_FATAL("Configuration",
                            ("Unsupported DRAM type: " + std::string(T::id)).c_str());
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
                         std::size_t channel) -> std::unique_ptr<DRAMPowerAdapter>
    {
        std::unique_ptr<DRAMPowerAdapter> adapter = std::visit(
            [this, &drampowerSimConfig, channel](
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
                        &databaseRecorder);
                }
                SC_REPORT_FATAL("DRAMSys", "Standard does not support the power analysis");
                return nullptr;
            },
            memSpecVar.getVariant());
        return adapter;
    };

    // Create DRAMPowerAdapters
    DRAMPowerAdapter* lastDRAMPower = nullptr;
    for (std::size_t i = 0; i < memSpec->numberOfChannels; ++i)
    {
        if (auto drampower = generator(i))
        {
            lastDRAMPower = drampower.get();
            DRAMPowers.emplace_back(std::move(drampower));
        }
        assert((!(0 == i) || nullptr != lastDRAMPower) &&
               "The first channel must produce a DRAMPowerAdapter object");
        DRAMPowerMappings.emplace_back(lastDRAMPower);
    }
#endif
}

std::unique_ptr<Arbiter> DRAMSys::createArbiter()
{
    if (mcConfig->arbiter == McConfig::Arbiter::Simple)
        return std::make_unique<ArbiterSimple>(
            "arbiter", *simConfig, *mcConfig, *memSpec, *addressDecoder, databaseRecorder);

    if (mcConfig->arbiter == McConfig::Arbiter::Fifo)
        return std::make_unique<ArbiterFifo>(
            "arbiter", *simConfig, *mcConfig, *memSpec, *addressDecoder, databaseRecorder);

    if (mcConfig->arbiter == McConfig::Arbiter::Reorder)
        return std::make_unique<ArbiterReorder>(
            "arbiter", *simConfig, *mcConfig, *memSpec, *addressDecoder, databaseRecorder);

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
