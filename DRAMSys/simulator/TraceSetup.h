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
 *    Derek Christ
 */

#ifndef TRACESETUP_H
#define TRACESETUP_H

#include <Configuration.h>
#include <vector>
#include <string>
#include <memory>
#include <tlm>

#include "MemoryManager.h"
#include "configuration/Configuration.h"

class TrafficInitiator;

class TraceSetup
{
public:
    TraceSetup(const Configuration& config,
               const DRAMSysConfiguration::TraceSetup &traceSetup,
               const std::string &pathToResources,
               std::vector<std::unique_ptr<TrafficInitiator>> &devices);

    void trafficInitiatorTerminates();
    void transactionFinished();
    tlm::tlm_generic_payload& allocatePayload(unsigned dataLength);
    tlm::tlm_generic_payload& allocatePayload();

private:
    unsigned int numberOfTrafficInitiators;
    uint64_t totalTransactions = 0;
    uint64_t remainingTransactions;
    unsigned int finishedTrafficInitiators = 0;
    MemoryManager memoryManager;
    unsigned defaultDataLength = 64;

    static void loadBar(uint64_t x, uint64_t n, unsigned int w = 50, unsigned int granularity = 1);
};

#endif // TRACESETUP_H
