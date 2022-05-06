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

#ifndef DRAMSYSCONFIGURATION_TRACESETUP_H
#define DRAMSYSCONFIGURATION_TRACESETUP_H

#include "util.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <variant>

namespace DRAMSysConfiguration
{
using json = nlohmann::json;

enum class TrafficInitiatorType
{
    Player,
    Generator,
    Hammer,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(TrafficInitiatorType, {{TrafficInitiatorType::Invalid, nullptr},
                                                    {TrafficInitiatorType::Player, "player"},
                                                    {TrafficInitiatorType::Hammer, "hammer"},
                                                    {TrafficInitiatorType::Generator, "generator"}})

enum class AddressDistribution
{
    Random,
    Sequential,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(AddressDistribution, {{AddressDistribution::Invalid, nullptr},
                                                   {AddressDistribution::Random, "random"},
                                                   {AddressDistribution::Sequential, "sequential"}})

struct TrafficInitiator
{
    virtual ~TrafficInitiator() = 0;

    uint64_t clkMhz;
    std::string name;
    std::optional<unsigned int> maxPendingReadRequests;
    std::optional<unsigned int> maxPendingWriteRequests;
    std::optional<bool> addLengthConverter;
};

struct TracePlayer : public TrafficInitiator
{
};

struct TraceGeneratorState
{
    virtual ~TraceGeneratorState() = 0;
};

struct TraceGeneratorTrafficState : public TraceGeneratorState
{
    uint64_t numRequests;
    double rwRatio;
    AddressDistribution addressDistribution;
    std::optional<uint64_t> addressIncrement;
    std::optional<uint64_t> minAddress;
    std::optional<uint64_t> maxAddress;
    std::optional<uint64_t> clksPerRequest;
    std::optional<std::string> notify;
};

struct TraceGeneratorIdleState : public TraceGeneratorState
{
    uint64_t idleClks;
};

struct TraceGeneratorStateTransition
{
    unsigned int to;
    float probability;
};

struct TraceGenerator : public TrafficInitiator
{
    std::optional<uint64_t> seed;
    std::optional<uint64_t> maxTransactions;
    std::optional<unsigned> dataLength;
    std::map<unsigned int, std::variant<TraceGeneratorIdleState, TraceGeneratorTrafficState>> states;
    std::multimap<unsigned int, TraceGeneratorStateTransition> transitions;
    std::optional<std::string> idleUntil;
};

struct TraceHammer : public TrafficInitiator
{
    uint64_t numRequests;
    uint64_t rowIncrement;
};

struct TraceSetup
{
    std::vector<std::variant<TracePlayer, TraceGenerator, TraceHammer>> initiators;
};

void to_json(json &j, const TraceSetup &c);
void from_json(const json &j, TraceSetup &c);

void from_dump(const std::string &dump, TraceSetup &c);
std::string dump(const TraceSetup &c, unsigned int indentation = -1);

} // namespace Configuration

#endif // DRAMSYSCONFIGURATION_TRACESETUP_H
