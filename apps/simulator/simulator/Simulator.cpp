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

#include "generator/TrafficGenerator.h"
#include "hammer/RowHammer.h"
#include "player/StlPlayer.h"
#include "simulator/request/RequestIssuer.h"
#include "util.h"

#include <DRAMSys/configuration/memspec/MemSpec.h>
#include <DRAMSys/simulation/SimConfig.h>

Simulator::Simulator(DRAMSys::Config::Configuration configuration,
                     std::filesystem::path baseConfig) :
    storageEnabled(configuration.simconfig.StoreMode == DRAMSys::Config::StoreModeType::Store),
    memoryManager(storageEnabled),
    configuration(std::move(configuration)),
    dramSys(std::make_unique<DRAMSys::DRAMSys>("DRAMSys", this->configuration)),
    baseConfig(std::move(baseConfig))
{
    terminateInitiator = [this]()
    {
        terminatedInitiators++;

        if (terminatedInitiators == initiators.size())
            sc_core::sc_stop();
    };

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

    for (const auto& initiatorConfig : *this->configuration.tracesetup)
    {
        auto initiator = instantiateInitiator(initiatorConfig);
        totalTransactions += initiator->totalRequests();

        initiator->iSocket.bind(dramSys->tSocket);
        initiators.push_back(std::move(initiator));
    }
}

std::unique_ptr<RequestIssuer>
Simulator::instantiateInitiator(const DRAMSys::Config::Initiator& initiator)
{
    uint64_t memorySize = dramSys->getMemSpec().getSimMemSizeInBytes();
    sc_core::sc_time interfaceClk = dramSys->getMemSpec().tCK;

    return std::visit(
        [this, memorySize, interfaceClk](auto&& config) -> std::unique_ptr<RequestIssuer>
        {
            using T = std::decay_t<decltype(config)>;
            if constexpr (std::is_same_v<T, DRAMSys::Config::TrafficGenerator> ||
                          std::is_same_v<T, DRAMSys::Config::TrafficGeneratorStateMachine>)
            {
                auto generator = std::make_unique<TrafficGenerator>(config, memorySize);

                return std::make_unique<RequestIssuer>(config.name.c_str(),
                                                       std::move(generator),
                                                       memoryManager,
                                                       interfaceClk,
                                                       config.maxPendingReadRequests,
                                                       config.maxPendingWriteRequests,
                                                       finishTransaction,
                                                       terminateInitiator);
            }
            else if constexpr (std::is_same_v<T, DRAMSys::Config::TracePlayer>)
            {
                std::filesystem::path tracePath = baseConfig.parent_path() / config.name;

                std::optional<StlPlayer::TraceType> traceType;

                auto extension = tracePath.extension();
                if (extension == ".stl")
                    traceType = StlPlayer::TraceType::Absolute;
                else if (extension == ".rstl")
                    traceType = StlPlayer::TraceType::Relative;

                if (!traceType.has_value())
                {
                    std::string report = extension.string() + " is not a valid trace format.";
                    SC_REPORT_FATAL("Simulator", report.c_str());
                }

                auto player = std::make_unique<StlPlayer>(
                    config, tracePath.c_str(), *traceType, storageEnabled);

                return std::make_unique<RequestIssuer>(tracePath.stem().c_str(),
                                                       std::move(player),
                                                       memoryManager,
                                                       interfaceClk,
                                                       std::nullopt,
                                                       std::nullopt,
                                                       finishTransaction,
                                                       terminateInitiator);
            }
            else if constexpr (std::is_same_v<T, DRAMSys::Config::RowHammer>)
            {
                auto hammer = std::make_unique<RowHammer>(config);

                return std::make_unique<RequestIssuer>(config.name.c_str(),
                                                       std::move(hammer),
                                                       memoryManager,
                                                       interfaceClk,
                                                       1,
                                                       1,
                                                       finishTransaction,
                                                       terminateInitiator);
            }
        },
        initiator.getVariant());
}

void Simulator::run()
{
    // Store the starting of the simulation in wall-clock time:
    auto start = std::chrono::high_resolution_clock::now();

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

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Simulation took " + std::to_string(elapsed.count()) + " seconds.\n";
}
