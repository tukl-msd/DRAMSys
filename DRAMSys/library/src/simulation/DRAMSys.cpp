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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Eder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include "DRAMSys.h"
#include "../common/third_party/nlohmann/single_include/nlohmann/json.hpp"
#include "../common/DebugManager.h"
#include "../common/utils.h"
#include "../simulation/TemperatureController.h"
#include "../error/ecchamming.h"
#include "dram/DramDDR3.h"
#include "dram/DramDDR4.h"
#include "dram/DramWideIO.h"
#include "dram/DramLPDDR4.h"
#include "dram/DramWideIO2.h"
#include "dram/DramHBM2.h"
#include "dram/DramGDDR5.h"
#include "dram/DramGDDR5X.h"
#include "dram/DramGDDR6.h"
#include "../controller/Controller.h"

DRAMSys::DRAMSys(sc_module_name name,
                 std::string simulationToRun,
                 std::string pathToResources)
    : DRAMSys(name, simulationToRun, pathToResources, true)
{}

DRAMSys::DRAMSys(sc_module_name name,
                 std::string simulationToRun,
                 std::string pathToResources,
                 bool initAndBind)
    : sc_module(name), tSocket("DRAMSys_tSocket")
{
    // Initialize ecc pointer
    ecc = nullptr;

    logo();

    // Read Configuration Setup:
    nlohmann::json simulationdoc = parseJSON(simulationToRun);

    if (simulationdoc["simulation"].empty())
        SC_REPORT_FATAL("SimulationManager",
                        "Cannot load simulation: simulation node expected");

    Configuration::getInstance().setPathToResources(pathToResources);

    // Load config and initialize modules
    Configuration::getInstance().loadMCConfig(Configuration::getInstance(),
            pathToResources
            + "configs/mcconfigs/"
            + std::string(simulationdoc["simulation"]["mcconfig"]));

    Configuration::getInstance().loadMemSpec(Configuration::getInstance(),
            pathToResources
            + "configs/memspecs/"
            + std::string(simulationdoc["simulation"]["memspec"]));

    Configuration::getInstance().loadSimConfig(Configuration::getInstance(),
            pathToResources
            + "configs/simulator/"
            + std::string(simulationdoc["simulation"]["simconfig"]));

    Configuration::getInstance().loadTemperatureSimConfig(Configuration::getInstance(),
            pathToResources
            + "configs/thermalsim/"
            + std::string(simulationdoc["simulation"]["thermalconfig"]));

    // Setup the debug manager:
    setupDebugManager(Configuration::getInstance().simulationName);

    if (initAndBind)
    {
        // Instantiate all internal DRAMSys modules:
        std::string amconfig = simulationdoc["simulation"]["addressmapping"];
        instantiateModules(pathToResources, amconfig);
        // Connect all internal DRAMSys modules:
        bindSockets();
        report(headline);
    }
}

DRAMSys::~DRAMSys()
{
    if (ecc)
        delete ecc;

    delete arbiter;

    for (auto dram : drams)
        delete dram;

    for (auto controller : controllers)
        delete controller;

    for (auto tlmChecker : playersTlmCheckers)
        delete tlmChecker;

    for (auto tlmChecker : controllersTlmCheckers)
        delete tlmChecker;
}

void DRAMSys::logo()
{
#define GREENTXT(s)  std::string(("\u001b[38;5;28m"+std::string((s))+"\033[0m"))
#define DGREENTXT(s) std::string(("\u001b[38;5;22m"+std::string((s))+"\033[0m"))
#define LGREENTXT(s) std::string(("\u001b[38;5;82m"+std::string((s))+"\033[0m"))
#define BLACKTXT(s)  std::string(("\u001b[38;5;232m"+std::string((s))+"\033[0m"))
#define BOLDTXT(s)   std::string(("\033[1;37m"+std::string((s))+"\033[0m"))
    cout << std::endl
         << BLACKTXT("■ ■ ")<< DGREENTXT("■  ")
         << BOLDTXT("DRAMSys4.0, Copyright (c) 2020")
         << std::endl
         << BLACKTXT("■ ") << DGREENTXT("■ ") << GREENTXT("■  ")
         << "Technische Universitaet Kaiserslautern,"
         << std::endl
         << DGREENTXT("■ ") << GREENTXT("■ ") << LGREENTXT("■  " )
         << "Fraunhofer IESE"
         << std::endl
         << std::endl;
#undef GREENTXT
#undef DGREENTXT
#undef LGREENTXT
#undef BLACKTXT
#undef BOLDTXT
}

