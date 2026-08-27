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

#include "GeneratorProducer.h"

#include "RandomState.h"
#include "SequentialState.h"

namespace DRAMSys::Initiators
{

GeneratorProducer::GeneratorProducer(unsigned clkMhz,
                                     uint64_t numRequests,
                                     unsigned int dataLength,
                                     unsigned int dataAlignment,
                                     double rwRatio,
                                     AddressDistribution addressDistribution,
                                     uint64_t addressIncrement,
                                     uint64_t minAddress,
                                     uint64_t maxAddress,
                                     uint64_t seed) :
    generatorPeriod(sc_core::sc_time(1.0 / static_cast<double>(clkMhz), sc_core::SC_US))
{
    if (addressDistribution == AddressDistribution::Random)
    {
        auto producer = std::make_unique<RandomState>(
            numRequests, seed, rwRatio, minAddress, maxAddress, dataLength, dataAlignment);
        producers.emplace(0, std::move(producer));
    }
    else
    {
        auto producer = std::make_unique<SequentialState>(
            numRequests, seed, rwRatio, addressIncrement, minAddress, maxAddress, dataLength);
        producers.emplace(0, std::move(producer));
    }
}

GeneratorProducer::GeneratorProducer(
    StateMachineDescriptor const& desc) :
    stateTransistions(desc.transitions),
    generatorPeriod(sc_core::sc_time(1.0 / static_cast<double>(desc.clkMhz), sc_core::SC_US))
{
    unsigned int dataLength = desc.dataLength;
    unsigned int dataAlignment = desc.dataAlignment;

    for (auto const& state : desc.states)
    {
        std::visit(
            [=](auto&& arg)
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
                        auto producer = std::make_unique<RandomState>(activeState.numRequests,
                                                                      desc.seed,
                                                                      activeState.rwRatio,
                                                                      activeState.minAddress,
                                                                      activeState.maxAddress,
                                                                      dataLength,
                                                                      dataAlignment);

                        producers.emplace(activeState.id, std::move(producer));
                    }
                    else
                    {
                        auto producer =
                            std::make_unique<SequentialState>(activeState.numRequests,
                                                              desc.seed,
                                                              activeState.rwRatio,
                                                              activeState.addressIncrement,
                                                              activeState.minAddress,
                                                              activeState.maxAddress,
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

Request GeneratorProducer::nextRequest()
{
    if (currentState == STOP_STATE)
        return Request{Request::Command::Stop, 0, 0, {}};

    Request request = producers[currentState]->nextRequest();
    requestsInState++;

    // Switch state if necessary
    unsigned ticksToIdle = 0;
    if (requestsInState >= producers[currentState]->totalRequests())
    {
        // Reset current producer to its initial state
        producers[currentState]->reset();

        auto newState = stateTransition(currentState);

        auto idleStateIt = idleStateClks.find(newState);
        while (idleStateIt != idleStateClks.cend())
        {
            ticksToIdle += idleStateIt->second;
            newState = stateTransition(newState);
            idleStateIt = idleStateClks.find(newState);
        }

        currentState = newState;
        requestsInState = 0;
    }

    if (currentState == STOP_STATE)
        // Allow the issuer to finish before the response comes back
        nextTriggerTime = sc_core::SC_ZERO_TIME;
    else
        nextTriggerTime = generatorPeriod * (1 + ticksToIdle);

    return request;
}

uint64_t GeneratorProducer::totalRequests()
{
    // Store current state of random generator
    std::default_random_engine tempGenerator(randomGenerator);

    // Reset generator to initial state
    randomGenerator.seed();

    uint64_t totalRequests = 0;
    unsigned int currentState = 0;

    if (producers.find(currentState) != producers.cend())
        totalRequests += producers.at(currentState)->totalRequests();

    while (currentState != STOP_STATE)
    {
        currentState = stateTransition(currentState);

        if (producers.find(currentState) != producers.cend())
            totalRequests += producers.at(currentState)->totalRequests();
    }

    // Restore state of random generator
    randomGenerator = tempGenerator;

    return totalRequests;
}

unsigned int GeneratorProducer::stateTransition(unsigned int from)
{
    using Transition = StateTransitionDescriptor;

    std::vector<Transition> relevantTransitions;
    std::copy_if(stateTransistions.cbegin(),
                 stateTransistions.cend(),
                 std::back_inserter(relevantTransitions),
                 [from](Transition transition) { return transition.from == from; });

    if (relevantTransitions.empty())
        return STOP_STATE;

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

} // namespace DRAMSys::Initiators
