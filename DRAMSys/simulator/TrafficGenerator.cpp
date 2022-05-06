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
 *    Robert Gernhardt
 *    Matthias Jung
 *    Derek Christ
 */

#include "TrafficGenerator.h"
#include "TraceSetup.h"

#include <limits>

using namespace sc_core;
using namespace tlm;

TrafficGeneratorIf::TrafficGeneratorIf(const sc_core::sc_module_name& name, const Configuration& config,
                                       TraceSetup& setup,
                                       unsigned int maxPendingReadRequests, unsigned int maxPendingWriteRequests,
                                       unsigned int dataLength, bool addLengthConverter)
    : TrafficInitiator(name, config, setup, maxPendingReadRequests, maxPendingWriteRequests, dataLength,
                       addLengthConverter)
{
}

void TrafficGeneratorIf::sendNextPayload()
{
    prepareNextPayload();

    if (finished)
        return;

    // TODO: column / burst breite

    uint64_t address = getNextAddress();

    tlm_command command = getNextCommand();

    if (command == tlm::TLM_READ_COMMAND)
        pendingReadRequests++;
    else if (command == tlm::TLM_WRITE_COMMAND)
        pendingWriteRequests++;

    tlm_generic_payload& payload = setup.allocatePayload();
    payload.acquire();
    payload.set_address(address);
    payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload.set_dmi_allowed(false);
    payload.set_byte_enable_length(0);
    payload.set_data_length(defaultDataLength);
    payload.set_command(command);

    sc_time generatorClk = getGeneratorClk();
    sc_time sendingOffset;
    if (transactionsSent == 0)
        sendingOffset = SC_ZERO_TIME + generatorClk * clksToIdle();
    else
        sendingOffset = (generatorClk * clksPerRequest()) - (sc_time_stamp() % generatorClk) + generatorClk * clksToIdle();

    // TODO: do not send two requests in the same cycle
    sendToTarget(payload, tlm::BEGIN_REQ, sendingOffset);

    transactionsSent++;
    PRINTDEBUGMESSAGE(name(), "Performing request #" + std::to_string(transactionsSent));
    payloadSent();
}

TrafficGenerator::TrafficGenerator(const sc_module_name& name, const Configuration& config,
                                   const DRAMSysConfiguration::TraceGenerator& conf, TraceSetup& setup)
    : TrafficGeneratorIf(name, config, setup, conf.maxPendingReadRequests.value_or(defaultMaxPendingReadRequests),
                         conf.maxPendingWriteRequests.value_or(defaultMaxPendingWriteRequests),
                         conf.dataLength.value_or(config.memSpec->defaultBytesPerBurst),
                         conf.addLengthConverter.value_or(false)),
      generatorClk(TrafficInitiator::evaluateGeneratorClk(conf)), conf(conf),
      maxTransactions(conf.maxTransactions.value_or(std::numeric_limits<uint64_t>::max())),
      simMemSizeInBytes(config.memSpec->getSimMemSizeInBytes()),
      randomGenerator(std::default_random_engine(conf.seed.value_or(defaultSeed)))
{
    // Perform checks for all states
    for (const auto &state : conf.states)
    {
        if (auto trafficState = std::get_if<DRAMSysConfiguration::TraceGeneratorTrafficState>(&state.second))
        {
            uint64_t minAddress = evaluateMinAddress(*trafficState);
            uint64_t maxAddress = evaluateMaxAddress(*trafficState, simMemSizeInBytes);
            double rwRatio = (*trafficState).rwRatio;

            if (minAddress > config.memSpec->getSimMemSizeInBytes() - 1)
                SC_REPORT_FATAL("TrafficGenerator", "minAddress is out of range.");

            if (maxAddress > config.memSpec->getSimMemSizeInBytes() - 1)
                SC_REPORT_FATAL("TrafficGenerator", "minAddress is out of range.");

            if (maxAddress < minAddress)
                SC_REPORT_FATAL("TrafficGenerator", "maxAddress is smaller than minAddress.");

            if (rwRatio < 0 || rwRatio > 1)
                SC_REPORT_FATAL("TraceSetup", "Read/Write ratio is not a number between 0 and 1.");

            if (const auto &eventName = trafficState->notify)
            {
                stateTranstitionEvents.emplace(std::piecewise_construct, std::forward_as_tuple(*eventName),
                                               std::forward_as_tuple(eventName->c_str(), state.first));
            }
        }
    }

    if (auto trafficState =
            std::get_if<DRAMSysConfiguration::TraceGeneratorTrafficState>(&conf.states.at(currentState)))
    {
        uint64_t minAddress = evaluateMinAddress(*trafficState);
        uint64_t maxAddress = evaluateMaxAddress(*trafficState, simMemSizeInBytes);
        randomAddressDistribution = std::uniform_int_distribution<uint64_t>(minAddress, maxAddress);
        currentClksPerRequest = trafficState->clksPerRequest.value_or(defaultClksPerRequest);
    }

    calculateTransitions();
}

