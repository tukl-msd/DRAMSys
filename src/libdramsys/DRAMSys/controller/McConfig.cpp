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

#include "McConfig.h"

namespace DRAMSys
{

McConfig::McConfig(const Config::McConfig& config, const MemSpec& memSpec) :
    pagePolicy(config.PagePolicy.value_or(DEFAULT_PAGE_POLICY)),
    scheduler(config.Scheduler.value_or(DEFAULT_SCHEDULER)),
    schedulerBuffer(config.SchedulerBuffer.value_or(DEFAULT_SCHEDULER_BUFFER)),
    lowWatermark(config.LowWatermark.value_or(DEFAULT_LOW_WATERMARK)),
    highWatermark(config.HighWatermark.value_or(DEFAULT_HIGH_WATERMARK)),
    cmdMux(config.CmdMux.value_or(DEFAULT_CMD_MUX)),
    respQueue(config.RespQueue.value_or(DEFAULT_RESP_QUEUE)),
    arbiter(config.Arbiter.value_or(DEFAULT_ARBITER)),
    requestBufferSize(config.RequestBufferSize.value_or(DEFAULT_REQUEST_BUFFER_SIZE)),
    requestBufferSizeRead(config.RequestBufferSizeRead.value_or(DEFAULT_REQUEST_BUFFER_SIZE_READ)),
    requestBufferSizeWrite(
        config.RequestBufferSizeWrite.value_or(DEFAULT_REQUEST_BUFFER_SIZE_WRITE)),
    refreshPolicy(config.RefreshPolicy.value_or(DEFAULT_REFRESH_POLICY)),
    refreshMaxPostponed(config.RefreshMaxPostponed.value_or(DEFAULT_REFRESH_MAX_POSTPONED)),
    refreshMaxPulledin(config.RefreshMaxPulledin.value_or(DEFAULT_REFRESH_MAX_PULLEDIN)),
    powerDownPolicy(config.PowerDownPolicy.value_or(DEFAULT_POWER_DOWN_POLICY)),
    maxActiveTransactions(config.MaxActiveTransactions.value_or(DEFAULT_MAX_ACTIVE_TRANSACTIONS)),
    refreshManagement(config.RefreshManagement.value_or(DEFAULT_REFRESH_MANAGEMENT)),
    arbitrationDelayFw(sc_core::sc_time(
        config.ArbitrationDelayFw.value_or(DEFAULT_ARBITRATION_DELAY_FW_NS), sc_core::SC_NS)),
    arbitrationDelayBw(sc_core::sc_time(
        config.ArbitrationDelayBw.value_or(DEFAULT_ARBITRATION_DELAY_BW_NS), sc_core::SC_NS)),
    thinkDelayFw(
        sc_core::sc_time(config.ThinkDelayFw.value_or(DEFAULT_THINK_DELAY_FW_NS), sc_core::SC_NS)),
    thinkDelayBw(
        sc_core::sc_time(config.ThinkDelayBw.value_or(DEFAULT_THINK_DELAY_BW_NS), sc_core::SC_NS)),
    phyDelayFw(
        sc_core::sc_time(config.PhyDelayFw.value_or(DEFAULT_PHY_DELAY_FW_NS), sc_core::SC_NS)),
    phyDelayBw(
        sc_core::sc_time(config.PhyDelayBw.value_or(DEFAULT_PHY_DELAY_BW_NS), sc_core::SC_NS)),
    blockingReadDelay(sc_core::sc_time(
        config.BlockingReadDelay.value_or(DEFAULT_BLOCKING_READ_DELAY_NS), sc_core::SC_NS)),
    blockingWriteDelay(sc_core::sc_time(
        config.BlockingWriteDelay.value_or(DEFAULT_BLOCKING_WRITE_DELAY_NS), sc_core::SC_NS))

{
    if (schedulerBuffer == Config::SchedulerBufferType::ReadWrite &&
        config.RequestBufferSize.has_value())
    {
        SC_REPORT_WARNING("McConfig",
                          "RequestBufferSize ignored when using ReadWrite SchedulerBuffer. Use "
                          "RequestBufferSizeRead and RequestBufferSizeWrite instead!");
    }

    if (pagePolicy == Config::PagePolicyType::Invalid)
        SC_REPORT_FATAL("McConfig", "Invalid PagePolicy");

    if (scheduler == Config::SchedulerType::Invalid)
        SC_REPORT_FATAL("McConfig", "Invalid Scheduler");

    if (schedulerBuffer == Config::SchedulerBufferType::Invalid)
        SC_REPORT_FATAL("McConfig", "Invalid SchedulerBuffer");

    if (cmdMux == Config::CmdMuxType::Invalid)
        SC_REPORT_FATAL("McConfig", "Invalid CmdMux");

    if (respQueue == Config::RespQueueType::Invalid)
        SC_REPORT_FATAL("McConfig", "Invalid RespQueue");

    if (refreshPolicy == Config::RefreshPolicyType::Invalid)
        SC_REPORT_FATAL("McConfig", "Invalid RefreshPolicy");

    if (powerDownPolicy == Config::PowerDownPolicyType::Invalid)
        SC_REPORT_FATAL("Configuration", "Invalid PowerDownPolicy");

    if (arbiter == Config::ArbiterType::Invalid)
        SC_REPORT_FATAL("Arbiter", "Invalid Arbiter");

    if (requestBufferSize < 1)
        SC_REPORT_FATAL("Configuration", "Minimum request buffer size is 1!");

    if (requestBufferSizeRead < 1)
        SC_REPORT_FATAL("Configuration", "Minimum request buffer size is 1!");

    if (requestBufferSizeWrite < 1)
        SC_REPORT_FATAL("Configuration", "Minimum request buffer size is 1!");

    arbitrationDelayFw = std::round(arbitrationDelayFw / memSpec.tCK) * memSpec.tCK;
    arbitrationDelayBw = std::round(arbitrationDelayBw / memSpec.tCK) * memSpec.tCK;

    thinkDelayFw = std::round(thinkDelayFw / memSpec.tCK) * memSpec.tCK;
    thinkDelayBw = std::round(thinkDelayBw / memSpec.tCK) * memSpec.tCK;

    phyDelayFw = std::round(phyDelayFw / memSpec.tCK) * memSpec.tCK;
    phyDelayBw = std::round(phyDelayBw / memSpec.tCK) * memSpec.tCK;

    blockingReadDelay = std::round(blockingReadDelay / memSpec.tCK) * memSpec.tCK;
    blockingWriteDelay = std::round(blockingWriteDelay / memSpec.tCK) * memSpec.tCK;
}

} // namespace DRAMSys
