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
 *    Éder F. Zulian
 *    Felipe S. Prado
 *    Derek Christ
 */

#ifndef STLPLAYER_H
#define STLPLAYER_H

#include <sstream>
#include <vector>
#include <array>
#include <thread>
#include <fstream>

#include <systemc>
#include <tlm>
#include "TraceSetup.h"
#include "TrafficInitiator.h"

struct LineContent
{
    sc_core::sc_time sendingTime;
    unsigned dataLength;
    tlm::tlm_command command;
    uint64_t address;
    std::vector<unsigned char> data;
};

class StlPlayer : public TrafficInitiator
{
public:
    StlPlayer(const sc_core::sc_module_name &name,
              const Configuration& config,
              const std::string &pathToTrace,
              const sc_core::sc_time &playerClk,
              unsigned int maxPendingReadRequests,
              unsigned int maxPendingWriteRequests,
              bool addLengthConverter,
              TraceSetup& setup,
              bool relative);

    ~StlPlayer() override;
    void sendNextPayload() override;
    uint64_t getNumberOfLines() const;

private:
    void parseTraceFile();
    std::vector<LineContent>::const_iterator swapBuffers();

    std::ifstream file;
    uint64_t lineCnt = 0;
    uint64_t numberOfLines = 0;

    const sc_core::sc_time playerClk;  // May be different from the memory clock!

    static constexpr unsigned lineBufferSize = 10000;

    std::vector<LineContent>* currentBuffer;
    std::vector<LineContent>* parseBuffer;
    std::array<std::vector<LineContent>, 2> lineContents;
    std::vector<LineContent>::const_iterator lineIterator;

    std::thread parserThread;

    const bool relative;
};

#endif // STLPLAYER_H
