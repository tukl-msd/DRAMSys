/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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

#include "DRAMSys/common/utils.h"
#include "DRAMSys/config/DRAMSysConfiguration.h"
#include "DRAMSys/controller/Command.h"

#include <string>
#include <systemc>
#include <tlm>
#include <vector>

namespace DRAMSys
{

class MemSpec
{
public:
    MemSpec& operator=(const MemSpec&) = delete;
    MemSpec& operator=(MemSpec&&) = delete;
    virtual ~MemSpec() = default;

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
    const unsigned bytesPerBeat;
    const unsigned defaultBytesPerBurst;
    const unsigned maxBytesPerBurst;

    // Clock
    const double fCKMHz;
    const sc_core::sc_time tCK;

    const std::string memoryId;
    const enum class MemoryType {
        DDR3,
        DDR4,
        DDR5,
        LPDDR4,
        LPDDR5,
        WideIO,
        WideIO2,
        GDDR5,
        GDDR5X,
        GDDR6,
        HBM2,
        HBM3,
        STTMRAM
    } memoryType;

    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalAB() const;
    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalPB() const;
    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalP2B() const;
    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalSB() const;

    [[nodiscard]] virtual unsigned getPer2BankOffset() const;

    [[nodiscard]] virtual unsigned getRAAIMT() const;
    [[nodiscard]] virtual unsigned getRAAMMT() const;
    [[nodiscard]] virtual unsigned getRAADEC() const;

    [[nodiscard]] virtual bool hasRasAndCasBus() const;

    [[nodiscard]] virtual sc_core::sc_time
    getExecutionTime(Command command, const tlm::tlm_generic_payload& payload) const = 0;
    [[nodiscard]] virtual TimeInterval
    getIntervalOnDataStrobe(Command command, const tlm::tlm_generic_payload& payload) const = 0;
    [[nodiscard]] virtual bool requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const;

    [[nodiscard]] sc_core::sc_time getCommandLength(Command command) const;
    [[nodiscard]] double getCommandLengthInCycles(Command command) const;
    [[nodiscard]] uint64_t getSimMemSizeInBytes() const;

protected:
    MemSpec(const DRAMSys::Config::MemSpec& memSpec,
            MemoryType memoryType,
            unsigned numberOfChannels,
            unsigned pseudoChannelsPerChannel,
            unsigned ranksPerChannel,
            unsigned banksPerRank,
            unsigned groupsPerRank,
            unsigned banksPerGroup,
            unsigned banksPerChannel,
            unsigned bankGroupsPerChannel,
            unsigned devicesPerRank);

    [[nodiscard]] static bool allBytesEnabled(const tlm::tlm_generic_payload& trans);

    MemSpec(const MemSpec&) = default;
    MemSpec(MemSpec&&) = default;

    // Command lengths in cycles on bus, usually one clock cycle
    std::vector<double> commandLengthInCycles;
    sc_core::sc_time burstDuration;
    uint64_t memorySizeBytes = 0;
};

} // namespace DRAMSys

#endif // MEMSPEC_H
