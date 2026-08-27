/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#pragma once

#include "RowHammerProducer.h"

#include <DRAMSys/common/MemoryManager.h>
#include <DRAMSys/initiators/request/RequestIssuer.h>

namespace DRAMSys::Initiators
{

struct RowHammerDescriptor
{
    std::string name;
    uint64_t clkMhz;
    uint64_t numRequests;
    unsigned rowIncrement;
    unsigned dataLength;
    std::optional<unsigned int> maxPendingReadRequests;
    std::optional<unsigned int> maxPendingWriteRequests;
};

class RowHammer : public Initiators::RequestIssuer
{
public:
    RowHammer(RowHammerDescriptor const& desc) :
        RequestIssuer(desc.name.c_str(),
                      std::make_unique<RowHammerProducer>(
                          desc.clkMhz, desc.numRequests, desc.rowIncrement, desc.dataLength),
                      false,
                      desc.maxPendingReadRequests,
                      desc.maxPendingWriteRequests)
    {
    }
};

} // namespace DRAMSys::Initiators
