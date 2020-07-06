/*
 * Copyright (c) 2017, Technische Universität Kaiserslautern
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
 *    Matthias Jung
 *    Luiza Correa
 */

#include "TraceSetup.h"

TraceSetup::TraceSetup(std::string uri,
                       std::string pathToResources,
                       std::vector<TracePlayer *> *devices)
{
    // Load Simulation:
    nlohmann::json simulationdoc = parseJSON(uri);

    if (simulationdoc["simulation"].empty())
        SC_REPORT_FATAL("traceSetup",
                    "Cannot load simulation: simulation node expected");

    // Load TracePlayers:
    for (auto it : simulationdoc["simulation"]["tracesetup"].items())
    {
        auto value = it.value();
        if (!value.empty())
        {
            sc_time playerClk;
            unsigned int frequencyMHz = value["clkMhz"];

            if (frequencyMHz == 0)
                SC_REPORT_FATAL("traceSetup", "No Frequency Defined");
            else
                playerClk = sc_time(1.0 / frequencyMHz, SC_US);

            std::string name = value["name"];

            size_t pos = name.rfind('.');
            if (pos == std::string::npos)
                throw std::runtime_error("Name of the trace file does not contain a valid extension.");

            // Get the extension and make it lower case
            std::string ext = name.substr(pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            std::string stlFile =  pathToResources + std::string("traces/") + name;
            std::string moduleName = name;

            // replace all '.' to '_'
            std::replace(moduleName.begin(), moduleName.end(), '.', '_');

            TracePlayer *player;
            if (ext == "stl")
                player = new StlPlayer<false>(moduleName.c_str(), stlFile, playerClk, this);
            else if (ext == "rstl")
                player = new StlPlayer<true>(moduleName.c_str(), stlFile, playerClk, this);
            else
                throw std::runtime_error("Unsupported file extension in " + name);

            devices->push_back(player);

            if (Configuration::getInstance().simulationProgressBar)
                totalTransactions += player->getNumberOfLines(stlFile);
        }
    }

    remainingTransactions = totalTransactions;
    numberOfTracePlayers = devices->size();
}

void TraceSetup::tracePlayerTerminates()
{
    finishedTracePlayers++;

    if (finishedTracePlayers == numberOfTracePlayers)
        sc_stop();
}
void TraceSetup::transactionFinished()
{
    remainingTransactions--;

    loadbar(totalTransactions - remainingTransactions, totalTransactions);

    if (remainingTransactions == 0)
        std::cout << std::endl;
}
