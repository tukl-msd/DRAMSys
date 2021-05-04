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
 */

#ifndef STLPLAYER_H
#define STLPLAYER_H

#include <sstream>
#include <vector>
#include <array>
#include <thread>
#include "TraceSetup.h"
#include "TracePlayer.h"

struct LineContent
{
    sc_time sendingTime;
    tlm::tlm_command cmd;
    uint64_t addr;
    std::vector<unsigned char> data;
};

class StlPlayer : public TracePlayer
{
public:
    StlPlayer(sc_module_name name,
              std::string pathToTrace,
              sc_time playerClk,
              TraceSetup *setup,
              bool relative);

    virtual ~StlPlayer() override;

    virtual void nextPayload() override;

private:
    void parseTraceFile();
    std::vector<LineContent>::const_iterator swapBuffers();

    std::ifstream file;
    uint64_t lineCnt;

    unsigned int burstlength;
    unsigned int dataLength;
    sc_time playerClk;  // May be different from the memory clock!

    static constexpr unsigned lineBufferSize = 10000;

    std::vector<LineContent> *currentBuffer;
    std::vector<LineContent> *parseBuffer;
    std::array<std::vector<LineContent>, 2> lineContents;
    std::vector<LineContent>::const_iterator lineIterator;

    std::thread parserThread;

    std::string time;
    std::string command;
    std::string address;
    std::string dataStr;

    std::string line;
    std::istringstream iss;

    const bool relative;
};

#endif // STLPLAYER_H
