/*
 * Copyright (c) 2018, RPTU Kaiserslautern-Landau
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
 *    Johannes Feldmann
 *    Lukas Steiner
 *    Luiza Correa
 *    Derek Christ
 */

#include "AddressDecoder.h"
#include "DRAMSys/config/AddressMapping.h"

#include <bitset>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace DRAMSys
{

static void addMapping(std::vector<Config::AddressMapping::BitEntry> const& mappingVector,
                       std::vector<unsigned>& bitVector,
                       std::vector<std::vector<unsigned>>& xorVector)
{
    for (const Config::AddressMapping::BitEntry& bitEntry : mappingVector)
    {
        if (bitEntry.get_type() == Config::AddressMapping::BitEntry::Type::SINGLE) {
            bitVector.push_back(bitEntry.at(0));
        } else {
            bitVector.push_back(bitEntry.at(0));
            xorVector.push_back(bitEntry);
        }
    }
}

AddressDecoder::AddressDecoder(const Config::AddressMapping& addressMapping)
{
    if (const auto& channelBits = addressMapping.CHANNEL_BIT)
    {
        addMapping(*channelBits, vChannelBits, vXor);
    }

    if (const auto& rankBits = addressMapping.RANK_BIT)
    {
        addMapping(*rankBits, vRankBits, vXor);
    }

    if (const auto& stackBits = addressMapping.STACK_BIT)
    {
        addMapping(*stackBits, vStackBits, vXor);
    }

    // HBM pseudo channels are internally modelled as ranks
    if (const auto& pseudoChannelBits = addressMapping.PSEUDOCHANNEL_BIT)
    {
        addMapping(*pseudoChannelBits, vRankBits, vXor);
    }

    if (const auto& bankGroupBits = addressMapping.BANKGROUP_BIT)
    {
        addMapping(*bankGroupBits, vBankGroupBits, vXor);
    }

    if (const auto& byteBits = addressMapping.BYTE_BIT)
    {
        addMapping(*byteBits, vByteBits, vXor);
    }

    if (const auto& bankBits = addressMapping.BANK_BIT)
    {
        addMapping(*bankBits, vBankBits, vXor);
    }

    if (const auto& rowBits = addressMapping.ROW_BIT)
    {
        addMapping(*rowBits, vRowBits, vXor);
    }

    if (const auto& columnBits = addressMapping.COLUMN_BIT)
    {
        addMapping(*columnBits, vColumnBits, vXor);
    }

    unsigned channels = std::lround(std::pow(2.0, vChannelBits.size()));
    unsigned ranks = std::lround(std::pow(2.0, vRankBits.size()));
    unsigned stacks = std::lround(std::pow(2.0, vStackBits.size()));
    unsigned bankGroups = std::lround(std::pow(2.0, vBankGroupBits.size()));
    unsigned banks = std::lround(std::pow(2.0, vBankBits.size()));
    unsigned rows = std::lround(std::pow(2.0, vRowBits.size()));
    unsigned columns = std::lround(std::pow(2.0, vColumnBits.size()));
    unsigned bytes = std::lround(std::pow(2.0, vByteBits.size()));

    maximumAddress = static_cast<uint64_t>(bytes) * columns * rows * banks * bankGroups * stacks *
                         ranks * channels -
                     1;

    bankgroupsPerRank = bankGroups;
    banksPerGroup = banks;
}

void AddressDecoder::plausibilityCheck(const MemSpec& memSpec)
{
    unsigned channels = std::lround(std::pow(2.0, vChannelBits.size()));
    unsigned ranks = std::lround(std::pow(2.0, vRankBits.size()));
    unsigned stacks = std::lround(std::pow(2.0, vStackBits.size()));
    unsigned bankGroups = std::lround(std::pow(2.0, vBankGroupBits.size()));
    unsigned banks = std::lround(std::pow(2.0, vBankBits.size()));
    unsigned rows = std::lround(std::pow(2.0, vRowBits.size()));
    unsigned columns = std::lround(std::pow(2.0, vColumnBits.size()));
    unsigned bytes = std::lround(std::pow(2.0, vByteBits.size()));

    maximumAddress = static_cast<uint64_t>(bytes) * columns * rows * banks * bankGroups * stacks *
                         ranks * channels -
                     1;

    auto totalAddressBits = static_cast<unsigned>(std::log2(maximumAddress));
    for (unsigned bitPosition = 0; bitPosition < totalAddressBits; bitPosition++)
    {
        if (std::count(vChannelBits.begin(), vChannelBits.end(), bitPosition) +
                std::count(vRankBits.begin(), vRankBits.end(), bitPosition) +
                std::count(vStackBits.begin(), vStackBits.end(), bitPosition) +
                std::count(vBankGroupBits.begin(), vBankGroupBits.end(), bitPosition) +
                std::count(vBankBits.begin(), vBankBits.end(), bitPosition) +
                std::count(vRowBits.begin(), vRowBits.end(), bitPosition) +
                std::count(vColumnBits.begin(), vColumnBits.end(), bitPosition) +
                std::count(vByteBits.begin(), vByteBits.end(), bitPosition) !=
            1)
            SC_REPORT_FATAL("AddressDecoder", "Not all address bits occur exactly once");
    }

    int highestByteBit = -1;

    if (!vByteBits.empty())
    {
        highestByteBit = static_cast<int>(*std::max_element(vByteBits.begin(), vByteBits.end()));

        for (unsigned bitPosition = 0; bitPosition <= static_cast<unsigned>(highestByteBit);
             bitPosition++)
        {
            if (std::find(vByteBits.begin(), vByteBits.end(), bitPosition) == vByteBits.end())
                SC_REPORT_FATAL("AddressDecoder", "Byte bits are not continuous starting from 0");
        }
    }

    auto maxBurstLengthBits = static_cast<unsigned>(std::log2(memSpec.maxBurstLength));

    for (unsigned bitPosition = highestByteBit + 1;
         bitPosition < highestByteBit + 1 + maxBurstLengthBits;
         bitPosition++)
    {
        if (std::find(vColumnBits.begin(), vColumnBits.end(), bitPosition) == vColumnBits.end())
            SC_REPORT_FATAL("AddressDecoder", "No continuous column bits for maximum burst length");
    }

    unsigned absoluteBankGroups = bankgroupsPerRank * ranks;
    unsigned absoluteBanks = banksPerGroup * absoluteBankGroups;

    if (memSpec.numberOfChannels != channels || memSpec.ranksPerChannel != ranks ||
        memSpec.bankGroupsPerChannel != absoluteBankGroups ||
        memSpec.banksPerChannel != absoluteBanks || memSpec.rowsPerBank != rows ||
        memSpec.columnsPerRow != columns)
        SC_REPORT_FATAL("AddressDecoder", "Memspec and address mapping do not match");
}

DecodedAddress AddressDecoder::decodeAddress(uint64_t encAddr) const
{
    if (encAddr > maximumAddress)
        SC_REPORT_WARNING("AddressDecoder",
                          ("Address " + std::to_string(encAddr) +
                           " out of range (maximum address is " + std::to_string(maximumAddress) +
                           ")")
                              .c_str());

    // Apply XOR
    // For each used xor:
    //   Get the first bit and second bit. Apply a bitwise xor operator and save it back to the
    //   first bit.
    auto tempAddr = encAddr;
    for (const auto& it : vXor)
    {
        uint64_t xoredBit = std::accumulate(it.cbegin(),
                                            it.cend(),
                                            0,
                                            [tempAddr](uint64_t acc, unsigned xorBit)
                                            { return acc ^= (tempAddr >> xorBit) & UINT64_C(1); });
        encAddr &= ~(UINT64_C(1) << it[0]);
        encAddr |= xoredBit << it[0];
    }

    DecodedAddress decAddr;

    for (unsigned it = 0; it < vChannelBits.size(); it++)
        decAddr.channel |= ((encAddr >> vChannelBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vRankBits.size(); it++)
        decAddr.rank |= ((encAddr >> vRankBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vStackBits.size(); it++)
        decAddr.stack |= ((encAddr >> vStackBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vBankGroupBits.size(); it++)
        decAddr.bankgroup |= ((encAddr >> vBankGroupBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vBankBits.size(); it++)
        decAddr.bank |= ((encAddr >> vBankBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vRowBits.size(); it++)
        decAddr.row |= ((encAddr >> vRowBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vColumnBits.size(); it++)
        decAddr.column |= ((encAddr >> vColumnBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vByteBits.size(); it++)
        decAddr.byte |= ((encAddr >> vByteBits[it]) & UINT64_C(1)) << it;

    decAddr.bankgroup = decAddr.bankgroup + decAddr.rank * bankgroupsPerRank;
    decAddr.bank = decAddr.bank + decAddr.bankgroup * banksPerGroup;

    return decAddr;
}

unsigned AddressDecoder::decodeChannel(uint64_t encAddr) const
{
    if (encAddr > maximumAddress)
        SC_REPORT_WARNING("AddressDecoder",
                          ("Address " + std::to_string(encAddr) +
                           " out of range (maximum address is " + std::to_string(maximumAddress) +
                           ")")
                              .c_str());

    // Apply XOR
    // For each used xor:
    //   Get the first bit and second bit. Apply a bitwise xor operator and save it back to the
    //   first bit.
    auto tempAddr = encAddr;
    for (const auto& it : vXor)
    {
        uint64_t xoredBit = std::accumulate(it.cbegin(),
                                            it.cend(),
                                            0,
                                            [tempAddr](uint64_t acc, unsigned xorBit)
                                            { return acc ^= (tempAddr >> xorBit) & UINT64_C(1); });
        encAddr &= ~(UINT64_C(1) << it[0]);
        encAddr |= xoredBit << it[0];
    }

    unsigned channel = 0;

    for (unsigned it = 0; it < vChannelBits.size(); it++)
        channel |= ((encAddr >> vChannelBits[it]) & UINT64_C(1)) << it;

    return channel;
}

uint64_t AddressDecoder::encodeAddress(DecodedAddress decodedAddress) const
{
    // Convert absoulte addressing for bank, bankgroup to relative
    decodedAddress.bankgroup = decodedAddress.bankgroup % bankgroupsPerRank;
    decodedAddress.bank = decodedAddress.bank % banksPerGroup;

    uint64_t address = 0;

    for (unsigned i = 0; i < vChannelBits.size(); i++)
        address |= ((decodedAddress.channel >> i) & 0x1) << vChannelBits[i];

    for (unsigned i = 0; i < vRankBits.size(); i++)
        address |= ((decodedAddress.rank >> i) & 0x1) << vRankBits[i];

    for (unsigned i = 0; i < vStackBits.size(); i++)
        address |= ((decodedAddress.stack >> i) & 0x1) << vStackBits[i];

    for (unsigned i = 0; i < vBankGroupBits.size(); i++)
        address |= ((decodedAddress.bankgroup >> i) & 0x1) << vBankGroupBits[i];

    for (unsigned i = 0; i < vBankBits.size(); i++)
        address |= ((decodedAddress.bank >> i) & 0x1) << vBankBits[i];

    for (unsigned i = 0; i < vRowBits.size(); i++)
        address |= ((decodedAddress.row >> i) & 0x1) << vRowBits[i];

    for (unsigned i = 0; i < vColumnBits.size(); i++)
        address |= ((decodedAddress.column >> i) & 0x1) << vColumnBits[i];

    for (unsigned i = 0; i < vByteBits.size(); i++)
        address |= ((decodedAddress.byte >> i) & 0x1) << vByteBits[i];

    // TODO: XOR encoding

    return address;
}

void AddressDecoder::print() const
{
    std::cout << headline << std::endl;
    std::cout << "Used Address Mapping:" << std::endl;
    std::cout << std::endl;

    for (int it = static_cast<int>(vChannelBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vChannelBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) == vChannelBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " Ch " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vRankBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vRankBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) == vRankBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " Ra " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vStackBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vStackBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) == vStackBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " SID " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vBankGroupBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vBankGroupBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) ==
                vBankGroupBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " Bg " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vBankBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vBankBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) == vBankBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " Ba " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vRowBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vRowBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) == vRowBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " Ro " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vColumnBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vColumnBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) == vColumnBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " Co " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vByteBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vByteBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto xorMapping : vXor)
        {
            if (xorMapping.at(0) == vByteBits[static_cast<std::vector<unsigned>::size_type>(it)])
            {
                for (auto it = xorMapping.cbegin() + 1; it != xorMapping.cend(); it++)
                    addressBits |= (UINT64_C(1) << *it);
            }
        }
        std::cout << " By " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    std::cout << std::endl;
}

} // namespace DRAMSys
