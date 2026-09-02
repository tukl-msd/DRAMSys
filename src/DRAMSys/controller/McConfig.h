/*
 * Copyright (c) 2024, RPTU Kaiserslautern-Landau
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

#ifndef MC_CONFIG_H
#define MC_CONFIG_H

#include <DRAMSys/configuration/json/McConfig.h>
#include <DRAMSys/configuration/memspec/MemSpec.h>

#include <systemc>

namespace DRAMSys
{

struct McConfig
{
    McConfig(const Config::McConfig& config, sc_core::sc_time tCK);

    enum class PagePolicy : uint8_t
    {
        Open,
        OpenAdaptive,
        Closed,
        ClosedAdaptive,
    } pagePolicy;
    enum class Scheduler : uint8_t
    {
        Fifo,
        FrFcfs,
        FrFcfsGrp,
        GrpFrFcfs,
        GrpFrFcfsWm,
    } scheduler;
    enum class SchedulerBuffer : uint8_t
    {
        Bankwise,
        ReadWrite,
        Shared,
    } schedulerBuffer;

    unsigned int lowWatermark;
    unsigned int highWatermark;

    enum class CmdMux : uint8_t
    {
        Oldest,
        Strict,
    } cmdMux;
    enum class RespQueue : uint8_t
    {
        Fifo,
        Reorder,
    } respQueue;
    enum class Arbiter : uint8_t
    {
        Simple,
        Fifo,
        Reorder,
    } arbiter;

    unsigned int requestBufferSize;
    unsigned int requestBufferSizeRead;
    unsigned int requestBufferSizeWrite;

    enum class RefreshPolicy : uint8_t
    {
        NoRefresh,
        AllBank,
        PerBank,
        Per2Bank,
        SameBank,
        DualBank,
    } refreshPolicy;
    unsigned int refreshMaxPostponed;
    unsigned int refreshMaxPulledin;

    enum class PowerDownPolicy : uint8_t
    {
        NoPowerDown,
        Staggered,
    } powerDownPolicy;
    unsigned int maxActiveTransactions;
    bool refreshManagement;

    unsigned int gearing;

    sc_core::sc_time arbitrationDelayFw;
    sc_core::sc_time arbitrationDelayBw;
    sc_core::sc_time thinkDelayFw;
    sc_core::sc_time thinkDelayBw;
    sc_core::sc_time phyDelayFw;
    sc_core::sc_time phyDelayBw;
    sc_core::sc_time blockingReadDelay;
    sc_core::sc_time blockingWriteDelay;

    static constexpr Config::PagePolicyType DEFAULT_PAGE_POLICY = Config::PagePolicyType::Open;
    static constexpr Config::SchedulerType DEFAULT_SCHEDULER = Config::SchedulerType::FrFcfs;
    static constexpr Config::SchedulerBufferType DEFAULT_SCHEDULER_BUFFER =
        Config::SchedulerBufferType::Bankwise;
    static constexpr unsigned int DEFAULT_LOW_WATERMARK = 0;
    static constexpr unsigned int DEFAULT_HIGH_WATERMARK = 0;
    static constexpr Config::CmdMuxType DEFAULT_CMD_MUX = Config::CmdMuxType::Oldest;
    static constexpr Config::RespQueueType DEFAULT_RESP_QUEUE = Config::RespQueueType::Fifo;
    static constexpr Config::ArbiterType DEFAULT_ARBITER = Config::ArbiterType::Simple;
    static constexpr unsigned int DEFAULT_REQUEST_BUFFER_SIZE = 8;
    static constexpr unsigned int DEFAULT_REQUEST_BUFFER_SIZE_READ = 8;
    static constexpr unsigned int DEFAULT_REQUEST_BUFFER_SIZE_WRITE = 8;
    static constexpr Config::RefreshPolicyType DEFAULT_REFRESH_POLICY =
        Config::RefreshPolicyType::AllBank;
    static constexpr unsigned int DEFAULT_REFRESH_MAX_POSTPONED = 0;
    static constexpr unsigned int DEFAULT_REFRESH_MAX_PULLEDIN = 0;
    static constexpr Config::PowerDownPolicyType DEFAULT_POWER_DOWN_POLICY =
        Config::PowerDownPolicyType::NoPowerDown;
    static constexpr unsigned int DEFAULT_MAX_ACTIVE_TRANSACTIONS = 64;
    static constexpr bool DEFAULT_REFRESH_MANAGEMENT = false;
    static constexpr unsigned DEFAULT_ARBITRATION_DELAY_FW = 0;
    static constexpr unsigned DEFAULT_ARBITRATION_DELAY_BW = 0;
    static constexpr unsigned DEFAULT_THINK_DELAY_FW = 1;
    static constexpr unsigned DEFAULT_THINK_DELAY_BW = 0;
    static constexpr unsigned DEFAULT_PHY_DELAY_FW = 0;
    static constexpr unsigned DEFAULT_PHY_DELAY_BW = 0;
    static constexpr unsigned DEFAULT_BLOCKING_READ_DELAY = 60;
    static constexpr unsigned DEFAULT_BLOCKING_WRITE_DELAY = 60;
    static constexpr unsigned DEFAULT_GEARING = 1;
};

} // namespace DRAMSys

#endif
