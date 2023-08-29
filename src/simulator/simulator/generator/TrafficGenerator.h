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

#include "RandomProducer.h"
#include "SequentialProducer.h"
#include "simulator/Initiator.h"
#include "simulator/MemoryManager.h"
#include "simulator/request/RequestIssuer.h"

#include <DRAMSys/config/DRAMSysConfiguration.h>

class TrafficGenerator : public Initiator
{
public:
    TrafficGenerator(DRAMSys::Config::TrafficGenerator const& config,
                     MemoryManager& memoryManager,
                     uint64_t memorySize,
                     unsigned int defaultDataLength,
                     std::function<void()> transactionFinished,
                     std::function<void()> terminateInitiator);

    TrafficGenerator(DRAMSys::Config::TrafficGeneratorStateMachine const& config,
                     MemoryManager& memoryManager,
                     uint64_t memorySize,
                     unsigned int defaultDataLength,
                     std::function<void()> transactionFinished,
                     std::function<void()> terminateInitiator);

    void bind(tlm_utils::multi_target_base<>& target) override { issuer.iSocket.bind(target); }

    uint64_t totalRequests() override;
    Request nextRequest();

    std::optional<unsigned int> stateTransition(unsigned int from);

private:
    uint64_t requestsInState = 0;
    unsigned int currentState = 0;
    const std::vector<DRAMSys::Config::TrafficGeneratorStateTransition> stateTransistions;

    using IdleClks = uint64_t;
    std::unordered_map<unsigned int, IdleClks> idleStateClks;
    const sc_core::sc_time generatorPeriod;

    std::default_random_engine randomGenerator;

    std::unordered_map<unsigned int, std::unique_ptr<RequestProducer>> producers;
    RequestIssuer issuer;
};
