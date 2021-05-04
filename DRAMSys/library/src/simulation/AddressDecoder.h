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

#ifndef ADDRESSDECODER_H
#define ADDRESSDECODER_H

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

#include "../common/third_party/nlohmann/single_include/nlohmann/json.hpp"

struct DecodedAddress
{
    DecodedAddress(unsigned channel, unsigned rank,
                   unsigned bankgroup, unsigned bank,
                   unsigned row, unsigned column, unsigned bytes)
        : channel(channel), rank(rank), bankgroup(bankgroup),
          bank(bank), row(row), column(column), byte(bytes) {}

    DecodedAddress()
    : channel(0), rank(0), bankgroup(0),
      bank(0), row(0), column(0), byte(0) {}

    unsigned channel;
    unsigned rank;
    unsigned bankgroup;
    unsigned bank;
    unsigned row;
    unsigned column;
    unsigned byte;
};

class AddressDecoder
{
public:
    AddressDecoder(std::string);
    DecodedAddress decodeAddress(uint64_t addr);
    void print();

private:
    std::vector<unsigned> getAttrToVectorFromJson(nlohmann::json obj, std::string strName);
    unsigned int getUnsignedAttrFromJson(nlohmann::json obj, std::string strName);

    unsigned banksPerGroup;
    unsigned bankgroupsPerRank;

    uint64_t maximumAddress;

    // This container stores for each used xor gate a pair of address bits, the first bit is overwritten with the result
    std::vector<std::pair<unsigned, unsigned>> vXor;
    std::vector<unsigned> vChannelBits;
    std::vector<unsigned> vRankBits;
    std::vector<unsigned> vBankGroupBits;
    std::vector<unsigned> vBankBits;
    std::vector<unsigned> vRowBits;
    std::vector<unsigned> vColumnBits;
    std::vector<unsigned> vByteBits;
};

#endif // ADDRESSDECODER_H
