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

#ifndef DRAMSYSCONFIGURATION_MCCONFIG_H
#define DRAMSYSCONFIGURATION_MCCONFIG_H

#include "DRAMSys/util/json.h"

#include <optional>
#include <string>
#include <utility>

namespace DRAMSys::Config
{

enum class PagePolicyType
{
    Open,
    OpenAdaptive,
    Closed,
    ClosedAdaptive,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(PagePolicyType,
                             {
                                 {PagePolicyType::Invalid, nullptr},
                                 {PagePolicyType::Open, "Open"},
                                 {PagePolicyType::OpenAdaptive, "OpenAdaptive"},
                                 {PagePolicyType::Closed, "Closed"},
                                 {PagePolicyType::ClosedAdaptive, "ClosedAdaptive"},
                             })

enum class SchedulerType
{
    Fifo,
    FrFcfs,
    FrFcfsGrp,
    GrpFrFcfs,
    GrpFrFcfsWm,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(SchedulerType,
                             {{SchedulerType::Invalid, nullptr},
                              {SchedulerType::Fifo, "Fifo"},
                              {SchedulerType::FrFcfs, "FrFcfs"},
                              {SchedulerType::FrFcfsGrp, "FrFcfsGrp"},
                              {SchedulerType::GrpFrFcfs, "GrpFrFcfs"},
                              {SchedulerType::GrpFrFcfsWm, "GrpFrFcfsWm"}})

enum class SchedulerBufferType
{
    Bankwise,
    ReadWrite,
    Shared,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(SchedulerBufferType,
                             {{SchedulerBufferType::Invalid, nullptr},
                              {SchedulerBufferType::Bankwise, "Bankwise"},
                              {SchedulerBufferType::ReadWrite, "ReadWrite"},
                              {SchedulerBufferType::Shared, "Shared"}})

enum class CmdMuxType
{
    Oldest,
    Strict,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(CmdMuxType,
                             {{CmdMuxType::Invalid, nullptr},
                              {CmdMuxType::Oldest, "Oldest"},
                              {CmdMuxType::Strict, "Strict"}})

enum class RespQueueType
{
    Fifo,
    Reorder,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(RespQueueType,
                             {{RespQueueType::Invalid, nullptr},
                              {RespQueueType::Fifo, "Fifo"},
                              {RespQueueType::Reorder, "Reorder"}})

enum class RefreshPolicyType
{
    NoRefresh,
    AllBank,
    PerBank,
    Per2Bank,
    SameBank,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(RefreshPolicyType,
                             {{RefreshPolicyType::Invalid, nullptr},
                              {RefreshPolicyType::NoRefresh, "NoRefresh"},
                              {RefreshPolicyType::AllBank, "AllBank"},
                              {RefreshPolicyType::PerBank, "PerBank"},
                              {RefreshPolicyType::Per2Bank, "Per2Bank"},
                              {RefreshPolicyType::SameBank, "SameBank"},

                              // Alternative conversions to provide backwards-compatibility
                              // when deserializing. Will not be used for serializing.
                              {RefreshPolicyType::AllBank, "Rankwise"},
                              {RefreshPolicyType::PerBank, "Bankwise"},
                              {RefreshPolicyType::SameBank, "Groupwise"}})

enum class PowerDownPolicyType
{
    NoPowerDown,
    Staggered,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerDownPolicyType,
                             {{PowerDownPolicyType::Invalid, nullptr},
                              {PowerDownPolicyType::NoPowerDown, "NoPowerDown"},
                              {PowerDownPolicyType::Staggered, "Staggered"}})

enum class ArbiterType
{
    Simple,
    Fifo,
    Reorder,
    Invalid = -1
};

NLOHMANN_JSON_SERIALIZE_ENUM(ArbiterType,
                             {{ArbiterType::Invalid, nullptr},
                              {ArbiterType::Simple, "Simple"},
                              {ArbiterType::Fifo, "Fifo"},
                              {ArbiterType::Reorder, "Reorder"}})

struct McConfig
{
    static constexpr std::string_view KEY = "mcconfig";
    static constexpr std::string_view SUB_DIR = "mcconfig";

    std::optional<PagePolicyType> PagePolicy;
    std::optional<SchedulerType> Scheduler;
    std::optional<unsigned int> HighWatermark;
    std::optional<unsigned int> LowWatermark;
    std::optional<SchedulerBufferType> SchedulerBuffer;
    std::optional<unsigned int> RequestBufferSize;
    std::optional<unsigned int> RequestBufferSizeRead;
    std::optional<unsigned int> RequestBufferSizeWrite;
    std::optional<CmdMuxType> CmdMux;
    std::optional<RespQueueType> RespQueue;
    std::optional<RefreshPolicyType> RefreshPolicy;
    std::optional<unsigned int> RefreshMaxPostponed;
    std::optional<unsigned int> RefreshMaxPulledin;
    std::optional<PowerDownPolicyType> PowerDownPolicy;
    std::optional<ArbiterType> Arbiter;
    std::optional<unsigned int> MaxActiveTransactions;
    std::optional<bool> RefreshManagement;
    std::optional<unsigned int> ArbitrationDelayFw;
    std::optional<unsigned int> ArbitrationDelayBw;
    std::optional<unsigned int> ThinkDelayFw;
    std::optional<unsigned int> ThinkDelayBw;
    std::optional<unsigned int> PhyDelayFw;
    std::optional<unsigned int> PhyDelayBw;
    std::optional<unsigned int> BlockingReadDelay;
    std::optional<unsigned int> BlockingWriteDelay;
};

NLOHMANN_JSONIFY_ALL_THINGS(McConfig,
                            PagePolicy,
                            Scheduler,
                            HighWatermark,
                            LowWatermark,
                            SchedulerBuffer,
                            RequestBufferSize,
                            RequestBufferSizeRead,
                            RequestBufferSizeWrite,
                            CmdMux,
                            RespQueue,
                            RefreshPolicy,
                            RefreshMaxPostponed,
                            RefreshMaxPulledin,
                            PowerDownPolicy,
                            Arbiter,
                            MaxActiveTransactions,
                            RefreshManagement,
                            ArbitrationDelayFw,
                            ArbitrationDelayBw,
                            ThinkDelayFw,
                            ThinkDelayBw,
                            PhyDelayFw,
                            PhyDelayBw,
                            BlockingReadDelay,
                            BlockingWriteDelay)

} // namespace DRAMSys::Config

#endif // DRAMSYSCONFIGURATION_MCCONFIG_H
