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

#ifndef DRAMSYSCONFIGURATION_MCCONFIG_H
#define DRAMSYSCONFIGURATION_MCCONFIG_H

#include "util.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <utility>

namespace DRAMSysConfiguration
{
using json = nlohmann::json;

const std::string mcConfigPath = "configs/mcconfigs";

enum class PagePolicy
{
    Open,
    OpenAdaptive,
    Closed,
    ClosedAdaptive,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(PagePolicy, {
                                             {PagePolicy::Invalid, nullptr},
                                             {PagePolicy::Open, "Open"},
                                             {PagePolicy::OpenAdaptive, "OpenAdaptive"},
                                             {PagePolicy::Closed, "Closed"},
                                             {PagePolicy::ClosedAdaptive, "ClosedAdaptive"},
                                         })

enum class Scheduler
{
    Fifo,
    FrFcfs,
    FrFcfsGrp,
    GrpFrFcfs,
    GrpFrFcfsWm,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(Scheduler, {{Scheduler::Invalid, nullptr},
                                         {Scheduler::Fifo, "Fifo"},
                                         {Scheduler::FrFcfs, "FrFcfs"},
                                         {Scheduler::FrFcfsGrp, "FrFcfsGrp"},
                                         {Scheduler::GrpFrFcfs, "GrpFrFcfs"},
                                         {Scheduler::GrpFrFcfsWm, "GrpFrFcfsWm"}})

enum class SchedulerBuffer
{
    Bankwise,
    ReadWrite,
    Shared,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(SchedulerBuffer, {{SchedulerBuffer::Invalid, nullptr},
                                               {SchedulerBuffer::Bankwise, "Bankwise"},
                                               {SchedulerBuffer::ReadWrite, "ReadWrite"},
                                               {SchedulerBuffer::Shared, "Shared"}})

enum class CmdMux
{
    Oldest,
    Strict,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(CmdMux,
                             {{CmdMux::Invalid, nullptr}, {CmdMux::Oldest, "Oldest"}, {CmdMux::Strict, "Strict"}})

enum class RespQueue
{
    Fifo,
    Reorder,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(RespQueue, {{RespQueue::Invalid, nullptr},
                                         {RespQueue::Fifo, "Fifo"},
                                         {RespQueue::Reorder, "Reorder"}})

enum class RefreshPolicy
{
    NoRefresh,
    AllBank,
    PerBank,
    Per2Bank,
    SameBank,
    Invalid = -1
};

enum class PowerDownPolicy
{
    NoPowerDown,
    Staggered,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerDownPolicy, {{PowerDownPolicy::Invalid, nullptr},
                                               {PowerDownPolicy::NoPowerDown, "NoPowerDown"},
                                               {PowerDownPolicy::Staggered, "Staggered"}})

enum class Arbiter
{
    Simple,
    Fifo,
    Reorder,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(Arbiter, {{Arbiter::Invalid, nullptr},
                                       {Arbiter::Simple, "Simple"},
                                       {Arbiter::Fifo, "Fifo"},
                                       {Arbiter::Reorder, "Reorder"}})

struct McConfig
{
    std::optional<PagePolicy> pagePolicy;
    std::optional<Scheduler> scheduler;
    std::optional<unsigned int> highWatermark;
    std::optional<unsigned int> lowWatermark;
    std::optional<SchedulerBuffer> schedulerBuffer;
    std::optional<unsigned int> requestBufferSize;
    std::optional<CmdMux> cmdMux;
    std::optional<RespQueue> respQueue;
    std::optional<RefreshPolicy> refreshPolicy;
    std::optional<unsigned int> refreshMaxPostponed;
    std::optional<unsigned int> refreshMaxPulledin;
    std::optional<PowerDownPolicy> powerDownPolicy;
    std::optional<Arbiter> arbiter;
    std::optional<unsigned int> maxActiveTransactions;
    std::optional<bool> refreshManagement;
    std::optional<unsigned int> arbitrationDelayFw;
    std::optional<unsigned int> arbitrationDelayBw;
    std::optional<unsigned int> thinkDelayFw;
    std::optional<unsigned int> thinkDelayBw;
    std::optional<unsigned int> phyDelayFw;
    std::optional<unsigned int> phyDelayBw;
};

void to_json(json &j, const McConfig &c);
void from_json(const json &j, McConfig &c);

void to_json(json &j, const RefreshPolicy &r);
void from_json(const json &j, RefreshPolicy &r);

void from_dump(const std::string &dump, McConfig &c);
std::string dump(const McConfig &c, unsigned int indentation = -1);

} // namespace Configuration

#endif // DRAMSYSCONFIGURATION_MCCONFIG_H
