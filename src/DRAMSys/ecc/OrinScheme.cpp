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

#include "OrinScheme.h"
#include "InlineEcc.h"
#include "DRAMSys/simulation/AddressDecoder.h"

namespace DRAMSys
{

uint64_t OrinScheme::getMappedAddress(uint64_t address, AddressDecoder const& addressDecoder)
{
    auto decodedAddress = addressDecoder.decodeAddress(address);

    const unsigned int eccColumnsToSkip = decodedAddress.row * ECC_COLUMNS_PER_ROW;
    const unsigned int newColumn = (eccColumnsToSkip + decodedAddress.column) % START_ECC_COLUMN;

    const unsigned int rowsToSkip =
        std::floor((eccColumnsToSkip + decodedAddress.column) / START_ECC_COLUMN);
    const unsigned int newRow = rowsToSkip + decodedAddress.row;

    decodedAddress.row = newRow;
    decodedAddress.column = newColumn;

    return addressDecoder.encodeAddress(decodedAddress);
}

uint64_t OrinScheme::getRedundancyAddress(uint64_t address,
                                          AddressDecoder const& addressDecoder)
{
    DecodedAddress decodedAddress = addressDecoder.decodeAddress(address);

    unsigned int atom = decodedAddress.column;
    unsigned int eccAtom = std::floor(atom * ECC_ATOMS_PER_ATOM);
    unsigned int eccColumn = START_ECC_COLUMN + eccAtom;

    decodedAddress.column = eccColumn;
    return addressDecoder.encodeAddress(decodedAddress);
}

bool OrinScheme::registerRequest(tlm::tlm_generic_payload& payload)
{
    uint64_t mappedAddress = getMappedAddress(payload.get_address(), *addressDecoder);
    uint64_t redundancyAddress = getRedundancyAddress(mappedAddress, *addressDecoder);

    auto redundancyIt = std::find_if(redundancyQueue.begin(),
                                     redundancyQueue.end(),
                                     [redundancyAddress](auto&& redundancyEntry)
                                     { return redundancyEntry.address == redundancyAddress; });

    // Check if there is already an entry for this redundancy address
    if (redundancyIt != redundancyQueue.end())
    {
        // Entry already there, check if target can be appended
        if (redundancyIt->targetList.size() >= TARGET_LIST_ENTRIES)
            return false;

        // If redundancy is already flagged for writeback, block new request
        if (redundancyIt->writeBackIssued)
            return false;

        payload.set_address(mappedAddress);
        redundancyIt->targetList.push_back(Target{&payload, false, false});
        return true;
    }

    // New cache entry needed...

    if (redundancyQueue.size() >= REDUNDANCY_QUEUE_ENTRIES)
    {
        // Can oldest cache entry be evicted?
        if (!redundancyQueue.front().targetList.empty())
            return false;

        // Needs to be written back before being evicted
        if (redundancyQueue.front().dirty)
            return false;

        redundancyQueue.pop_front();
    }

    auto* arbiterExtension =
        dynamic_cast<ArbiterExtension*>(payload.get_extension<ArbiterExtension>()->clone());

    auto redundancy =
        Redundancy{redundancyAddress, false, false, false, false, arbiterExtension, {}};

    payload.set_address(mappedAddress);
    redundancy.targetList.push_back(Target{&payload, false, false});
    redundancyQueue.push_back(std::move(redundancy));

    return true;
}

tlm::tlm_generic_payload* OrinScheme::getNextRequest()
{
    for (auto& redundancy : redundancyQueue)
    {
        // At the moment writeback happens as soon as there are no targets and not when the entry
        // needs to be evicted
        if (redundancy.dirty && !redundancy.writeBackIssued && redundancy.targetList.empty())
        {
            auto* request = allocateEccPayload(redundancy.address, true);
            request->set_auto_extension<ArbiterExtension>(
                dynamic_cast<ArbiterExtension*>(redundancy.arbiterExtension->clone()));

            redundancy.writeBackIssued = true;

            return request;
        }

        if (!redundancy.fetchIssued)
        {
            auto* request = allocateEccPayload(redundancy.address, false);
            request->set_auto_extension<ArbiterExtension>(
                dynamic_cast<ArbiterExtension*>(redundancy.arbiterExtension->clone()));

            redundancy.fetchIssued = true;

            return request;
        }

        for (auto& target : redundancy.targetList)
        {
            if (target.issued)
                continue;

            if (target.payload->is_write())
                redundancy.dirty = true;

            target.issued = true;
            return target.payload;
        }
    }

    return nullptr;
}

bool OrinScheme::registerResponse(tlm::tlm_generic_payload* payload)
{
    for (auto redundancyIt = redundancyQueue.begin(); redundancyIt != redundancyQueue.end();
         ++redundancyIt)
    {
        // Check if response is from redundancy fetch...
        if (payload->get_address() == redundancyIt->address)
        {
            // Could possibly be both the read fetch or the write back

            // Write back
            if (redundancyIt->writeBackIssued)
            {
                assert(redundancyIt->targetList.empty());
                redundancyQueue.erase(redundancyIt);
                return false;
            }

            // Read fetch
            assert(!redundancyIt->targetList.empty());
            redundancyIt->valid = true;
            return false;
        }

        // ...or if it is a target fetch
        for (auto targetIt = redundancyIt->targetList.begin();
             targetIt != redundancyIt->targetList.end();
             ++targetIt)
        {
            if (targetIt->payload != payload)
                continue;

            assert(targetIt->issued);

            if (!redundancyIt->valid)
            {
                targetIt->valid = true;
                return false;
            }

            redundancyIt->targetList.erase(targetIt);

            if (redundancyIt->targetList.empty() && !redundancyIt->dirty)
            {
                redundancyQueue.erase(redundancyIt);
            }

            return true;
        }
    }

    // Should never be here
    assert(false);

    return false;
}

tlm::tlm_generic_payload* OrinScheme::getNextResponse()
{
    for (auto redundancyIt = redundancyQueue.begin(); redundancyIt != redundancyQueue.end();
         ++redundancyIt)
    {
        if (!redundancyIt->valid)
            continue;

        for (auto targetIt = redundancyIt->targetList.begin();
             targetIt != redundancyIt->targetList.end();
             ++targetIt)
        {
            if (!targetIt->valid)
                continue;

            auto* targetResponse = targetIt->payload;

            redundancyIt->targetList.erase(targetIt);

            if (redundancyIt->targetList.empty())
            {
                redundancyQueue.erase(redundancyIt);
            }

            return targetResponse;
        }
    }

    return nullptr;
}

tlm::tlm_generic_payload* OrinScheme::allocateEccPayload(uint64_t address, bool write)
{
    tlm::tlm_generic_payload* payload = memoryManager.allocate();
    payload->acquire();
    payload->set_address(address);
    payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload->set_dmi_allowed(false);
    payload->set_byte_enable_length(0);
    payload->set_data_length(atomSize);
    payload->set_streaming_width(atomSize);
    payload->set_command(write ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND);
    payload->set_auto_extension<DRAMSys::EccExtension>(new DRAMSys::EccExtension);
    return payload;
}

} // namespace DRAMSys