void DRAMSys::setupDebugManager(const std::string &traceName __attribute__((unused)))
{
#ifndef NDEBUG
    auto &dbg = DebugManager::getInstance();
    dbg.writeToConsole = false;
    dbg.writeToFile = true;
    if (dbg.writeToFile)
        dbg.openDebugFile(traceName + ".txt");
#endif
}

void DRAMSys::instantiateModules(const std::string &pathToResources,
                                 const std::string &amconfig)
{
    // The first call to getInstance() creates the Temperature Controller.
    // The same instance will be accessed by all other modules.
    TemperatureController::getInstance();

    // Create new ECC Controller
    if (Configuration::getInstance().ECCMode == "Hamming")
        ecc = new ECCHamming("ECCHamming");
    else if (Configuration::getInstance().ECCMode == "Disabled")
        ecc = nullptr;
    else
        SC_REPORT_FATAL("DRAMSys", "Unsupported ECC mode");

    // Save ECC Controller into the configuration struct to adjust it dynamically
    Configuration::getInstance().pECC = ecc;

    // Create arbiter
    arbiter = new Arbiter("arbiter", pathToResources + "configs/amconfigs/" + amconfig);

    // Create controllers and DRAMs
    std::string memoryType = Configuration::getInstance().memSpec->memoryType;
    for (size_t i = 0; i < Configuration::getInstance().memSpec->numberOfChannels; i++)
    {
        std::string str = "controller" + std::to_string(i);

        ControllerIF *controller = new Controller(str.c_str());
        controllers.push_back(controller);

        str = "dram" + std::to_string(i);
        Dram *dram;

        if (memoryType == "DDR3")
            dram = new DramDDR3(str.c_str());
        else if (memoryType == "WIDEIO_SDR")
            dram = new DramWideIO(str.c_str());
        else if (memoryType == "DDR4")
            dram = new DramDDR4(str.c_str());
        else if (memoryType == "LPDDR4")
            dram = new DramLPDDR4(str.c_str());
        else if (memoryType == "WIDEIO2")
            dram = new DramWideIO2(str.c_str());
        else if (memoryType == "HBM2")
            dram = new DramHBM2(str.c_str());
        else if (memoryType == "GDDR5")
            dram = new DramGDDR5(str.c_str());
        else if (memoryType == "GDDR5X")
            dram = new DramGDDR5X(str.c_str());
        else if (memoryType == "GDDR6")
            dram = new DramGDDR6(str.c_str());
        else
            SC_REPORT_FATAL("DRAMSys", "Unsupported DRAM type");

        drams.push_back(dram);

        if (Configuration::getInstance().checkTLM2Protocol)
        {
            str = "TLMCheckerController" + std::to_string(i);
            tlm_utils::tlm2_base_protocol_checker<> *controllerTlmChecker =
                new tlm_utils::tlm2_base_protocol_checker<>(str.c_str());
            controllersTlmCheckers.push_back(controllerTlmChecker);
        }
    }
}

void DRAMSys::bindSockets()
{
    // If ECC Controller enabled, put it between Trace and arbiter
    if (Configuration::getInstance().ECCMode == "Hamming")
    {
        assert(ecc != nullptr);
        tSocket.bind(ecc->t_socket);
        ecc->i_socket.bind(arbiter->tSocket);
    }
    else if (Configuration::getInstance().ECCMode == "Disabled")
        tSocket.bind(arbiter->tSocket);
    else
        SC_REPORT_FATAL("DRAMSys", "Unsupported ECC mode");

    if (Configuration::getInstance().checkTLM2Protocol)
    {
        for (size_t i = 0; i < Configuration::getInstance().memSpec->numberOfChannels; i++)
        {
            arbiter->iSocket.bind(controllersTlmCheckers[i]->target_socket);
            controllersTlmCheckers[i]->initiator_socket.bind(controllers[i]->tSocket);
            controllers[i]->iSocket.bind(drams[i]->tSocket);
        }
    }
    else
    {
        for (size_t i = 0; i < Configuration::getInstance().memSpec->numberOfChannels; i++)
        {
            arbiter->iSocket.bind(controllers[i]->tSocket);
            controllers[i]->iSocket.bind(drams[i]->tSocket);
        }
    }
}

void DRAMSys::report(std::string message)
{
    PRINTDEBUGMESSAGE(name(), message);
    std::cout << message << std::endl;
}
