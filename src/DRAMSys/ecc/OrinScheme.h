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

#ifndef ORINSCHEME_H
#define ORINSCHEME_H

#include "EccScheme.h"

#include <DRAMSys/common/dramExtensions.h>
#include <DRAMSys/common/MemoryManager.h>
#include <DRAMSys/simulation/Arbiter.h>

namespace DRAMSys
{

class OrinScheme : public EccScheme
{
public:
    OrinScheme(std::shared_ptr<const AddressDecoder> addressDecoder,
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

    [[nodiscard]] static uint64_t getMappedAddress(uint64_t address,
                                                   AddressDecoder const& addressDecoder);
    [[nodiscard]] static uint64_t getRedundancyAddress(uint64_t address,
                                                       AddressDecoder const& addressDecoder);

    [[nodiscard]] tlm::tlm_generic_payload* allocateEccPayload(uint64_t address, bool write);

    static constexpr unsigned COLUMNS_PER_ROW = 64;
    
    // ECC
    static constexpr unsigned ECC_COLUMNS_PER_ROW = 8;
    static constexpr unsigned START_ECC_COLUMN = COLUMNS_PER_ROW - ECC_COLUMNS_PER_ROW;
    static constexpr double ECC_ATOMS_PER_ATOM = 1.0 / 16;

    static constexpr std::size_t REDUNDANCY_QUEUE_ENTRIES = 16;
    static constexpr std::size_t TARGET_LIST_ENTRIES = 4;

    std::deque<Redundancy> redundancyQueue;

    MemoryManager memoryManager;
    std::shared_ptr<const AddressDecoder> addressDecoder;
    unsigned int atomSize;
};

} // namespace DRAMSys

#endif // ORINSCHEME_H
