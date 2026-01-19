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

#include "simulator/request/RequestProducer.h"

#include <DRAMSys/configuration/json/TraceSetup.h>

#include <systemc>
#include <tlm>

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <thread>
#include <vector>

class StlPlayer : public RequestProducer
{
public:
    enum class TraceType : uint8_t
    {
        Absolute,
        Relative,
    };

    StlPlayer(DRAMSys::Config::TracePlayer const& config,
              std::filesystem::path const& trace,
              TraceType traceType,
              bool storageEnabled);

    //TODO temporary fix
    ~StlPlayer() {
        if (parserThread.joinable())
            parserThread.join();
    }

    Request nextRequest() override;
    sc_core::sc_time nextTrigger() override;
    uint64_t totalRequests() override { return numberOfLines; }

private:
    struct LineContent
    {
        unsigned cycle{};
        enum class Command : uint8_t
        {
            Read,
            Write
        } command{};
        uint64_t address{};
        std::optional<unsigned> dataLength;
        std::vector<uint8_t> data;
    };

    std::optional<LineContent> currentLine() const;

    void parseTraceFile();
    void swapBuffers();
    void incrementLine();

    TraceType traceType;
    bool storageEnabled;
    sc_core::sc_time playerPeriod;
    unsigned int defaultDataLength;

    std::ifstream traceFile;
    uint64_t currentParsedLine = 0;
    uint64_t numberOfLines = 0;

    std::array<std::vector<LineContent>, 2> lineBuffers;
    std::size_t parseIndex = 0;
    std::size_t consumeIndex = 1;

    std::vector<LineContent>::const_iterator readoutIt;

    std::thread parserThread;
};
