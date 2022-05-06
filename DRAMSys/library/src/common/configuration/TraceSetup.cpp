/*
 * Copyright (c) 2021, Technische Universität Kaiserslautern
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

#include "TraceSetup.h"

#include <variant>

namespace DRAMSysConfiguration
{

TrafficInitiator::~TrafficInitiator()
{
}

TraceGeneratorState::~TraceGeneratorState()
{
}

void to_json(json &j, const TraceSetup &c)
{
    // Create an empty array
    j = json::array();

    for (const auto &initiator : c.initiators)
    {
        json initiator_j;

        std::visit(
            [&initiator_j](auto &&initiator)
            {
                initiator_j["name"] = initiator.name;
                initiator_j["clkMhz"] = initiator.clkMhz;
                initiator_j["maxPendingReadRequests"] = initiator.maxPendingReadRequests;
                initiator_j["maxPendingWriteRequests"] = initiator.maxPendingWriteRequests;
                initiator_j["addLengthConverter"] = initiator.addLengthConverter;

                using T = std::decay_t<decltype(initiator)>;
                if constexpr (std::is_same_v<T, TraceGenerator>)
                {
                    initiator_j["type"] = "generator";
                    initiator_j["seed"] = initiator.seed;
                    initiator_j["maxTransactions"] = initiator.maxTransactions;
                    initiator_j["idleUntil"] = initiator.idleUntil;

                    // When there are less than 2 states, flatten out the json.
                    if (initiator.states.size() == 1)
                    {
                        std::visit(
                            [&initiator_j](auto &&state)
                            {
                                using U = std::decay_t<decltype(state)>;

                                if constexpr (std::is_same_v<U, TraceGeneratorTrafficState>)
                                {
                                    initiator_j["numRequests"] = state.numRequests;
                                    initiator_j["rwRatio"] = state.rwRatio;
                                    initiator_j["addressDistribution"] = state.addressDistribution;
                                    initiator_j["addressIncrement"] = state.addressIncrement;
                                    initiator_j["minAddress"] = state.minAddress;
                                    initiator_j["maxAddress"] = state.maxAddress;
                                    initiator_j["clksPerRequest"] = state.clksPerRequest;
                                    initiator_j["notify"] = state.notify;
                                }
                                else // if constexpr (std::is_same_v<U, TraceGeneratorIdleState>)
                                {
                                    initiator_j["idleClks"] = state.idleClks;
                                }
                            },
                            initiator.states.at(0));
                    }
                    else
                    {
                        json states_j = json::array();

                        for (const auto &state : initiator.states)
                        {
                            json state_j;
                            state_j["id"] = state.first;

                            std::visit(
                                [&state_j](auto &&state)
                                {
                                    using U = std::decay_t<decltype(state)>;

                                    if constexpr (std::is_same_v<U, TraceGeneratorTrafficState>)
                                    {
                                        state_j["numRequests"] = state.numRequests;
                                        state_j["rwRatio"] = state.rwRatio;
                                        state_j["addressDistribution"] = state.addressDistribution;
                                        state_j["addressIncrement"] = state.addressIncrement;
                                        state_j["minAddress"] = state.minAddress;
                                        state_j["maxAddress"] = state.maxAddress;
                                        state_j["clksPerRequest"] = state.clksPerRequest;
                                        state_j["notify"] = state.notify;
                                    }
                                    else // if constexpr (std::is_same_v<U, TraceGeneratorIdleState>)
                                    {
                                        state_j["idleClks"] = state.idleClks;
                                    }
                                },
                                state.second);

                            remove_null_values(state_j);
                            states_j.insert(states_j.end(), state_j);
                        }
                        initiator_j["states"] = states_j;

                        json transitions_j = json::array();

                        for (const auto &transition : initiator.transitions)
                        {
                            json transition_j;
                            transition_j["from"] = transition.first;
                            transition_j["to"] = transition.second.to;
                            transition_j["probability"] = transition.second.probability;
                            remove_null_values(transition_j);
                            transitions_j.insert(transitions_j.end(), transition_j);
                        }
                        initiator_j["transitions"] = transitions_j;
                    }
                }
                else if constexpr (std::is_same_v<T, TraceHammer>)
                {
                    initiator_j["type"] = "hammer";
                    initiator_j["numRequests"] = initiator.numRequests;
                    initiator_j["rowIncrement"] = initiator.rowIncrement;
                }
                else // if constexpr (std::is_same_v<T, TracePlayer>)
                {
                    initiator_j["type"] = "player";
                }
            },
            initiator);

        remove_null_values(initiator_j);
        j.insert(j.end(), initiator_j);
    }
}

void from_json(const json &j, TraceSetup &c)
{
    for (const auto &initiator_j : j)
    {
        // Default to Player, when not specified
        TrafficInitiatorType type = initiator_j.value("type", TrafficInitiatorType::Player);

        std::variant<TracePlayer, TraceGenerator, TraceHammer> initiator;

        if (type == TrafficInitiatorType::Player)
        {
            initiator = TracePlayer{};
        }
        else if (type == TrafficInitiatorType::Generator)
        {
            TraceGenerator generator;

            auto process_state = [](const json &state_j)
                -> std::pair<unsigned int, std::variant<TraceGeneratorIdleState, TraceGeneratorTrafficState>>
            {
                std::variant<TraceGeneratorIdleState, TraceGeneratorTrafficState> state;

                if (state_j.contains("idleClks"))
                {
                    // Idle state
                    TraceGeneratorIdleState idleState;
                    state_j.at("idleClks").get_to(idleState.idleClks);

                    state = std::move(idleState);
                }
                else
                {
                    // Traffic state
                    TraceGeneratorTrafficState trafficState;
                    state_j.at("numRequests").get_to(trafficState.numRequests);
                    state_j.at("rwRatio").get_to(trafficState.rwRatio);
                    state_j.at("addressDistribution").get_to(trafficState.addressDistribution);

                    if (state_j.contains("addressIncrement"))
                        state_j.at("addressIncrement").get_to(trafficState.addressIncrement);

                    if (state_j.contains("minAddress"))
                        state_j.at("minAddress").get_to(trafficState.minAddress);

                    if (state_j.contains("maxAddress"))
                        state_j.at("maxAddress").get_to(trafficState.maxAddress);

                    if (state_j.contains("clksPerRequest"))
                        state_j.at("clksPerRequest").get_to(trafficState.clksPerRequest);

                    if (state_j.contains("notify"))
                        state_j.at("notify").get_to(trafficState.notify);

                    state = std::move(trafficState);
                }

                // Default to 0
                unsigned int id = 0;

                if (state_j.contains("id"))
                    id = state_j.at("id");

                return {id, std::move(state)};
            };

            if (initiator_j.contains("states"))
            {
                for (const auto &state_j : initiator_j.at("states"))
                {
                    auto state = process_state(state_j);
                    generator.states[state.first] = std::move(state.second);
                }

                for (const auto &transition_j : initiator_j.at("transitions"))
                {
                    TraceGeneratorStateTransition transition;
                    unsigned int from = transition_j.at("from");
                    transition.to = transition_j.at("to");
                    transition.probability = transition_j.at("probability");
                    generator.transitions.emplace(from, transition);
                }
            }
            else // Only one state will be created
            {
                auto state = process_state(initiator_j);
                generator.states[state.first] = std::move(state.second);
            }

            if (initiator_j.contains("seed"))
                initiator_j.at("seed").get_to(generator.seed);

            if (initiator_j.contains("maxTransactions"))
                initiator_j.at("maxTransactions").get_to(generator.maxTransactions);

            if (initiator_j.contains("dataLength"))
                initiator_j.at("dataLength").get_to(generator.dataLength);

            if (initiator_j.contains("idleUntil"))
                initiator_j.at("idleUntil").get_to(generator.idleUntil);

            initiator = generator;
        }
        else if (type == TrafficInitiatorType::Hammer)
        {
            TraceHammer hammer;

            initiator_j.at("numRequests").get_to(hammer.numRequests);
            initiator_j.at("rowIncrement").get_to(hammer.rowIncrement);

            initiator = hammer;
        }

        std::visit(
            [&initiator_j](auto &&initiator)
            {
                initiator_j.at("name").get_to(initiator.name);
                initiator_j.at("clkMhz").get_to(initiator.clkMhz);

                if (initiator_j.contains("maxPendingReadRequests"))
                    initiator_j.at("maxPendingReadRequests").get_to(initiator.maxPendingReadRequests);

                if (initiator_j.contains("maxPendingWriteRequests"))
                    initiator_j.at("maxPendingWriteRequests").get_to(initiator.maxPendingWriteRequests);

                if (initiator_j.contains("addLengthConverter"))
                    initiator_j.at("addLengthConverter").get_to(initiator.addLengthConverter);
            },
            initiator);

        c.initiators.emplace_back(std::move(initiator));
    }
}

void from_dump(const std::string &dump, TraceSetup &c)
{
    json json_tracesetup = json::parse(dump).at("tracesetup");
    json_tracesetup.get_to(c);
}

std::string dump(const TraceSetup &c, unsigned int indentation)
{
    json json_tracesetup;
    json_tracesetup["tracesetup"] = c;
    return json_tracesetup.dump(indentation);
}

} // namespace DRAMSysConfiguration
