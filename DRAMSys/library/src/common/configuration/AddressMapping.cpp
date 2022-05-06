/*
 * Copyright (c) 2021, Technische Universität Kaiserslautern
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

#include <iostream>

namespace DRAMSysConfiguration
{

void to_json(json &j, const AddressMapping &m)
{
    json congen = json{};

    congen = json{{"BYTE_BIT", m.byteBits},
                  {"COLUMN_BIT", m.columnBits},
                  {"ROW_BIT", m.rowBits},
                  {"BANK_BIT", m.bankBits},
                  {"BANKGROUP_BIT", m.bankGroupBits},
                  {"RANK_BIT", m.rankBits},
                  {"CHANNEL_BIT", m.channelBits},
                  {"XOR", m.xorBits}};

    remove_null_values(congen);

    j["CONGEN"] = congen;
}

void from_json(const json &j, AddressMapping &m)
{
    json j_addressmapping = get_config_json(j, addressMappingPath, "CONGEN");

    json congen;
    if (j_addressmapping["CONGEN"].is_null())
        congen = j_addressmapping;
    else
        congen = j_addressmapping["CONGEN"];

    if (congen.contains("BYTE_BIT"))
        congen.at("BYTE_BIT").get_to(m.byteBits);

    if (congen.contains("COLUMN_BIT"))
        congen.at("COLUMN_BIT").get_to(m.columnBits);

    if (congen.contains("ROW_BIT"))
        congen.at("ROW_BIT").get_to(m.rowBits);

    if (congen.contains("BANK_BIT"))
        congen.at("BANK_BIT").get_to(m.bankBits);

    if (congen.contains("BANKGROUP_BIT"))
        congen.at("BANKGROUP_BIT").get_to(m.bankGroupBits);

    if (congen.contains("RANK_BIT"))
        congen.at("RANK_BIT").get_to(m.rankBits);

    // HBM pseudo channels are internally modelled as ranks
    if (congen.contains("PSEUDOCHANNEL_BIT"))
        congen.at("PSEUDOCHANNEL_BIT").get_to(m.rankBits);

    if (congen.contains("CHANNEL_BIT"))
        congen.at("CHANNEL_BIT").get_to(m.channelBits);

    if (congen.contains("XOR"))
        congen.at("XOR").get_to(m.xorBits);
}

void to_json(json &j, const XorPair &x)
{
    j = json{{"FIRST", x.first}, {"SECOND", x.second}};
}

void from_json(const json &j, XorPair &x)
{
    j.at("FIRST").get_to(x.first);
    j.at("SECOND").get_to(x.second);
}

void from_dump(const std::string &dump, AddressMapping &c)
{
    json json_addressmapping = json::parse(dump).at("addressmapping");
    json_addressmapping.get_to(c);
}

std::string dump(const AddressMapping &c, unsigned int indentation)
{
    json json_addressmapping;
    json_addressmapping["addressmapping"] = c;
    return json_addressmapping.dump(indentation);
}

} // namespace DRAMSysConfiguration
