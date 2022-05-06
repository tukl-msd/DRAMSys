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

#ifndef DRAMSYSCONFIGURATION_ADDRESSMAPPING_H
#define DRAMSYSCONFIGURATION_ADDRESSMAPPING_H

#include "util.h"

#include <nlohmann/json.hpp>
#include <optional>

namespace DRAMSysConfiguration
{
using json = nlohmann::json;

const std::string addressMappingPath = "configs/amconfigs";

struct XorPair
{
    unsigned int first;
    unsigned int second;
};

void to_json(json &j, const XorPair &x);
void from_json(const json &j, XorPair &x);

struct AddressMapping
{
    std::optional<std::vector<unsigned int>> byteBits;
    std::optional<std::vector<unsigned int>> columnBits;
    std::optional<std::vector<unsigned int>> rowBits;
    std::optional<std::vector<unsigned int>> bankBits;
    std::optional<std::vector<unsigned int>> bankGroupBits;
    std::optional<std::vector<unsigned int>> rankBits;
    std::optional<std::vector<unsigned int>> channelBits;
    std::optional<std::vector<XorPair>> xorBits;
};

void to_json(json &j, const AddressMapping &m);
void from_json(const json &j, AddressMapping &m);

void from_dump(const std::string &dump, AddressMapping &c);
std::string dump(const AddressMapping &c, unsigned int indentation = -1);

} // namespace Configuration

#endif // DRAMSYSCONFIGURATION_ADDRESSMAPPING_H
