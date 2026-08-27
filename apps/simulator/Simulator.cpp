/*
 * Copyright (c) 2023, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#include "Simulator.h"
#include "util.h"

#include <DRAMSys/configuration/json/TraceSetup.h>
#include <DRAMSys/configuration/memspec/MemSpec.h>
#include <DRAMSys/initiators/generator/GeneratorProducer.h>
#include <DRAMSys/initiators/generator/RowHammer.h>
#include <DRAMSys/initiators/generator/TrafficGenerator.h>
#include <DRAMSys/initiators/player/StlPlayer.h>
#include <DRAMSys/initiators/player/StlProducer.h>
#include <DRAMSys/initiators/request/Request.h>
#include <DRAMSys/initiators/request/RequestIssuer.h>
#include <DRAMSys/simulation/SimConfig.h>
#include <DRAMSys/statistics/JsonFormat.h>
#include <DRAMSys/statistics/PrettyFormat.h>

#include <fmt/format.h>
#include <fmt/os.h>

#include <chrono>
#include <memory>
#include <sstream>

using namespace DRAMSys::Initiators;

Simulator::Simulator(sc_core::sc_module_name const& name,
                     DRAMSys::Config::Configuration configuration,
                     std::filesystem::path baseConfig) :
    sc_core::sc_module(name),
    storageEnabled(configuration.simconfig.StoreMode == ::DRAMSys::Config::StoreModeType::Store),
    configuration(std::move(configuration)),
    dramSys(std::make_unique<DRAMSys::DRAMSys>("DRAMSys", this->configuration)),
    baseConfig(std::move(baseConfig)),
    stats(*this)
{
    terminateInitiator = [this]()
    {
        terminatedInitiators++;

        if (terminatedInitiators == initiators.size() && dramSys->idle())
        {
            sc_core::sc_stop();
        }
    };

    dramSys->registerIdleCallback(
        [this]()
        {
            if (terminatedInitiators == initiators.size() && dramSys->idle())
            {
                sc_core::sc_stop();
            }
        });

    finishTransaction = [this]()
    {
        transactionsFinished++;

        if (dramSys->getSimConfig().simulationProgressBar)
            loadBar(transactionsFinished, totalTransactions);
    };

    if (!this->configuration.tracesetup.has_value())
    {
        SC_REPORT_FATAL("Simulator", "No traffic initiators specified");
        std::abort(); // Silence warning
    }

    physicalStorage = std::make_unique<DRAMSys::PhysicalStorage>(dramSys->memorySize());
    dramSys->setBackingStore(physicalStorage->data());

    for (const auto& initiatorConfig : *this->configuration.tracesetup)
    {
        auto initiator = instantiateInitiator(initiatorConfig);
        initiator->registerTransactionFinishedCallback(finishTransaction);
        initiator->registerFinishedCallback(terminateInitiator);
        totalTransactions += initiator->totalRequests();

        initiator->iSocket.bind(dramSys->tSocket);
        initiators.push_back(std::move(initiator));
    }
}

std::unique_ptr<RequestIssuer>
Simulator::instantiateInitiator(const ::DRAMSys::Config::Initiator& initiator)
{
    uint64_t memorySize = dramSys->memorySize();

    return std::visit(
        [this, memorySize](auto&& config) -> std::unique_ptr<RequestIssuer>
        {
            using T = std::decay_t<decltype(config)>;
            if constexpr (std::is_same_v<T, ::DRAMSys::Config::TrafficGenerator>)
            {
                TrafficGeneratorDescriptor desc;
                desc.name = config.name;
                desc.clkMhz = config.clkMhz;
                desc.numRequests = config.numRequests;
                desc.addressDistribution =
                    config.addressDistribution == DRAMSys::Config::AddressDistribution::Sequential
                        ? GeneratorProducer::AddressDistribution::Sequential
                        : GeneratorProducer::AddressDistribution::Random;
                desc.seed = config.seed.value_or(0);
                desc.dataLength = config.dataLength;
                desc.dataAlignment = config.dataAlignment.value_or(desc.dataLength);
                desc.rwRatio = config.rwRatio;
                desc.addressIncrement = config.addressIncrement.value_or(desc.dataLength);
                desc.minAddress = config.minAddress.value_or(0);
                desc.maxAddress = config.maxAddress.value_or(memorySize - desc.dataLength);
                desc.maxPendingReadRequests = config.maxPendingReadRequests;
                desc.maxPendingWriteRequests = config.maxPendingWriteRequests;

                return std::make_unique<TrafficGenerator>(desc);
            }
            else if constexpr (std::is_same_v<T, ::DRAMSys::Config::TrafficGeneratorStateMachine>)
            {
                GeneratorProducer::StateMachineDescriptor desc;
                desc.clkMhz = config.clkMhz;
                desc.name = config.name;
                desc.maxPendingReadRequests = config.maxPendingReadRequests;
                desc.maxPendingWriteRequests = config.maxPendingWriteRequests;
                desc.seed = config.seed.value_or(0);
                desc.dataLength = config.dataLength;

                for (auto const& state : config.states)
                {
                    std::visit(
                        [&](auto&& state)
                        {
                            using R = std::decay_t<decltype(state)>;
                            if constexpr (std::is_same_v<R,
                                                         GeneratorProducer::ActiveStateDescriptor>)
                            {
                                GeneratorProducer::ActiveStateDescriptor state_desc{};
                                state_desc.id = state.id;
                                state_desc.numRequests = state.numRequests;
                                state_desc.rwRatio = state.rwRatio;
                                state_desc.addressDistribution = state.addressDistribution;
                                state_desc.addressIncrement = state.addressIncrement;
                                state_desc.minAddress = state.minAddress;
                                state_desc.maxAddress = state.maxAddress;
                                desc.states.emplace_back(state_desc);
                            }
                            else if constexpr (std::is_same_v<
                                                   R,
                                                   GeneratorProducer::IdleStateDescriptor>)
                            {
                                GeneratorProducer::IdleStateDescriptor state_desc{};
                                state_desc.id = state.id;
                                state_desc.idleClks = state.idleClks;
                                desc.states.emplace_back(state_desc);
                            }
                        },
                        state);
                }

                for (auto const& transition : desc.transitions)
                {
                    GeneratorProducer::StateTransitionDescriptor trans_desc{};
                    trans_desc.from = transition.from;
                    trans_desc.to = transition.to;
                    trans_desc.probability = transition.probability;
                    desc.transitions.emplace_back(trans_desc);
                }

                return std::make_unique<TrafficGeneratorStateMachine>(desc);
            }
            else if constexpr (std::is_same_v<T, ::DRAMSys::Config::TracePlayer>)
            {
                std::filesystem::path tracePath = baseConfig.parent_path() / config.name;

                std::optional<StlProducer::TraceType> traceType;

                auto extension = tracePath.extension();
                if (extension == ".stl")
                    traceType = StlProducer::TraceType::Absolute;
                else if (extension == ".rstl")
                    traceType = StlProducer::TraceType::Relative;

                if (!traceType.has_value())
                {
                    std::string report = extension.string() + " is not a valid trace format.";
                    SC_REPORT_FATAL("Simulator", report.c_str());
                }

                StlPlayerDescriptor desc;
                desc.name = config.name;
                desc.clkMhz = config.clkMhz;
                desc.dataLength = config.dataLength;
                desc.tracePath = tracePath;
                desc.traceType = traceType.value();
                desc.storageEnabled = storageEnabled;
                desc.maxPendingReadRequests = config.maxPendingReadRequests;
                desc.maxPendingWriteRequests = config.maxPendingWriteRequests;

                return std::make_unique<StlPlayer>(desc);
            }
            else if constexpr (std::is_same_v<T, ::DRAMSys::Config::RowHammer>)
            {
                RowHammerDescriptor desc;
                desc.name = config.name;
                desc.clkMhz = config.clkMhz;
                desc.numRequests = config.numRequests;
                desc.rowIncrement = config.rowIncrement;
                desc.dataLength = config.dataLength;
                desc.maxPendingReadRequests = config.maxPendingReadRequests;
                desc.maxPendingWriteRequests = config.maxPendingWriteRequests;

                return std::make_unique<RowHammer>(desc);
            }
        },
        initiator.getVariant());
}

void Simulator::run()
{
    // Store the starting of the simulation in wall-clock time:
    startTime = std::chrono::high_resolution_clock::now();

    // Start the SystemC simulation
    if (configuration.simconfig.SimulationTime.has_value())
    {
        sc_core::sc_start(
            sc_core::sc_time(configuration.simconfig.SimulationTime.value(), sc_core::SC_SEC));
    }
    else
    {
        sc_core::sc_start();
    }

    if (!sc_core::sc_end_of_simulation_invoked())
    {
        SC_REPORT_WARNING("Simulator", "Simulation stopped without explicit sc_stop()");
        sc_core::sc_stop();
    }

    std::stringstream stats;
    DRAMSys::Stats::PrettyFormat::collectStats(this, stats);
    fmt::print("{}", stats.str());
}

Simulator::Stats::Stats(Simulator const& simulator) :
    Group(simulator.name()),
    wallclockTime(addStat<DRAMSys::Stats::ScalarStat>("WallclockTime",
                                                      "Wall-clock time elapsed by the simulation",
                                                      DRAMSys::Stats::Quantity::Time)),
    simulationTicks(addStat<DRAMSys::Stats::ScalarStat>(
        "SimulationTicks",
        fmt::format("Total simulation ticks ({})", sc_core::sc_get_time_resolution().to_string()),
        DRAMSys::Stats::Quantity::Count)),
    simulationTime(addStat<DRAMSys::Stats::ScalarStat>(
        "SimulationTime", "Total simulation duration", DRAMSys::Stats::Quantity::Time))
{
}

void Simulator::updateStats()
{
    double wallclockTime =
        std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime)
            .count();
    stats.wallclockTime = wallclockTime;

    sc_core::sc_time diffSimTime = sc_core::sc_time_stamp() - lastSimTime;
    stats.simulationTime = diffSimTime.to_seconds();
    stats.simulationTicks = static_cast<double>(diffSimTime.value());
}

void Simulator::resetStats()
{
    startTime = std::chrono::high_resolution_clock::now();
    lastSimTime = sc_core::sc_time_stamp();
}
