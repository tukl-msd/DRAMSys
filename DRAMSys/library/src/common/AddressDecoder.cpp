/*
 * Copyright (c) 2018, Technische Universität Kaiserslautern
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
 */

#include <cmath>
#include <bitset>

#include "AddressDecoder.h"
#include "utils.h"
#include "../configuration/Configuration.h"

using json = nlohmann::json;

unsigned int AddressDecoder::getUnsignedAttrFromJson(nlohmann::json obj, std::string strName)
{
    if (!obj[strName].empty())
    {
        if (obj[strName].is_number_unsigned())
        {
            return obj[strName];
        }
        else
        {
            SC_REPORT_FATAL("AddressDecoder", ("Attribute " + strName + " is not a number.").c_str());
            return (unsigned)(-1);
        }
    }
    else
    {
        SC_REPORT_FATAL("AddressDecoder", ("Attribute " + strName + " is empty or not found.").c_str());
        return (unsigned)(-1);
    }
}

std::vector<unsigned> AddressDecoder::getAttrToVectorFromJson(nlohmann::json obj, std::string strName)
{
    std::vector<unsigned> vParameter;
    if (!obj[strName].empty())
    {
        for (auto it : obj[strName].items())
        {
            auto valor = it.value();
            if (valor.is_number_unsigned())
                vParameter.push_back(it.value());
            else
                SC_REPORT_FATAL("AddressDecoder", ("Attribute " + strName + " is not a number.").c_str());
        }
    }
    return vParameter;
}

AddressDecoder::AddressDecoder(std::string pathToAddressMapping)
{
    json addrFile = parseJSON(pathToAddressMapping);
    json mapping;
    if (addrFile["CONGEN"].empty())
        SC_REPORT_FATAL("AddressDecorder", "Root node name differs from \"CONGEN\". File format not supported.");

    // Load address mapping
    if (!addrFile["CONGEN"]["SOLUTION"].empty())
    {
        bool foundID0 = false;
        for (auto it : addrFile["CONGEN"]["SOLUTION"].items())
        {
            if (getUnsignedAttrFromJson(it.value(), "ID") == 0)
            {
                foundID0 = true;
                mapping = it.value();
                break;
            }
        }
        if (!foundID0)
            SC_REPORT_FATAL("AddressDecoder", "No mapping with ID 0 was found.");
    }
    else
        mapping = addrFile["CONGEN"];

    for (auto xorItem : mapping["XOR"].items())
    {
        auto value = xorItem.value();
        if (!value.empty())
            vXor.push_back(std::pair<unsigned, unsigned>(getUnsignedAttrFromJson(value, "FIRST"),
                                                         getUnsignedAttrFromJson(value, "SECOND")));
    }

    vChannelBits = getAttrToVectorFromJson(mapping,"CHANNEL_BIT");
    vRankBits = getAttrToVectorFromJson(mapping,"RANK_BIT");
    vBankGroupBits = getAttrToVectorFromJson(mapping,"BANKGROUP_BIT");
    vBankBits = getAttrToVectorFromJson(mapping,"BANK_BIT");
    vRowBits = getAttrToVectorFromJson(mapping,"ROW_BIT");
    vColumnBits = getAttrToVectorFromJson(mapping,"COLUMN_BIT");
    vByteBits = getAttrToVectorFromJson(mapping,"BYTE_BIT");

    uint64_t channels = (uint64_t)(pow(2.0, vChannelBits.size()) + 0.5);
    uint64_t ranks = (uint64_t)(pow(2.0, vRankBits.size()) + 0.5);
    uint64_t bankgroups = (uint64_t)(pow(2.0, vBankGroupBits.size()) + 0.5);
    uint64_t banks = (uint64_t)(pow(2.0, vBankBits.size()) + 0.5);
    uint64_t rows = (uint64_t)(pow(2.0, vRowBits.size()) + 0.5);
    uint64_t columns = (uint64_t)(pow(2.0, vColumnBits.size()) + 0.5);
    uint64_t bytes = (uint64_t)(pow(2.0, vByteBits.size()) + 0.5);

    maximumAddress = bytes * columns * rows * banks * bankgroups * ranks * channels - 1;

    banksPerGroup = banks;
    banks = banksPerGroup * bankgroups * ranks;

    bankgroupsPerRank = bankgroups;
    bankgroups = bankgroupsPerRank * ranks;

    Configuration &config = Configuration::getInstance();
    MemSpec *memSpec = config.memSpec;

    if (memSpec->numberOfChannels != channels || memSpec->numberOfRanks != ranks
            || memSpec->numberOfBankGroups != bankgroups || memSpec->numberOfBanks != banks
            || memSpec->numberOfRows != rows || memSpec->numberOfColumns != columns
            || memSpec->numberOfDevicesOnDIMM * memSpec->bitWidth != bytes * 8)
        SC_REPORT_FATAL("AddressDecoder", "Memspec and address mapping do not match");
}

