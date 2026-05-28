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
 * Author:
 *    Derek Christ
 */

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace DRAMSys::Statistics
{

enum class Quantity : std::uint8_t
{
    Bandwidth, // B/s
    Time,      // s
    Energy,    // J
    Count,
    Percentage
};

class Stat
{
public:
    virtual ~Stat() = default;

    Stat(const Stat&) = default;
    Stat& operator=(const Stat&) = default;

    Stat(Stat&&) = delete;
    Stat& operator=(Stat&&) = delete;

    Stat(std::string name, std::string description, Quantity quantity) :
        name(std::move(name)),
        description(std::move(description)),
        quantity(quantity)
    {
    }

    std::string name;
    std::string description;
    Quantity quantity;
};

class ScalarStat : public Stat
{
public:
    ScalarStat(std::string name, std::string description, Quantity quantity) :
        Stat(std::move(name), std::move(description), quantity)
    {
    }

    ScalarStat& operator=(double value)
    {
        this->value = value;
        return *this;
    }

    double value = 0;
};

class VectorStat : public Stat
{
public:
    VectorStat(std::string name, std::string description, Quantity quantity) :
        Stat(std::move(name), std::move(description), quantity)
    {
    }

    std::vector<double> values;
};

} // namespace DRAMSys::Statistics
