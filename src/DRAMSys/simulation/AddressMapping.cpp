/*
 * Copyright (c) 2026, Julius-Maximilians-Universität Würzburg
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

#include "AddressMapping.h"

namespace DRAMSys
{

static std::vector<AddressMapping::BitEntry>
populateBitVector(std::optional<std::vector<Config::AddressMapping::BitEntry>> entry)
{
    if (!entry.has_value())
        return {};

    std::vector<AddressMapping::BitEntry> result;

    for (auto const& element : entry.value())
    {
        std::vector<unsigned> temp;
        std::copy(element.cbegin(), element.cend(), std::back_inserter(temp));
        result.push_back(temp);
    }

    return result;
}

AddressMapping::AddressMapping(Config::AddressMapping const& addressMapping) :
    byteBits(populateBitVector(addressMapping.BYTE_BIT)),
    burstBits(populateBitVector(addressMapping.BURST_BIT)),
    columnBits(populateBitVector(addressMapping.COLUMN_BIT)),
    rowBits(populateBitVector(addressMapping.ROW_BIT)),
    bankBits(populateBitVector(addressMapping.BANK_BIT)),
    bankGroupBits(populateBitVector(addressMapping.BANKGROUP_BIT)),
    rankBits(populateBitVector(addressMapping.RANK_BIT)),
    stackBits(populateBitVector(addressMapping.STACK_BIT)),
    pseudochannelBits(populateBitVector(addressMapping.PSEUDOCHANNEL_BIT)),
    channelBits(populateBitVector(addressMapping.CHANNEL_BIT))
{
}

} // namespace DRAMSys
