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
 *    Robert Gernhardt
 *    Matthias Jung
 *    Luiza Correa
 *    Lukas Steiner
 *    Derek Christ
 */

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <list>
#include <chrono>
#include <Configuration.h>
#include <memory>
#include <systemc>

#include "simulation/DRAMSys.h"
#include "TraceSetup.h"
#include "TrafficInitiator.h"
#include "LengthConverter.h"

#ifdef RECORDING
#include "simulation/DRAMSysRecordable.h"
#endif

using namespace sc_core;

std::string pathOfFile(const std::string &file)
{
    return file.substr(0, file.find_last_of('/'));
}

int main(int argc, char **argv)
{
    return sc_main(argc, argv);
}

int sc_main(int argc, char **argv)
{
    sc_set_time_resolution(1, SC_PS);

    std::string resources;
    std::string simulationJson;
    // Run only with default config (ddr3-example.json):
    if (argc == 1)
    {
        // Get path of resources:
        resources = pathOfFile(argv[0])
                    + std::string("/../../DRAMSys/library/resources/");
        simulationJson = resources + "simulations/ddr3-example.json";
    }
    // Run with specific config but default resource folders:
    else if (argc == 2)
    {
        // Get path of resources:
        resources = pathOfFile(argv[0])
                    + std::string("/../../DRAMSys/library/resources/");
        simulationJson = argv[1];
    }
    // Run with spefific config and specific resource folder:
    else if (argc == 3)
    {
        simulationJson = argv[1];
        resources = argv[2];
    }

    std::vector<std::unique_ptr<TrafficInitiator>> players;
    std::vector<std::unique_ptr<LengthConverter>> lengthConverters;

    DRAMSysConfiguration::Configuration configLib = DRAMSysConfiguration::from_path(simulationJson, resources);

    // Instantiate DRAMSys:
    std::unique_ptr<DRAMSys> dramSys;

#ifdef RECORDING
    if (configLib.simConfig.databaseRecording.value_or(false))
        dramSys = std::make_unique<DRAMSysRecordable>("DRAMSys", configLib);
    else
#endif
        dramSys = std::make_unique<DRAMSys>("DRAMSys", configLib);

    if (!configLib.traceSetup.has_value())
        SC_REPORT_FATAL("sc_main", "No tracesetup section provided.");

    // Instantiate STL Players:
    TraceSetup setup(dramSys->getConfig(), configLib.traceSetup.value(), resources, players);

    // Bind STL Players with DRAMSys:
    for (auto& player : players)
    {
        if (player->addLengthConverter)
        {
            std::string converterName("Converter_");
            lengthConverters.emplace_back(std::make_unique<LengthConverter>(converterName.append(player->name()).c_str(),
                    dramSys->getConfig().memSpec->maxBytesPerBurst,
                    dramSys->getConfig().storeMode != Configuration::StoreMode::NoStorage));
            player->iSocket.bind(lengthConverters.back()->tSocket);
            lengthConverters.back()->iSocket.bind(dramSys->tSocket);
        }
        else
        {
            player->iSocket.bind(dramSys->tSocket);
        }
    }

    // Store the starting of the simulation in wallclock time:
    auto start = std::chrono::high_resolution_clock::now();

    // Start SystemC Simulation:
    sc_set_stop_mode(SC_STOP_FINISH_DELTA);
    sc_start();

    if (!sc_end_of_simulation_invoked())
    {
        SC_REPORT_WARNING("sc_main", "Simulation stopped without explicit sc_stop()");
        sc_stop();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Simulation took " + std::to_string(elapsed.count()) + " seconds." << std::endl;
    return 0;
}
