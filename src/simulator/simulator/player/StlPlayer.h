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
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 *    Éder F. Zulian
 *    Felipe S. Prado
 *    Derek Christ
 */

#pragma once

#include "simulator/request/Request.h"
#include "simulator/request/RequestProducer.h"

#include <systemc>
#include <tlm>

#include <array>
#include <fstream>
#include <memory>
#include <thread>
#include <vector>

class StlPlayer : public RequestProducer
{
public:
    enum class TraceType
    {
        Absolute,
        Relative,
    };

    StlPlayer(std::string_view tracePath,
              unsigned int clkMhz,
              unsigned int defaultDataLength,
              TraceType traceType,
              bool storageEnabled);

    Request nextRequest() override;

    uint64_t totalRequests() override { return numberOfLines; }

private:
    void parseTraceFile();
    std::vector<Request>::const_iterator swapBuffers();

    static constexpr std::size_t LINE_BUFFER_SIZE = 10000;

    const TraceType traceType;
    const bool storageEnabled;
    const sc_core::sc_time playerPeriod;
    const unsigned int defaultDataLength;

    std::ifstream traceFile;
    uint64_t currentLine = 0;
    uint64_t numberOfLines = 0;

    std::array<std::shared_ptr<std::vector<Request>>, 2> lineBuffers;
    std::shared_ptr<std::vector<Request>> parseBuffer;
    std::shared_ptr<std::vector<Request>> readoutBuffer;

    std::vector<Request>::const_iterator readoutIt;

    std::thread parserThread;
};
