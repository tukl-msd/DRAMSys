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
#include "DRAMSys/configuration/Configuration.h"

#include <bitset>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace DRAMSys
{

AddressDecoder::AddressDecoder(const DRAMSys::Config::AddressMapping& addressMapping,
                               const MemSpec& memSpec)
{
    if (const auto& channelBits = addressMapping.CHANNEL_BIT)
    {
        std::copy(channelBits->begin(), channelBits->end(), std::back_inserter(vChannelBits));
    }

    if (const auto& rankBits = addressMapping.RANK_BIT)
    {
        std::copy(rankBits->begin(), rankBits->end(), std::back_inserter(vRankBits));
    }

    // HBM pseudo channels are internally modelled as ranks
    if (const auto& pseudoChannelBits = addressMapping.PSEUDOCHANNEL_BIT)
    {
        std::copy(
            pseudoChannelBits->begin(), pseudoChannelBits->end(), std::back_inserter(vRankBits));
    }

    if (const auto& bankGroupBits = addressMapping.BANKGROUP_BIT)
    {
        std::copy(bankGroupBits->begin(), bankGroupBits->end(), std::back_inserter(vBankGroupBits));
    }

    if (const auto& byteBits = addressMapping.BYTE_BIT)
    {
        std::copy(byteBits->begin(), byteBits->end(), std::back_inserter(vByteBits));
    }

    if (const auto& xorBits = addressMapping.XOR)
    {
        for (const auto& xorBit : *xorBits)
        {
            vXor.emplace_back(xorBit.FIRST, xorBit.SECOND);
        }
    }

    if (const auto& bankBits = addressMapping.BANK_BIT)
    {
        std::copy(bankBits->begin(), bankBits->end(), std::back_inserter(vBankBits));
    }

    if (const auto& rowBits = addressMapping.ROW_BIT)
    {
        std::copy(rowBits->begin(), rowBits->end(), std::back_inserter(vRowBits));
    }

    if (const auto& columnBits = addressMapping.COLUMN_BIT)
    {
        std::copy(columnBits->begin(), columnBits->end(), std::back_inserter(vColumnBits));
    }

    unsigned channels = std::lround(std::pow(2.0, vChannelBits.size()));
    unsigned ranks = std::lround(std::pow(2.0, vRankBits.size()));
    unsigned bankGroups = std::lround(std::pow(2.0, vBankGroupBits.size()));
    unsigned banks = std::lround(std::pow(2.0, vBankBits.size()));
    unsigned rows = std::lround(std::pow(2.0, vRowBits.size()));
    unsigned columns = std::lround(std::pow(2.0, vColumnBits.size()));
    unsigned bytes = std::lround(std::pow(2.0, vByteBits.size()));

    maximumAddress =
        static_cast<uint64_t>(bytes) * columns * rows * banks * bankGroups * ranks * channels - 1;

    auto totalAddressBits = static_cast<unsigned>(std::log2(maximumAddress));
    for (unsigned bitPosition = 0; bitPosition < totalAddressBits; bitPosition++)
    {
        if (std::count(vChannelBits.begin(), vChannelBits.end(), bitPosition) +
                std::count(vRankBits.begin(), vRankBits.end(), bitPosition) +
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

        for (unsigned bitPosition = 0; bitPosition <= static_cast<unsigned>(highestByteBit); bitPosition++)
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

    bankgroupsPerRank = bankGroups;
    bankGroups = bankgroupsPerRank * ranks;

    banksPerGroup = banks;
    banks = banksPerGroup * bankGroups;

    if (memSpec.numberOfChannels != channels || memSpec.ranksPerChannel != ranks ||
        memSpec.bankGroupsPerChannel != bankGroups || memSpec.banksPerChannel != banks ||
        memSpec.rowsPerBank != rows || memSpec.columnsPerRow != columns ||
        memSpec.devicesPerRank * memSpec.bitWidth != bytes * 8)
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
    for (const auto& it : vXor)
    {
        uint64_t xoredBit =
            (((encAddr >> it.first) & UINT64_C(1)) ^ ((encAddr >> it.second) & UINT64_C(1)));
        encAddr &= ~(UINT64_C(1) << it.first);
        encAddr |= xoredBit << it.first;
    }

    DecodedAddress decAddr;

    for (unsigned it = 0; it < vChannelBits.size(); it++)
        decAddr.channel |= ((encAddr >> vChannelBits[it]) & UINT64_C(1)) << it;

    for (unsigned it = 0; it < vRankBits.size(); it++)
        decAddr.rank |= ((encAddr >> vRankBits[it]) & UINT64_C(1)) << it;

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
    for (const auto& it : vXor)
    {
        uint64_t xoredBit =
            (((encAddr >> it.first) & UINT64_C(1)) ^ ((encAddr >> it.second) & UINT64_C(1)));
        encAddr &= ~(UINT64_C(1) << it.first);
        encAddr |= xoredBit << it.first;
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
        for (auto it2 : vXor)
        {
            if (it2.first == vChannelBits[static_cast<std::vector<unsigned>::size_type>(it)])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ch " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vRankBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vRankBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto it2 : vXor)
        {
            if (it2.first == vRankBits[static_cast<std::vector<unsigned>::size_type>(it)])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ra " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vBankGroupBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vBankGroupBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto it2 : vXor)
        {
            if (it2.first == vBankGroupBits[static_cast<std::vector<unsigned>::size_type>(it)])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Bg " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vBankBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vBankBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto it2 : vXor)
        {
            if (it2.first == vBankBits[static_cast<std::vector<unsigned>::size_type>(it)])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ba " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vRowBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vRowBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto it2 : vXor)
        {
            if (it2.first == vRowBits[static_cast<std::vector<unsigned>::size_type>(it)])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ro " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vColumnBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vColumnBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto it2 : vXor)
        {
            if (it2.first == vColumnBits[static_cast<std::vector<unsigned>::size_type>(it)])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Co " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    for (int it = static_cast<int>(vByteBits.size() - 1); it >= 0; it--)
    {
        uint64_t addressBits =
            (UINT64_C(1) << vByteBits[static_cast<std::vector<unsigned>::size_type>(it)]);
        for (auto it2 : vXor)
        {
            if (it2.first == vByteBits[static_cast<std::vector<unsigned>::size_type>(it)])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " By " << std::setw(2) << it << ": " << std::bitset<64>(addressBits)
                  << std::endl;
    }

    std::cout << std::endl;
}

} // namespace DRAMSys