void TrafficGenerator::calculateTransitions()
{
    unsigned int state = 0;
    uint64_t totalTransactions = 0;
    stateSequence.push_back(state);

    while (true)
    {
        auto transitionsIt = conf.transitions.equal_range(state);
        float probabilityAccumulated = 0.0f;
        std::map<unsigned int, std::pair<float, float>> transitionsDistribution;

        for (auto it = transitionsIt.first; it != transitionsIt.second; ++it)
        {
            float lowerLimit = probabilityAccumulated;
            probabilityAccumulated += it->second.probability;
            float upperLimit = probabilityAccumulated;
            transitionsDistribution[it->second.to] = {lowerLimit, upperLimit};
        }

        if (probabilityAccumulated > 1.001f)
            SC_REPORT_WARNING("TrafficGenerator", "Sum of transition probabilities greater than 1.");

        float random = randomDistribution(randomGenerator);
        bool transitionFound = false;

        for (const auto &transition : transitionsDistribution)
        {
            auto to = transition.first;
            auto limits = transition.second;

            if (limits.first < random && limits.second > random)
            {
                state = to;
                stateSequence.push_back(state);
                transitionFound = true;
                break;
            }
        }

        if (transitionFound)
        {
            if (auto trafficState =
                    std::get_if<DRAMSysConfiguration::TraceGeneratorTrafficState>(&conf.states.at(state)))
                totalTransactions += trafficState->numRequests;

            if (totalTransactions < maxTransactions)
                continue;
        }

        break;
    }

    stateIt = stateSequence.cbegin();
}

bool TrafficGenerator::hasStateTransitionEvent(const std::string &eventName) const
{
    auto it = stateTranstitionEvents.find(eventName);

    if (it == stateTranstitionEvents.end())
        return false;

    return true;
}

const sc_core::sc_event &TrafficGenerator::getStateTransitionEvent(const std::string &eventName) const
{
    auto it = stateTranstitionEvents.find(eventName);

    if (it == stateTranstitionEvents.end())
        SC_REPORT_FATAL("TraceSetup", "StateTransitionEvent not found.");

    return it->second.event;
}

uint64_t TrafficGenerator::getTotalTransactions() const
{
    uint64_t totalTransactions = 0;

    for (auto state : stateSequence)
    {
        if (auto trafficState = std::get_if<DRAMSysConfiguration::TraceGeneratorTrafficState>(&conf.states.at(state)))
            totalTransactions += trafficState->numRequests;
    }

    if (totalTransactions > maxTransactions)
        totalTransactions = maxTransactions;

    return totalTransactions;
}

void TrafficGenerator::waitUntil(const sc_core::sc_event *ev)
{
    startEvent = ev;
}

void TrafficGenerator::transitionToNextState()
{
    ++stateIt;

    if (stateIt == stateSequence.cend() || transactionsSent >= maxTransactions)
    {
        // No transition performed.
        finished = true;
        return;
    }

    currentState = *stateIt;

    // Notify
    for (auto &it : stateTranstitionEvents)
    {
        if (it.second.stateId == currentState)
            it.second.event.notify();
    }

    if (auto idleState = std::get_if<DRAMSysConfiguration::TraceGeneratorIdleState>(&conf.states.at(currentState)))
    {
        currentClksToIdle += idleState->idleClks;
        transitionToNextState();
        return;
    }
    else if (auto trafficState =
                 std::get_if<DRAMSysConfiguration::TraceGeneratorTrafficState>(&conf.states.at(currentState)))
    {
        uint64_t minAddress = evaluateMinAddress(*trafficState);
        uint64_t maxAddress = evaluateMaxAddress(*trafficState, simMemSizeInBytes);
        randomAddressDistribution = std::uniform_int_distribution<uint64_t>(minAddress, maxAddress);
        currentClksPerRequest = trafficState->clksPerRequest.value_or(defaultClksPerRequest);
    }

    currentAddress = 0x00;
    transactionsSentInCurrentState = 0;
}

