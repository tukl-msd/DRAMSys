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

#include "TrafficGenerator.h"

TrafficGenerator::TrafficGenerator(DRAMSys::Config::TrafficGeneratorStateMachine const& config,
                                   MemoryManager& memoryManager,
                                   uint64_t memorySize,
                                   unsigned int defaultDataLength,
                                   std::function<void()> transactionFinished,
                                   std::function<void()> terminateInitiator) :
    stateTransistions(config.transitions),
    generatorPeriod(sc_core::sc_time(1.0 / static_cast<double>(config.clkMhz), sc_core::SC_US)),
    issuer(
        config.name.c_str(),
        memoryManager,
        config.clkMhz,
        config.maxPendingReadRequests,
        config.maxPendingWriteRequests,
        [this] { return nextRequest(); },
        std::move(transactionFinished),
        std::move(terminateInitiator))
{
    unsigned int dataLength = config.dataLength.value_or(defaultDataLength);
    unsigned int dataAlignment = config.dataAlignment.value_or(dataLength);

    for (auto const& state : config.states)
    {
        std::visit(
            [=, &config](auto&& arg)
            {
                using DRAMSys::Config::TrafficGeneratorActiveState;
                using DRAMSys::Config::TrafficGeneratorIdleState;
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, TrafficGeneratorActiveState>)
                {
                    auto const& activeState = arg;
                    if (activeState.addressDistribution ==
                        DRAMSys::Config::AddressDistribution::Random)
                    {
                        auto producer = std::make_unique<RandomProducer>(activeState.numRequests,
                                                                         config.seed,
                                                                         activeState.rwRatio,
                                                                         activeState.minAddress,
                                                                         activeState.maxAddress,
                                                                         memorySize,
                                                                         dataLength,
                                                                         dataAlignment);

                        producers.emplace(activeState.id, std::move(producer));
                    }
                    else
                    {
                        auto producer =
                            std::make_unique<SequentialProducer>(activeState.numRequests,
                                                                 config.seed,
                                                                 activeState.rwRatio,
                                                                 activeState.addressIncrement,
                                                                 activeState.minAddress,
                                                                 activeState.maxAddress,
                                                                 memorySize,
                                                                 dataLength);

                        producers.emplace(activeState.id, std::move(producer));
                    }
                }
                else if constexpr (std::is_same_v<T, TrafficGeneratorIdleState>)
                {
                    auto const& idleState = arg;
                    idleStateClks.emplace(idleState.id, idleState.idleClks);
                }
            },
            state);
    }
}

TrafficGenerator::TrafficGenerator(DRAMSys::Config::TrafficGenerator const& config,
                                   MemoryManager& memoryManager,
                                   uint64_t memorySize,
                                   unsigned int defaultDataLength,
                                   std::function<void()> transactionFinished,
                                   std::function<void()> terminateInitiator) :
    generatorPeriod(sc_core::sc_time(1.0 / static_cast<double>(config.clkMhz), sc_core::SC_US)),
    issuer(
        config.name.c_str(),
        memoryManager,
        config.clkMhz,
        config.maxPendingReadRequests,
        config.maxPendingWriteRequests,
        [this] { return nextRequest(); },
        std::move(transactionFinished),
        std::move(terminateInitiator))
{
    unsigned int dataLength = config.dataLength.value_or(defaultDataLength);
    unsigned int dataAlignment = config.dataAlignment.value_or(dataLength);

    if (config.addressDistribution == DRAMSys::Config::AddressDistribution::Random)
    {
        auto producer = std::make_unique<RandomProducer>(config.numRequests,
                                                         config.seed,
                                                         config.rwRatio,
                                                         config.minAddress,
                                                         config.maxAddress,
                                                         memorySize,
                                                         dataLength,
                                                         dataAlignment);
        producers.emplace(0, std::move(producer));
    }
    else
    {
        auto producer = std::make_unique<SequentialProducer>(config.numRequests,
                                                             config.seed,
                                                             config.rwRatio,
                                                             config.addressIncrement,
                                                             config.minAddress,
                                                             config.maxAddress,
                                                             memorySize,
                                                             dataLength);
        producers.emplace(0, std::move(producer));
    }
}

Request TrafficGenerator::nextRequest()
{
    uint64_t clksToIdle = 0;
    if (requestsInState >= producers[currentState]->totalRequests())
    {
        // Reset current producer to its initial state
        producers[currentState]->reset();

        auto newState = stateTransition(currentState);

        if (!newState.has_value())
            return Request{Request::Command::Stop};

        auto idleStateIt = idleStateClks.find(newState.value());
        while (idleStateIt != idleStateClks.cend())
        {
            clksToIdle += idleStateIt->second;
            newState = stateTransition(currentState);

            if (!newState.has_value())
                return Request{Request::Command::Stop};

            currentState = newState.value();
            idleStateIt = idleStateClks.find(newState.value());
        }

        currentState = newState.value();
        requestsInState = 0;
    }

    requestsInState++;

    Request request = producers[currentState]->nextRequest();
    request.delay += generatorPeriod * static_cast<double>(clksToIdle);
    return request;
}

uint64_t TrafficGenerator::totalRequests()
{
    // Store current state of random generator
    std::default_random_engine tempGenerator(randomGenerator);

    // Reset generator to initial state
    randomGenerator.seed();

    uint64_t totalRequests = 0;
    unsigned int currentState = 0;

    if (producers.find(currentState) != producers.cend())
        totalRequests += producers.at(currentState)->totalRequests();

    while (auto nextState = stateTransition(currentState))
    {
        currentState = nextState.value();

        if (producers.find(currentState) != producers.cend())
            totalRequests += producers.at(currentState)->totalRequests();
    }

    // Restore state of random generator
    randomGenerator = tempGenerator;

    return totalRequests;
}

std::optional<unsigned int> TrafficGenerator::stateTransition(unsigned int from)
{
    using Transition = DRAMSys::Config::TrafficGeneratorStateTransition;

    std::vector<Transition> relevantTransitions;
    std::copy_if(stateTransistions.cbegin(),
                 stateTransistions.cend(),
                 std::back_inserter(relevantTransitions),
                 [from](Transition transition) { return transition.from == from; });

    if (relevantTransitions.empty())
        return std::nullopt;

    std::vector<double> propabilities;
    std::for_each(relevantTransitions.cbegin(),
                  relevantTransitions.cend(),
                  [&propabilities](Transition transition)
                  { propabilities.push_back(transition.probability); });

    assert(propabilities.size() == relevantTransitions.size());

    std::discrete_distribution<std::size_t> stateTransitionDistribution(propabilities.cbegin(),
                                                                        propabilities.cend());

    std::size_t index = stateTransitionDistribution(randomGenerator);
    return relevantTransitions[index].to;
}
