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

#include "SequentialProducer.h"
#include "definitions.h"

SequentialProducer::SequentialProducer(uint64_t numRequests,
                                       std::optional<uint64_t> seed,
                                       double rwRatio,
                                       std::optional<uint64_t> addressIncrement,
                                       std::optional<uint64_t> minAddress,
                                       std::optional<uint64_t> maxAddress,
                                       uint64_t memorySize,
                                       unsigned int dataLength) :
    numberOfRequests(numRequests),
    addressIncrement(addressIncrement.value_or(dataLength)),
    minAddress(minAddress.value_or(DEFAULT_MIN_ADDRESS)),
    maxAddress(maxAddress.value_or(memorySize - 1)),
    seed(seed.value_or(DEFAULT_SEED)),
    rwRatio(rwRatio),
    dataLength(dataLength),
    randomGenerator(this->seed)
{
    if (minAddress > memorySize - 1)
        SC_REPORT_FATAL("TrafficGenerator", "minAddress is out of range.");

    if (maxAddress > memorySize - 1)
        SC_REPORT_FATAL("TrafficGenerator", "minAddress is out of range.");

    if (maxAddress < minAddress)
        SC_REPORT_FATAL("TrafficGenerator", "maxAddress is smaller than minAddress.");

    if (rwRatio < 0 || rwRatio > 1)
        SC_REPORT_FATAL("TraceSetup", "Read/Write ratio is not a number between 0 and 1.");
}

Request SequentialProducer::nextRequest()
{
    Request request;
    request.address = generatedRequests * addressIncrement % (maxAddress - minAddress) + minAddress;
    request.command = readWriteDistribution(randomGenerator) < rwRatio ? Request::Command::Read
                                                                       : Request::Command::Write;
    request.length = dataLength;
    request.delay = sc_core::SC_ZERO_TIME;

    generatedRequests++;
    return request;
}