void TrafficGenerator::prepareNextPayload()
{
    if (transactionsSent >= maxTransactions)
    {
        finished = true;
        return;
    }

    if (startEvent && transactionsSent == 0)
        wait(*startEvent);

    if (auto trafficState =
            std::get_if<DRAMSysConfiguration::TraceGeneratorTrafficState>(&conf.states.at(currentState)))
    {
        if (transactionsSentInCurrentState >= trafficState->numRequests)
            transitionToNextState();
    }

    // In case we are in an idle state right at the beginning of the simulation,
    // set the clksToIdle and transition to the next state.
    if (auto idleState =
        std::get_if<DRAMSysConfiguration::TraceGeneratorIdleState>(&conf.states.at(currentState)))
    {
        currentClksToIdle = idleState->idleClks;
        transitionToNextState();
    }
}

void TrafficGenerator::payloadSent()
{
    // Reset clks to idle.
    currentClksToIdle = 0;

    transactionsSentInCurrentState++;
}

tlm::tlm_command TrafficGenerator::getNextCommand()
{
    // An idle state should never reach this method.
    auto &state = std::get<DRAMSysConfiguration::TraceGeneratorTrafficState>(conf.states.at(currentState));

    tlm_command command;
    if (randomDistribution(randomGenerator) < state.rwRatio)
        command = tlm::TLM_READ_COMMAND;
    else
        command = tlm::TLM_WRITE_COMMAND;

    return command;
}

sc_core::sc_time TrafficGenerator::getGeneratorClk() const
{
    return generatorClk;
}

uint64_t TrafficGenerator::getNextAddress()
{
    using DRAMSysConfiguration::AddressDistribution;

    // An idle state should never reach this method.
    auto &state = std::get<DRAMSysConfiguration::TraceGeneratorTrafficState>(conf.states.at(currentState));

    uint64_t minAddress = evaluateMinAddress(state);
    uint64_t maxAddress = evaluateMaxAddress(state, simMemSizeInBytes);

    if (state.addressDistribution == AddressDistribution::Sequential)
    {
        uint64_t addressIncrement = state.addressIncrement.value_or(defaultAddressIncrement);

        uint64_t address = currentAddress;
        currentAddress += addressIncrement;
        if (currentAddress > maxAddress)
            currentAddress = minAddress;
        return address;
    }
    else if (state.addressDistribution == AddressDistribution::Random)
    {
        return randomAddressDistribution(randomGenerator);
    }
    else
    {
        return 0x00;
    }
}

uint64_t TrafficGenerator::evaluateMinAddress(const DRAMSysConfiguration::TraceGeneratorTrafficState &state)
{
    return state.minAddress.value_or(0x00);
}

uint64_t TrafficGenerator::evaluateMaxAddress(const DRAMSysConfiguration::TraceGeneratorTrafficState &state,
                                              uint64_t simMemSizeInBytes)
{
    return state.maxAddress.value_or(simMemSizeInBytes - 1);
}

TrafficGeneratorHammer::TrafficGeneratorHammer(const sc_core::sc_module_name &name, const Configuration& config,
                                               const DRAMSysConfiguration::TraceHammer &conf, TraceSetup& setup)
    : TrafficGeneratorIf(name, config, setup, 1, 1, config.memSpec->defaultBytesPerBurst, false),
      generatorClk(evaluateGeneratorClk(conf)), rowIncrement(conf.rowIncrement), numRequests(conf.numRequests)
{
}

tlm::tlm_command TrafficGeneratorHammer::getNextCommand()
{
    return tlm::TLM_READ_COMMAND;
}

sc_core::sc_time TrafficGeneratorHammer::getGeneratorClk() const
{
    return generatorClk;
}

uint64_t TrafficGeneratorHammer::getNextAddress()
{
    if (currentAddress == 0x0)
        currentAddress = rowIncrement;
    else
        currentAddress = 0x0;

    return currentAddress;
}

void TrafficGeneratorHammer::prepareNextPayload()
{
    if (transactionsSent >= numRequests)
        finished = true;
}