DecodedAddress AddressDecoder::decodeAddress(uint64_t encAddr)
{
    if (encAddr > maximumAddress)
        SC_REPORT_WARNING("AddressDecoder", ("Address " + std::to_string(encAddr) + " out of range (maximum address is " + std::to_string(maximumAddress) + ")").c_str());

    // Apply XOR
    // For each used xor:
    //   Get the first bit and second bit. Apply a bitwise xor operator and save it back to the first bit.
    for (auto it = vXor.begin(); it != vXor.end(); it++)
    {
        uint64_t xoredBit;
        xoredBit = (((encAddr >> it->first) & UINT64_C(1)) ^ ((encAddr >> it->second) & UINT64_C(1)));
        encAddr &= ~(UINT64_C(1) << it->first);
        encAddr |= xoredBit << it->first;
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

void AddressDecoder::print()
{
    std::cout << headline << std::endl;
    std::cout << "Used Address Mapping:" << std::endl;
    std::cout << std::endl;

    for (int it = vChannelBits.size() - 1; it >= 0; it--)
    {
        uint64_t addressBits = (UINT64_C(1) << vChannelBits[it]);
        for (auto it2 : vXor)
        {
            if (it2.first == vChannelBits[it])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ch " << std::setw(2) << it << ": " << std::bitset<64>(addressBits) << std::endl;
    }

    for (int it = vRankBits.size() - 1; it >= 0; it--)
    {
        uint64_t addressBits = (UINT64_C(1) << vRankBits[it]);
        for (auto it2 : vXor)
        {
            if (it2.first == vRankBits[it])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ra " << std::setw(2) << it << ": " << std::bitset<64>(addressBits) << std::endl;
    }

    for (int it = vBankGroupBits.size() - 1; it >= 0; it--)
    {
        uint64_t addressBits = (UINT64_C(1) << vBankGroupBits[it]);
        for (auto it2 : vXor)
        {
            if (it2.first == vBankGroupBits[it])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Bg " << std::setw(2) << it << ": " << std::bitset<64>(addressBits) << std::endl;
    }

    for (int it = vBankBits.size() - 1; it >= 0; it--)
    {
        uint64_t addressBits = (UINT64_C(1) << vBankBits[it]);
        for (auto it2 : vXor)
        {
            if (it2.first == vBankBits[it])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ba " << std::setw(2) << it << ": " << std::bitset<64>(addressBits) << std::endl;
    }

    for (int it = vRowBits.size() - 1; it >= 0; it--)
    {
        uint64_t addressBits = (UINT64_C(1) << vRowBits[it]);
        for (auto it2 : vXor)
        {
            if (it2.first == vRowBits[it])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Ro " << std::setw(2) << it << ": " << std::bitset<64>(addressBits) << std::endl;
    }

    for (int it = vColumnBits.size() - 1; it >= 0; it--)
    {
        uint64_t addressBits = (UINT64_C(1) << vColumnBits[it]);
        for (auto it2 : vXor)
        {
            if (it2.first == vColumnBits[it])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " Co " << std::setw(2) << it << ": " << std::bitset<64>(addressBits) << std::endl;
    }

    for (int it = vByteBits.size() - 1; it >= 0; it--)
    {
        uint64_t addressBits = (UINT64_C(1) << vByteBits[it]);
        for (auto it2 : vXor)
        {
            if (it2.first == vByteBits[it])
                addressBits |= (UINT64_C(1) << it2.second);
        }
        std::cout << " By " << std::setw(2) << it << ": " << std::bitset<64>(addressBits) << std::endl;
    }

    std::cout << std::endl;
}

