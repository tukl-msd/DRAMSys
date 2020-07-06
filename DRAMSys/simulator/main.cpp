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
 */

#include <iostream>
#include <string>
#include <systemc.h>
#include <utility>
#include <vector>
#include <chrono>

#include "simulation/DRAMSys.h"
#include "TraceSetup.h"

#ifdef RECORDING
#include "simulation/DRAMSysRecordable.h"
#include "common/third_party/nlohmann/single_include/nlohmann/json.hpp"

using json = nlohmann::json;
#endif

std::string pathOfFile(std::string file)
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
    if (argc == 1) {
        // Get path of resources:
        resources = pathOfFile(argv[0])
                    + std::string("/../../DRAMSys/library/resources/");
        simulationJson = resources + "simulations/ddr3-example.json";
    }
    // Run with specific config but default resource folders:
    else if (argc == 2) {
        // Get path of resources:
        resources = pathOfFile(argv[0])
                    + std::string("/../../DRAMSys/library/resources/");
        simulationJson = argv[1];
    }
    // Run with spefific config and specific resource folder:
    else if (argc == 3) {
        simulationJson = argv[1];
        resources = argv[2];
    }

    std::vector<TracePlayer *> players;

    // Instantiate DRAMSys:
    DRAMSys *dramSys;
#ifdef RECORDING
    json simulationdoc = parseJSON(simulationJson);
    json simulatordoc = parseJSON(resources + "configs/simulator/"
                                  + std::string(simulationdoc["simulation"]["simconfig"]));

    if (simulatordoc["simconfig"]["DatabaseRecording"])
        dramSys = new DRAMSysRecordable("DRAMSys", simulationJson, resources);
    else
#endif
        dramSys = new DRAMSys("DRAMSys", simulationJson, resources);

    // Instantiate STL Players:
    TraceSetup *ts = new TraceSetup(simulationJson, resources, &players);

    // Bind STL Players with DRAMSys:
    for (size_t i = 0; i < players.size(); i++)
    {
        if (Configuration::getInstance().checkTLM2Protocol)
        {
            std::string str = "TLMCheckerPlayer" + std::to_string(i);
            tlm_utils::tlm2_base_protocol_checker<> *playerTlmChecker =
                new tlm_utils::tlm2_base_protocol_checker<>(str.c_str());
            dramSys->playersTlmCheckers.push_back(playerTlmChecker);
            players[i]->iSocket.bind(dramSys->playersTlmCheckers[i]->target_socket);
            dramSys->playersTlmCheckers[i]->initiator_socket.bind(dramSys->tSocket);
        }
        else
        {
             players[i]->iSocket.bind(dramSys->tSocket);
        }
    }

    // Store the starting of the simulation in wallclock time:
    auto start = std::chrono::high_resolution_clock::now();

    // Kickstart the players:
    for (auto &p : players)
        p->nextPayload();

    // Start SystemC Simulation:
    sc_set_stop_mode(SC_STOP_FINISH_DELTA);
    sc_start();

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Simulation took " + std::to_string(elapsed.count()) + " seconds." << std::endl;

    delete dramSys;
    delete ts;

    return 0;
}
