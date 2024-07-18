/*
 * Copyright (c) 2020, RPTU Kaiserslautern-Landau
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
 * Author: Lukas Steiner
 */

#include "BufferCounterReadWrite.h"

using namespace tlm;

namespace DRAMSys
{

BufferCounterReadWrite::BufferCounterReadWrite(unsigned requestBufferSizeRead,
                                               unsigned requestBufferSizeWrite) :
    requestBufferSizeRead(requestBufferSizeRead),
    requestBufferSizeWrite(requestBufferSizeWrite)
{
    numReadWriteRequests = std::vector<unsigned>(2);
}

bool BufferCounterReadWrite::hasBufferSpace() const
{
    return (numReadWriteRequests[0] < requestBufferSizeRead &&
            numReadWriteRequests[1] < requestBufferSizeWrite);
}

void BufferCounterReadWrite::storeRequest(const tlm_generic_payload& trans)
{
    if (trans.is_read())
        numReadWriteRequests[0]++;
    else
        numReadWriteRequests[1]++;
}

void BufferCounterReadWrite::removeRequest(const tlm_generic_payload& trans)
{
    if (trans.is_read())
        numReadWriteRequests[0]--;
    else
        numReadWriteRequests[1]--;
}

const std::vector<unsigned>& BufferCounterReadWrite::getBufferDepth() const
{
    return numReadWriteRequests;
}

unsigned BufferCounterReadWrite::getNumReadRequests() const
{
    return numReadWriteRequests[0];
}

unsigned BufferCounterReadWrite::getNumWriteRequests() const
{
    return numReadWriteRequests[1];
}

} // namespace DRAMSys
