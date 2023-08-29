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
 *    Robert Gernhardt
 *    Matthias Jung
 */

#include "MemoryManager.h"

#include "DRAMSys/common/DebugManager.h"

using namespace tlm;

MemoryManager::MemoryManager(bool storageEnabled) : storageEnabled(storageEnabled)
{
}

MemoryManager::~MemoryManager()
{
    for (auto& innerBuffer : freePayloads)
    {
        while (!innerBuffer.second.empty())
        {
            tlm_generic_payload* payload = innerBuffer.second.top();
            if (storageEnabled)
                delete[] payload->get_data_ptr();
            payload->reset();
            delete payload;
            innerBuffer.second.pop();
            numberOfFrees++;
        }
    }

    // Comment in if you are suspecting a memory leak in the manager
    // PRINTDEBUGMESSAGE("MemoryManager","Number of allocated payloads: " +
    // to_string(numberOfAllocations)); PRINTDEBUGMESSAGE("MemoryManager","Number of freed payloads:
    // " + to_string(numberOfFrees));
}

tlm_generic_payload& MemoryManager::allocate(unsigned dataLength)
{
    if (freePayloads[dataLength].empty())
    {
        numberOfAllocations++;
        auto* payload = new tlm_generic_payload(this);

        if (storageEnabled)
        {
            // Allocate a data buffer and initialize it with zeroes:
            auto* data = new unsigned char[dataLength];
            std::fill(data, data + dataLength, 0);
            payload->set_data_ptr(data);
        }

        return *payload;
    }

    tlm_generic_payload* result = freePayloads[dataLength].top();
    freePayloads[dataLength].pop();
    return *result;
}

void MemoryManager::free(tlm_generic_payload* payload)
{
    unsigned dataLength = payload->get_data_length();
    freePayloads[dataLength].push(payload);
}
