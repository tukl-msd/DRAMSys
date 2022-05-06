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

#ifndef TRAFFICGENERATOR_H
#define TRAFFICGENERATOR_H

#include "TrafficInitiator.h"
#include "TraceSetup.h"

#include <cstdint>
#include <map>
#include <random>

class TrafficGeneratorIf : public TrafficInitiator
{
public:
    TrafficGeneratorIf(const sc_core::sc_module_name &name, const Configuration& config, TraceSetup& setup,
                       unsigned int maxPendingReadRequests, unsigned int maxPendingWriteRequests,
                       unsigned int dataLength, bool addLengthConverter);

private:
    void sendNextPayload() override;
    virtual void prepareNextPayload(){};
    virtual uint64_t getNextAddress() = 0;
    virtual tlm::tlm_command getNextCommand() = 0;
    virtual sc_core::sc_time getGeneratorClk() const = 0;
    virtual void payloadSent(){};
    virtual uint64_t clksPerRequest() const { return 1; }
    virtual uint64_t clksToIdle() const { return 0; }
};

class TrafficGenerator : public TrafficGeneratorIf
{
public:
    TrafficGenerator(const sc_core::sc_module_name &name, const Configuration& config,
                     const DRAMSysConfiguration::TraceGenerator &conf, TraceSetup& setup);

    uint64_t getTotalTransactions() const;
    void waitUntil(const sc_core::sc_event *ev);
    bool hasStateTransitionEvent(const std::string &eventName) const;
    const sc_core::sc_event &getStateTransitionEvent(const std::string &eventName) const;

private:
    static uint64_t evaluateMinAddress(const DRAMSysConfiguration::TraceGeneratorTrafficState& state);
    static uint64_t evaluateMaxAddress(const DRAMSysConfiguration::TraceGeneratorTrafficState& state,
                                       uint64_t simMemSizeInBytes);

    void prepareNextPayload() override;
    uint64_t getNextAddress() override;
    tlm::tlm_command getNextCommand() override;
    sc_core::sc_time getGeneratorClk() const override;
    void payloadSent() override;
    uint64_t clksPerRequest() const override { return currentClksPerRequest; };
    uint64_t clksToIdle() const override { return currentClksToIdle; }

    void calculateTransitions();
    void transitionToNextState();

    sc_core::sc_time generatorClk;

    const DRAMSysConfiguration::TraceGenerator &conf;
    unsigned int currentState = 0;
    uint64_t currentAddress = 0x00;
    uint64_t currentClksPerRequest = 1;
    uint64_t transactionsSentInCurrentState = 0;

    const uint64_t maxTransactions;
    const uint64_t simMemSizeInBytes;

    uint64_t currentClksToIdle = 0;

    std::vector<unsigned int> stateSequence;
    std::vector<unsigned int>::const_iterator stateIt;

    struct EventPair
    {
        EventPair(const std::string &name, unsigned int id) : event(name.c_str()), stateId(id)
        {
        }
        sc_core::sc_event event;
        unsigned int stateId;
    };
    std::map<std::string, EventPair> stateTranstitionEvents;

    bool idleAtStart = false;
    const sc_core::sc_event *startEvent = nullptr;

    std::default_random_engine randomGenerator;
    std::uniform_real_distribution<float> randomDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
    std::uniform_int_distribution<uint64_t> randomAddressDistribution;

    static constexpr uint64_t defaultSeed = 0;
    static constexpr uint64_t defaultClksPerRequest = 1;
    static constexpr uint64_t defaultAddressIncrement = 0x00;
};

class TrafficGeneratorHammer final : public TrafficGeneratorIf
{
public:
    TrafficGeneratorHammer(const sc_core::sc_module_name &name, const Configuration& config,
                           const DRAMSysConfiguration::TraceHammer &conf, TraceSetup& setup);

private:
    void prepareNextPayload() override;
    uint64_t getNextAddress() override;
    tlm::tlm_command getNextCommand() override;
    sc_core::sc_time getGeneratorClk() const override;

    sc_core::sc_time generatorClk;
    uint64_t rowIncrement;
    uint64_t currentAddress = 0x0;
    uint64_t numRequests;
};

#endif // TRAFFICGENERATOR_H
