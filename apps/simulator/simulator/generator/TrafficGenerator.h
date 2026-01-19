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

#pragma once

#include "GeneratorState.h"
#include "simulator/request/RequestProducer.h"

#include <DRAMSys/configuration/json/DRAMSysConfiguration.h>

#include <random>

class RequestProducer;

class TrafficGenerator : public RequestProducer
{
public:
    TrafficGenerator(DRAMSys::Config::TrafficGenerator const& config, uint64_t memorySize);

    TrafficGenerator(DRAMSys::Config::TrafficGeneratorStateMachine const& config,
                     uint64_t memorySize);

    uint64_t totalRequests() override;
    Request nextRequest() override;
    sc_core::sc_time nextTrigger() override { return nextTriggerTime; }

    unsigned int stateTransition(unsigned int from);

private:
    static constexpr unsigned int STOP_STATE = UINT_MAX;

    uint64_t requestsInState = 0;
    unsigned int currentState = 0;
    sc_core::sc_time nextTriggerTime = sc_core::SC_ZERO_TIME;
    std::vector<DRAMSys::Config::TrafficGeneratorStateTransition> stateTransistions;

    std::unordered_map<unsigned int, unsigned int> idleStateClks;
    sc_core::sc_time generatorPeriod;

    std::default_random_engine randomGenerator;

    std::unordered_map<unsigned int, std::unique_ptr<GeneratorState>> producers;
};
