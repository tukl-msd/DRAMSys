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
 *    Matthias Jung
 *    Lukas Steiner
 */

#ifndef MEMSPEC_H
#define MEMSPEC_H

#include <systemc.h>
#include <vector>
#include "../../common/dramExtensions.h"
#include "../../controller/Command.h"
#include "../../common/utils.h"
#include "../../common/third_party/nlohmann/single_include/nlohmann/json.hpp"

class MemSpec
{
public:
    unsigned numberOfChannels;
    unsigned numberOfRanks;
    unsigned banksPerRank;
    unsigned groupsPerRank;
    unsigned banksPerGroup;
    unsigned numberOfBanks;
    unsigned numberOfBankGroups;
    unsigned numberOfDevicesOnDIMM;
    unsigned numberOfRows;
    unsigned numberOfColumns;
    unsigned burstLength;
    unsigned dataRate;
    unsigned bitWidth;

    // Clock
    double fCKMHz;
    sc_time tCK;

    std::string memoryId;
    std::string memoryType;

    virtual ~MemSpec() {}

    virtual sc_time getRefreshIntervalAB() const = 0;
    virtual sc_time getRefreshIntervalPB() const = 0;

    virtual sc_time getExecutionTime(Command, const tlm::tlm_generic_payload &) const = 0;
    virtual TimeInterval getIntervalOnDataStrobe(Command) const = 0;

    sc_time getCommandLength(Command) const;

protected:
    MemSpec(nlohmann::json &memspec, unsigned numberOfChannels,
            unsigned numberOfRanks, unsigned banksPerRank,
            unsigned groupsPerRank, unsigned banksPerGroup,
            unsigned numberOfBanks, unsigned numberOfBankGroups,
            unsigned numberOfDevicesOnDIMM);

    // Command lengths in cycles on bus, usually one clock cycle
    std::vector<unsigned> commandLengthInCycles;
    sc_time burstDuration;
};

#endif // MEMSPEC_H

