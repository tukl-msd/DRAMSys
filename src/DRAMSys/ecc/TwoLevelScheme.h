/*
 * Copyright (c) 2024, RPTU Kaiserslautern-Landau
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
 * Author: Derek Christ
 */

#ifndef TWOLEVELSCHEME_H
#define TWOLEVELSCHEME_H

#include "EccScheme.h"

#include <DRAMSys/common/dramExtensions.h>
#include <DRAMSys/common/MemoryManager.h>
#include <DRAMSys/simulation/Arbiter.h>

#include <random>

namespace DRAMSys
{

class TwoLevelScheme : public EccScheme
{
public:
    TwoLevelScheme(std::shared_ptr<const AddressDecoder> addressDecoder,
                   unsigned int atomSize) :
        memoryManager(false),
        addressDecoder(std::move(addressDecoder)),
        atomSize(atomSize)
    {
    }

    [[nodiscard]] bool registerRequest(tlm::tlm_generic_payload& payload) override;
    [[nodiscard]] bool registerResponse(tlm::tlm_generic_payload* payload) override;

    [[nodiscard]] tlm::tlm_generic_payload* getNextRequest() override;
    [[nodiscard]] tlm::tlm_generic_payload* getNextResponse() override;

private:
    struct Target
    {
        tlm::tlm_generic_payload* payload;
        bool issued;
        bool valid;
        bool erroneous;
    };

    struct Redundancy
    {
        uint64_t address;
        bool fetchIssued;
        bool writeBackIssued;
        bool valid;
        bool dirty;
        ArbiterExtension* arbiterExtension;
        std::vector<Target> targetList;
    };

    struct SecondLevelRedundancy
    {
        uint64_t address;
        bool fetchIssued;
        bool writeBackIssued;
        bool valid;
        bool dirty;
        std::vector<uint64_t> targetAddressList;
    };

    [[nodiscard]] uint64_t getMappedAddress(uint64_t address) const;
    [[nodiscard]] uint64_t getFirstLevelRedundancyAddress(uint64_t address) const;
    [[nodiscard]] uint64_t getSecondLevelRedundancyAddress(uint64_t address) const;

    [[nodiscard]] tlm::tlm_generic_payload* allocateEccPayload(uint64_t address, bool write);

    static constexpr unsigned COLUMNS_PER_ROW = 64;

    // CRC (First level)
    static constexpr unsigned CRC_COLUMNS_PER_ROW = 8;
    static constexpr unsigned START_CRC_COLUMN = COLUMNS_PER_ROW - CRC_COLUMNS_PER_ROW;
    static constexpr double CRC_ATOMS_PER_DATA_ATOM = 1.0 / 16;

    // BCH (Second level)
    static constexpr double ECC_ATOMS_PER_DATA_ATOM = 1.0;
    static constexpr unsigned int ECC_ROWS_PER_DATA_ROW = 1;

    static constexpr std::size_t FIRST_LEVEL_REDUNDANCY_QUEUE_ENTRIES = 16;
    static constexpr std::size_t TARGET_LIST_ENTRIES = 4;

    std::deque<Redundancy> firstLevelRedundancyQueue;
    std::optional<SecondLevelRedundancy> secondLevelRedundancy;

    std::default_random_engine randomEngine;

    static constexpr double ERROR_RATE = 0.05;
    std::bernoulli_distribution errorDistribution{ERROR_RATE};

    MemoryManager memoryManager;
    std::shared_ptr<const AddressDecoder> addressDecoder;
    unsigned int atomSize;
};

} // namespace DRAMSys

#endif // TWOLEVELSCHEME_H
