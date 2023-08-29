/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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

#include "DRAMSys/util/json.h"

#include <optional>
#include <variant>

namespace DRAMSys::Config
{

enum class TrafficInitiatorType
{
    Player,
    Generator,
    Hammer,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(TrafficInitiatorType,
                             {{TrafficInitiatorType::Invalid, nullptr},
                              {TrafficInitiatorType::Player, "player"},
                              {TrafficInitiatorType::Hammer, "hammer"},
                              {TrafficInitiatorType::Generator, "generator"}})

enum class AddressDistribution
{
    Random,
    Sequential,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(AddressDistribution,
                             {{AddressDistribution::Invalid, nullptr},
                              {AddressDistribution::Random, "random"},
                              {AddressDistribution::Sequential, "sequential"}})

struct TracePlayer
{
    uint64_t clkMhz{};
    std::string name;
    std::optional<unsigned int> maxPendingReadRequests;
    std::optional<unsigned int> maxPendingWriteRequests;
};

NLOHMANN_JSONIFY_ALL_THINGS(
    TracePlayer, clkMhz, name, maxPendingReadRequests, maxPendingWriteRequests)

struct TrafficGeneratorActiveState
{
    unsigned int id{};

    uint64_t numRequests{};
    double rwRatio{};
    AddressDistribution addressDistribution;
    std::optional<uint64_t> addressIncrement;
    std::optional<uint64_t> minAddress;
    std::optional<uint64_t> maxAddress;
};

NLOHMANN_JSONIFY_ALL_THINGS(TrafficGeneratorActiveState,
                            id,
                            numRequests,
                            rwRatio,
                            addressDistribution,
                            addressIncrement,
                            minAddress,
                            maxAddress)

struct TrafficGeneratorIdleState
{
    unsigned int id;

    uint64_t idleClks;
};

NLOHMANN_JSONIFY_ALL_THINGS(TrafficGeneratorIdleState, id, idleClks)

struct TrafficGeneratorStateTransition
{
    unsigned int from;
    unsigned int to;
    float probability;
};

NLOHMANN_JSONIFY_ALL_THINGS(TrafficGeneratorStateTransition, from, to, probability)

struct TrafficGenerator
{
    uint64_t clkMhz{};
    std::string name;
    std::optional<unsigned int> maxPendingReadRequests;
    std::optional<unsigned int> maxPendingWriteRequests;

    std::optional<uint64_t> seed;
    std::optional<uint64_t> maxTransactions;
    std::optional<unsigned> dataLength;
    std::optional<unsigned> dataAlignment;

    uint64_t numRequests{};
    double rwRatio{};
    AddressDistribution addressDistribution;
    std::optional<uint64_t> addressIncrement;
    std::optional<uint64_t> minAddress;
    std::optional<uint64_t> maxAddress;
};

NLOHMANN_JSONIFY_ALL_THINGS(TrafficGenerator,
                            clkMhz,
                            name,
                            maxPendingReadRequests,
                            maxPendingWriteRequests,
                            seed,
                            maxTransactions,
                            dataLength,
                            dataAlignment,
                            numRequests,
                            rwRatio,
                            addressDistribution,
                            addressIncrement,
                            minAddress,
                            maxAddress)

struct TrafficGeneratorStateMachine
{
    uint64_t clkMhz{};
    std::string name;
    std::optional<unsigned int> maxPendingReadRequests;
    std::optional<unsigned int> maxPendingWriteRequests;

    std::optional<uint64_t> seed;
    std::optional<uint64_t> maxTransactions;
    std::optional<unsigned> dataLength;
    std::optional<unsigned> dataAlignment;
    std::vector<std::variant<TrafficGeneratorActiveState, TrafficGeneratorIdleState>> states;
    std::vector<TrafficGeneratorStateTransition> transitions;
};

NLOHMANN_JSONIFY_ALL_THINGS(TrafficGeneratorStateMachine,
                            clkMhz,
                            name,
                            maxPendingReadRequests,
                            maxPendingWriteRequests,
                            seed,
                            maxTransactions,
                            dataLength,
                            dataAlignment,
                            states,
                            transitions)

struct RowHammer
{
    uint64_t clkMhz{};
    std::string name;
    std::optional<unsigned int> maxPendingReadRequests;
    std::optional<unsigned int> maxPendingWriteRequests;

    uint64_t numRequests{};
    uint64_t rowIncrement{};
};

NLOHMANN_JSONIFY_ALL_THINGS(RowHammer,
                            clkMhz,
                            name,
                            maxPendingReadRequests,
                            maxPendingWriteRequests,
                            numRequests,
                            rowIncrement)

struct TraceSetupConstants
{
    static constexpr std::string_view KEY = "tracesetup";
    static constexpr std::string_view SUB_DIR = "tracesetup";
};

using Initiator =
    std::variant<TracePlayer, TrafficGenerator, TrafficGeneratorStateMachine, RowHammer>;

} // namespace DRAMSys::Config

#endif // DRAMSYSCONFIGURATION_TRACESETUP_H
