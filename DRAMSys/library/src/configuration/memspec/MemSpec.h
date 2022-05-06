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
 *    Derek Christ
 */

#ifndef MEMSPEC_H
#define MEMSPEC_H

#include <vector>
#include <string>
#include <Configuration.h>
#include <systemc>
#include <tlm>
#include "../../common/utils.h"
#include "../../controller/Command.h"

class MemSpec
{
public:
    const unsigned numberOfChannels;
    const unsigned pseudoChannelsPerChannel;
    const unsigned ranksPerChannel;
    const unsigned banksPerRank;
    const unsigned groupsPerRank;
    const unsigned banksPerGroup;
    const unsigned banksPerChannel;
    const unsigned bankGroupsPerChannel;
    const unsigned devicesPerRank;
    const unsigned rowsPerBank;
    const unsigned columnsPerRow;
    const unsigned defaultBurstLength;
    const unsigned maxBurstLength;
    const unsigned dataRate;
    const unsigned bitWidth;
    const unsigned dataBusWidth;
    const unsigned defaultBytesPerBurst;
    const unsigned maxBytesPerBurst;

    // Clock
    const double fCKMHz;
    const sc_core::sc_time tCK;

    const std::string memoryId;
    const enum class MemoryType {DDR3, DDR4, DDR5, LPDDR4, LPDDR5, WideIO,
            WideIO2, GDDR5, GDDR5X, GDDR6, HBM2, STTMRAM} memoryType;

    virtual ~MemSpec() = default;

    virtual sc_core::sc_time getRefreshIntervalAB() const;
    virtual sc_core::sc_time getRefreshIntervalPB() const;
    virtual sc_core::sc_time getRefreshIntervalP2B() const;
    virtual sc_core::sc_time getRefreshIntervalSB() const;

    virtual unsigned getPer2BankOffset() const;

    virtual unsigned getRAAIMT() const;
    virtual unsigned getRAAMMT() const;
    virtual unsigned getRAACDR() const;

    virtual bool hasRasAndCasBus() const;

    virtual sc_core::sc_time getExecutionTime(Command command, const tlm::tlm_generic_payload &payload) const = 0;
    virtual TimeInterval getIntervalOnDataStrobe(Command command, const tlm::tlm_generic_payload &payload) const = 0;

    sc_core::sc_time getCommandLength(Command) const;
    uint64_t getSimMemSizeInBytes() const;

protected:
    MemSpec(const DRAMSysConfiguration::MemSpec &memSpec,
            MemoryType memoryType,
            unsigned numberOfChannels, unsigned pseudoChannelsPerChannel,
            unsigned ranksPerChannel, unsigned banksPerRank,
            unsigned groupsPerRank, unsigned banksPerGroup,
            unsigned banksPerChannel, unsigned bankGroupsPerChannel,
            unsigned devicesPerRank);

    // Command lengths in cycles on bus, usually one clock cycle
    std::vector<unsigned> commandLengthInCycles;
    sc_core::sc_time burstDuration;
    uint64_t memorySizeBytes;
};

#endif // MEMSPEC_H

