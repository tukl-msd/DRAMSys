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
 *    Marco Mörz
 */

#ifndef MEMSPEC_H
#define MEMSPEC_H

#include "DRAMSys/common/utils.h"
#include "DRAMSys/controller/Command.h"

#include <DRAMPower/command/CmdType.h>
#include <DRAMPower/dram/dram_base.h>

#include <memory>
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

    static constexpr enum sc_core::sc_time_unit TCK_UNIT = sc_core::SC_SEC;

    const uint64_t numberOfChannels;
    const uint64_t ranksPerChannel;
    const uint64_t banksPerRank;
    const uint64_t groupsPerRank;
    const uint64_t banksPerGroup;
    const uint64_t banksPerChannel;
    const uint64_t bankGroupsPerChannel;
    const uint64_t devicesPerRank;
    const uint64_t rowsPerBank;
    const uint64_t columnsPerRow;
    const uint64_t defaultBurstLength;
    const uint64_t maxBurstLength;
    const uint64_t dataRate;
    const uint64_t bitWidth;
    const uint64_t dataBusWidth;
    const uint64_t defaultBytesPerBurst;
    const uint64_t maxBytesPerBurst;

    // Clock
    const sc_core::sc_time tCK;

    const std::string memoryId;
    const std::string memoryType;

    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalAB() const;
    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalPB() const;
    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalP2B() const;
    [[nodiscard]] virtual sc_core::sc_time getRefreshIntervalSB() const;

    [[nodiscard]] virtual unsigned getPer2BankOffset() const;

    [[nodiscard]] virtual unsigned getRAAIMT() const;
    [[nodiscard]] virtual unsigned getRAAMMT() const;
    [[nodiscard]] virtual unsigned getRAADEC() const;

    [[nodiscard]] virtual bool hasRasAndCasBus() const;
    [[nodiscard]] virtual bool pseudoChannelMode() const;

    [[nodiscard]] virtual sc_core::sc_time
    getExecutionTime(Command command, const tlm::tlm_generic_payload& payload) const = 0;
    [[nodiscard]] virtual TimeInterval
    getIntervalOnDataStrobe(Command command, const tlm::tlm_generic_payload& payload) const = 0;
    [[nodiscard]] virtual bool requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const;

    [[nodiscard]] sc_core::sc_time getCommandLength(Command command) const;
    [[nodiscard]] double getCommandLengthInCycles(Command command) const;
    [[nodiscard]] uint64_t getSimMemSizeInBytes() const;

    /**
     * @brief Creates the DRAMPower object if the standard is supported by DRAMPower.
     * If the standard is not supported, a fatal error is reported and the simulation is aborted.
     * @return unique_ptr to the DRAMPower object.
     */
    [[nodiscard]] virtual std::unique_ptr<DRAMPower::dram_base<DRAMPower::CmdType>>
    toDramPowerObject() const
    {
        SC_REPORT_FATAL("MemSpec", "DRAMPower does not support this memory standard");
        sc_core::sc_abort();
        // This line is never reached, but it is needed to avoid a compiler warning
        return nullptr;
    }

protected:
    [[nodiscard]] static bool allBytesEnabled(const tlm::tlm_generic_payload& trans)
    {
        if (trans.get_byte_enable_ptr() == nullptr)
            return true;

        for (std::size_t i = 0; i < trans.get_byte_enable_length(); i++)
        {
            if (trans.get_byte_enable_ptr()[i] != TLM_BYTE_ENABLED)
            {
                return false;
            }
        }

        return true;
    }

    template <typename MemSpecType>
    MemSpec(const MemSpecType& memSpec,
            uint64_t numberOfChannels,
            uint64_t ranksPerChannel,
            uint64_t banksPerRank,
            uint64_t groupsPerRank,
            uint64_t banksPerGroup,
            uint64_t banksPerChannel,
            uint64_t bankGroupsPerChannel,
            uint64_t devicesPerRank) :
        numberOfChannels(numberOfChannels),
        ranksPerChannel(ranksPerChannel),
        banksPerRank(banksPerRank),
        groupsPerRank(groupsPerRank),
        banksPerGroup(banksPerGroup),
        banksPerChannel(banksPerChannel),
        bankGroupsPerChannel(bankGroupsPerChannel),
        devicesPerRank(devicesPerRank),
        rowsPerBank(memSpec.memarchitecturespec.nbrOfRows),
        columnsPerRow(memSpec.memarchitecturespec.nbrOfColumns),
        defaultBurstLength(memSpec.memarchitecturespec.burstLength),
        maxBurstLength(memSpec.memarchitecturespec.maxBurstLength.has_value()
                           ? memSpec.memarchitecturespec.maxBurstLength.value()
                           : defaultBurstLength),
        dataRate(memSpec.memarchitecturespec.dataRate),
        bitWidth(memSpec.memarchitecturespec.width),
        dataBusWidth(bitWidth * devicesPerRank),
        defaultBytesPerBurst((defaultBurstLength * dataBusWidth) / 8),
        maxBytesPerBurst((maxBurstLength * dataBusWidth) / 8),
        tCK(sc_core::sc_time(memSpec.memtimingspec.tCK, TCK_UNIT)),
        memoryId(memSpec.memoryId),
        memoryType(memSpec.id),
        burstDuration(tCK * (static_cast<double>(defaultBurstLength) / dataRate))

    {
        commandLengthInCycles = std::vector<double>(Command::numberOfCommands(), 1);
    }

    MemSpec(const MemSpec&) = default;
    MemSpec(MemSpec&&) = default;

    // Command lengths in cycles on bus, usually one clock cycle
    std::vector<double> commandLengthInCycles;
    sc_core::sc_time burstDuration;
    uint64_t memorySizeBytes = 0;
};

} // namespace DRAMSys

#endif // MEMSPEC_H
