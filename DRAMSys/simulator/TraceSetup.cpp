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
 *    Derek Christ
 */

#include "TraceSetup.h"
#include "StlPlayer.h"
#include "TrafficGenerator.h"
#include <algorithm>
#include <iomanip>
#include <map>

using namespace sc_core;
using namespace tlm;

TraceSetup::TraceSetup(const Configuration& config,
                       const DRAMSysConfiguration::TraceSetup& traceSetup,
                       const std::string& pathToResources,
                       std::vector<std::unique_ptr<TrafficInitiator>>& players)
    : memoryManager(config.storeMode != Configuration::StoreMode::NoStorage)
{
    if (traceSetup.initiators.empty())
        SC_REPORT_FATAL("TraceSetup", "No traffic initiators specified");

    for (const auto &initiator : traceSetup.initiators)
    {
        std::visit(
            [&](auto &&initiator)
            {
                std::string name = initiator.name;
                double frequencyMHz = initiator.clkMhz;
                sc_time playerClk = sc_time(1.0 / frequencyMHz, SC_US);

                unsigned int maxPendingReadRequests = [=]() -> unsigned int
                {
                    if (const auto &maxPendingReadRequests = initiator.maxPendingReadRequests)
                        return *maxPendingReadRequests;
                    else
                        return 0;
                }();

                unsigned int maxPendingWriteRequests = [=]() -> unsigned int
                {
                    if (const auto &maxPendingWriteRequests = initiator.maxPendingWriteRequests)
                        return *maxPendingWriteRequests;
                    else
                        return 0;
                }();

                bool addLengthConverter = [=]() -> bool
                {
                    if (const auto &addLengthConverter = initiator.addLengthConverter)
                        return *addLengthConverter;
                    else
                        return false;
                }();

                using T = std::decay_t<decltype(initiator)>;
                if constexpr (std::is_same_v<T, DRAMSysConfiguration::TracePlayer>)
                {
                    size_t pos = name.rfind('.');
                    if (pos == std::string::npos)
                        throw std::runtime_error("Name of the trace file does not contain a valid extension.");

                    // Get the extension and make it lower case
                    std::string ext = name.substr(pos + 1);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    std::stringstream stlFileStream;
                    stlFileStream << pathToResources << "/traces/" << name;
                    std::string stlFile = stlFileStream.str();
                    std::string moduleName = name;

                    // replace all '.' to '_'
                    std::replace(moduleName.begin(), moduleName.end(), '.', '_');

                    StlPlayer *player;
                    if (ext == "stl")
                        player = new StlPlayer(moduleName.c_str(), config, stlFile, playerClk, maxPendingReadRequests,
                                               maxPendingWriteRequests, addLengthConverter, *this, false);
                    else if (ext == "rstl")
                        player = new StlPlayer(moduleName.c_str(), config, stlFile, playerClk, maxPendingReadRequests,
                                               maxPendingWriteRequests, addLengthConverter, *this, true);
                    else
                        throw std::runtime_error("Unsupported file extension in " + name);

                    players.push_back(std::unique_ptr<TrafficInitiator>(player));
                    totalTransactions += player->getNumberOfLines();
                }
                else if constexpr (std::is_same_v<T, DRAMSysConfiguration::TraceGenerator>)
                {
                    TrafficGenerator *trafficGenerator = new TrafficGenerator(name.c_str(), config, initiator, *this);
                    players.push_back(std::unique_ptr<TrafficInitiator>(trafficGenerator));

                    totalTransactions += trafficGenerator->getTotalTransactions();
                }
                else // if constexpr (std::is_same_v<T, DRAMSysConfiguration::TraceHammer>)
                {
                    uint64_t numRequests = initiator.numRequests;
                    uint64_t rowIncrement = initiator.rowIncrement;

                    players.push_back(
                        std::unique_ptr<TrafficInitiator>(new TrafficGeneratorHammer(name.c_str(), config, initiator, *this)));
                    totalTransactions += numRequests;
                }
            },
            initiator);
    }

    for (const auto &inititatorConf : traceSetup.initiators)
    {
        if (auto generatorConf = std::get_if<DRAMSysConfiguration::TraceGenerator>(&inititatorConf))
        {
            if (const auto &idleUntil = generatorConf->idleUntil)
            {
                const std::string name = generatorConf->name;
                auto listenerIt = std::find_if(players.begin(), players.end(),
                                               [&name](const std::unique_ptr<TrafficInitiator> &initiator)
                                               { return initiator->name() == name; });

                // Should be found
                auto listener = dynamic_cast<TrafficGenerator *>(listenerIt->get());

                auto notifierIt =
                    std::find_if(players.begin(), players.end(),
                                 [&idleUntil](const std::unique_ptr<TrafficInitiator> &initiator)
                                 {
                                     if (auto generator = dynamic_cast<const TrafficGenerator *>(initiator.get()))
                                     {
                                         if (generator->hasStateTransitionEvent(*idleUntil))
                                             return true;
                                     }

                                     return false;
                                 });

                if (notifierIt == players.end())
                    SC_REPORT_FATAL("TraceSetup", "Event to listen on not found.");

                auto notifier = dynamic_cast<TrafficGenerator *>(notifierIt->get());
                listener->waitUntil(&notifier->getStateTransitionEvent(*idleUntil));
            }
        }
    }

    remainingTransactions = totalTransactions;
    numberOfTrafficInitiators = players.size();
    defaultDataLength = config.memSpec->defaultBytesPerBurst;
}

void TraceSetup::trafficInitiatorTerminates()
{
    finishedTrafficInitiators++;

    if (finishedTrafficInitiators == numberOfTrafficInitiators)
        sc_stop();
}

void TraceSetup::transactionFinished()
{
    remainingTransactions--;

    loadBar(totalTransactions - remainingTransactions, totalTransactions);

    if (remainingTransactions == 0)
        std::cout << std::endl;
}

tlm_generic_payload& TraceSetup::allocatePayload(unsigned dataLength)
{
    return memoryManager.allocate(dataLength);
}

tlm_generic_payload& TraceSetup::allocatePayload()
{
    return allocatePayload(defaultDataLength);
}

void TraceSetup::loadBar(uint64_t x, uint64_t n, unsigned int w, unsigned int granularity)
{
    if ((n < 100) || ((x != n) && (x % (n / 100 * granularity) != 0)))
        return;

    float ratio = x / (float)n;
    unsigned int c = (ratio * w);
    float rest = (ratio * w) - c;
    std::cout << std::setw(3) << round(ratio * 100) << "% |";
    for (unsigned int x = 0; x < c; x++)
        std::cout << "█";

    if (rest >= 0 && rest < 0.125f && c != w)
        std::cout << " ";
    if (rest >= 0.125f && rest < 2 * 0.125f)
        std::cout << "▏";
    if (rest >= 2 * 0.125f && rest < 3 * 0.125f)
        std::cout << "▎";
    if (rest >= 3 * 0.125f && rest < 4 * 0.125f)
        std::cout << "▍";
    if (rest >= 4 * 0.125f && rest < 5 * 0.125f)
        std::cout << "▌";
    if (rest >= 5 * 0.125f && rest < 6 * 0.125f)
        std::cout << "▋";
    if (rest >= 6 * 0.125f && rest < 7 * 0.125f)
        std::cout << "▊";
    if (rest >= 7 * 0.125f && rest < 8 * 0.125f)
        std::cout << "▉";

    for (unsigned int x = c; x < (w - 1); x++)
        std::cout << " ";
    std::cout << "|\r" << std::flush;
}
