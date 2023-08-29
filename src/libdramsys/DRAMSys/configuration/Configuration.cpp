/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Éder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 *    Luiza Correa
 *    Derek Christ
 */

#include "Configuration.h"

#include "DRAMSys/configuration/memspec/MemSpecDDR3.h"
#include "DRAMSys/configuration/memspec/MemSpecDDR4.h"
#include "DRAMSys/configuration/memspec/MemSpecGDDR5.h"
#include "DRAMSys/configuration/memspec/MemSpecGDDR5X.h"
#include "DRAMSys/configuration/memspec/MemSpecGDDR6.h"
#include "DRAMSys/configuration/memspec/MemSpecHBM2.h"
#include "DRAMSys/configuration/memspec/MemSpecLPDDR4.h"
#include "DRAMSys/configuration/memspec/MemSpecSTTMRAM.h"
#include "DRAMSys/configuration/memspec/MemSpecWideIO.h"
#include "DRAMSys/configuration/memspec/MemSpecWideIO2.h"

#ifdef DDR5_SIM
#include "DRAMSys/configuration/memspec/MemSpecDDR5.h"
#endif
#ifdef LPDDR5_SIM
#include "DRAMSys/configuration/memspec/MemSpecLPDDR5.h"
#endif
#ifdef HBM3_SIM
#include "DRAMSys/configuration/memspec/MemSpecHBM3.h"
#endif

using namespace sc_core;

namespace DRAMSys
{

enum sc_time_unit string2TimeUnit(const std::string& s)
{
    if (s == "s")
        return SC_SEC;

    if (s == "ms")
        return SC_MS;

    if (s == "us")
        return SC_US;

    if (s == "ns")
        return SC_NS;

    if (s == "ps")
        return SC_PS;

    if (s == "fs")
        return SC_FS;

    SC_REPORT_FATAL("Configuration", ("Could not convert to enum sc_time_unit: " + s).c_str());
    throw;
}

void Configuration::loadSimConfig(const DRAMSys::Config::SimConfig& simConfig)
{
    addressOffset = simConfig.AddressOffset.value_or(addressOffset);
    checkTLM2Protocol = simConfig.CheckTLM2Protocol.value_or(checkTLM2Protocol);
    databaseRecording = simConfig.DatabaseRecording.value_or(databaseRecording);
    debug = simConfig.Debug.value_or(debug);
    enableWindowing = simConfig.EnableWindowing.value_or(enableWindowing);
    simulationName = simConfig.SimulationName.value_or(simulationName);
    simulationProgressBar = simConfig.SimulationProgressBar.value_or(simulationProgressBar);
    useMalloc = simConfig.UseMalloc.value_or(useMalloc);

    if (const auto& _storeMode = simConfig.StoreMode)
        storeMode = [=]
        {
            switch (*_storeMode)
            {
            case DRAMSys::Config::StoreModeType::NoStorage:
                return StoreMode::NoStorage;
            case DRAMSys::Config::StoreModeType::Store:
                return StoreMode::Store;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid StoreMode");
                return StoreMode::NoStorage; // Silence Warning
            }
        }();

    windowSize = simConfig.WindowSize.value_or(windowSize);
    if (windowSize == 0)
        SC_REPORT_FATAL("Configuration", "Minimum window size is 1");

    powerAnalysis = simConfig.PowerAnalysis.value_or(powerAnalysis);
#ifndef DRAMPOWER
    if (powerAnalysis)
        SC_REPORT_FATAL("Configuration",
                        "Power analysis is only supported with included DRAMPower library!");
#endif
}

void Configuration::loadMCConfig(const DRAMSys::Config::McConfig& mcConfig)
{
    if (const auto& _pagePolicy = mcConfig.PagePolicy)
        pagePolicy = [=]
        {
            switch (*_pagePolicy)
            {
            case DRAMSys::Config::PagePolicyType::Open:
                return PagePolicy::Open;
            case DRAMSys::Config::PagePolicyType::OpenAdaptive:
                return PagePolicy::OpenAdaptive;
            case DRAMSys::Config::PagePolicyType::Closed:
                return PagePolicy::Closed;
            case DRAMSys::Config::PagePolicyType::ClosedAdaptive:
                return PagePolicy::ClosedAdaptive;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid PagePolicy");
                return PagePolicy::Open; // Silence Warning
            }
        }();

    if (const auto& _scheduler = mcConfig.Scheduler)
        scheduler = [=]
        {
            switch (*_scheduler)
            {
            case DRAMSys::Config::SchedulerType::Fifo:
                return Scheduler::Fifo;
            case DRAMSys::Config::SchedulerType::FrFcfs:
                return Scheduler::FrFcfs;
            case DRAMSys::Config::SchedulerType::FrFcfsGrp:
                return Scheduler::FrFcfsGrp;
            case DRAMSys::Config::SchedulerType::GrpFrFcfs:
                return Scheduler::GrpFrFcfs;
            case DRAMSys::Config::SchedulerType::GrpFrFcfsWm:
                return Scheduler::GrpFrFcfsWm;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid Scheduler");
                return Scheduler::Fifo; // Silence Warning
            }
        }();

    if (const auto& _schedulerBuffer = mcConfig.SchedulerBuffer)
        schedulerBuffer = [=]
        {
            switch (*_schedulerBuffer)
            {
            case DRAMSys::Config::SchedulerBufferType::Bankwise:
                return SchedulerBuffer::Bankwise;
            case DRAMSys::Config::SchedulerBufferType::ReadWrite:
                return SchedulerBuffer::ReadWrite;
            case DRAMSys::Config::SchedulerBufferType::Shared:
                return SchedulerBuffer::Shared;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid SchedulerBuffer");
                return SchedulerBuffer::Bankwise; // Silence Warning
            }
        }();

    if (const auto& _cmdMux = mcConfig.CmdMux)
        cmdMux = [=]
        {
            switch (*_cmdMux)
            {
            case DRAMSys::Config::CmdMuxType::Oldest:
                return CmdMux::Oldest;
            case DRAMSys::Config::CmdMuxType::Strict:
                return CmdMux::Strict;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid CmdMux");
                return CmdMux::Oldest; // Silence Warning
            }
        }();

    if (const auto& _respQueue = mcConfig.RespQueue)
        respQueue = [=]
        {
            switch (*_respQueue)
            {
            case DRAMSys::Config::RespQueueType::Fifo:
                return RespQueue::Fifo;
            case DRAMSys::Config::RespQueueType::Reorder:
                return RespQueue::Reorder;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid RespQueue");
                return RespQueue::Fifo; // Silence Warning
            }
        }();

    if (const auto& _refreshPolicy = mcConfig.RefreshPolicy)
        refreshPolicy = [=]
        {
            switch (*_refreshPolicy)
            {
            case DRAMSys::Config::RefreshPolicyType::NoRefresh:
                return RefreshPolicy::NoRefresh;
            case DRAMSys::Config::RefreshPolicyType::AllBank:
                return RefreshPolicy::AllBank;
            case DRAMSys::Config::RefreshPolicyType::PerBank:
                return RefreshPolicy::PerBank;
            case DRAMSys::Config::RefreshPolicyType::Per2Bank:
                return RefreshPolicy::Per2Bank;
            case DRAMSys::Config::RefreshPolicyType::SameBank:
                return RefreshPolicy::SameBank;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid RefreshPolicy");
                return RefreshPolicy::NoRefresh; // Silence Warning
            }
        }();

    if (const auto& _powerDownPolicy = mcConfig.PowerDownPolicy)
        powerDownPolicy = [=]
        {
            switch (*_powerDownPolicy)
            {
            case DRAMSys::Config::PowerDownPolicyType::NoPowerDown:
                return PowerDownPolicy::NoPowerDown;
            case DRAMSys::Config::PowerDownPolicyType::Staggered:
                return PowerDownPolicy::Staggered;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid PowerDownPolicy");
                return PowerDownPolicy::NoPowerDown; // Silence Warning
            }
        }();

    if (const auto& _arbiter = mcConfig.Arbiter)
        arbiter = [=]
        {
            switch (*_arbiter)
            {
            case DRAMSys::Config::ArbiterType::Simple:
                return Arbiter::Simple;
            case DRAMSys::Config::ArbiterType::Fifo:
                return Arbiter::Fifo;
            case DRAMSys::Config::ArbiterType::Reorder:
                return Arbiter::Reorder;
            default:
                SC_REPORT_FATAL("Configuration", "Invalid Arbiter");
                return Arbiter::Simple; // Silence Warning
            }
        }();

    refreshMaxPostponed = mcConfig.RefreshMaxPostponed.value_or(refreshMaxPostponed);
    refreshMaxPulledin = mcConfig.RefreshMaxPulledin.value_or(refreshMaxPulledin);
    highWatermark = mcConfig.HighWatermark.value_or(highWatermark);
    lowWatermark = mcConfig.LowWatermark.value_or(lowWatermark);
    maxActiveTransactions = mcConfig.MaxActiveTransactions.value_or(maxActiveTransactions);
    refreshManagement = mcConfig.RefreshManagement.value_or(refreshManagement);

    requestBufferSize = mcConfig.RequestBufferSize.value_or(requestBufferSize);
    if (requestBufferSize == 0)
        SC_REPORT_FATAL("Configuration", "Minimum request buffer size is 1!");

    if (const auto& _arbitrationDelayFw = mcConfig.ArbitrationDelayFw)
    {
        arbitrationDelayFw =
            std::round(sc_time(*_arbitrationDelayFw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _arbitrationDelayBw = mcConfig.ArbitrationDelayBw)
    {
        arbitrationDelayBw =
            std::round(sc_time(*_arbitrationDelayBw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _thinkDelayFw = mcConfig.ThinkDelayFw)
    {
        thinkDelayFw = std::round(sc_time(*_thinkDelayFw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _thinkDelayBw = mcConfig.ThinkDelayBw)
    {
        thinkDelayBw = std::round(sc_time(*_thinkDelayBw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _phyDelayFw = mcConfig.PhyDelayFw)
    {
        phyDelayFw = std::round(sc_time(*_phyDelayFw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _phyDelayBw = mcConfig.PhyDelayBw)
    {
        phyDelayBw = std::round(sc_time(*_phyDelayBw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    {
        auto _blockingReadDelay = mcConfig.BlockingReadDelay.value_or(60);
        blockingReadDelay =
            std::round(sc_time(_blockingReadDelay, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    {
        auto _blockingWriteDelay = mcConfig.BlockingWriteDelay.value_or(60);
        blockingWriteDelay =
            std::round(sc_time(_blockingWriteDelay, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }
}

void Configuration::loadMemSpec(const DRAMSys::Config::MemSpec& memSpecConfig)
{
    std::string memoryType = memSpecConfig.memoryType;

    if (memoryType == "DDR3")
        memSpec = std::make_unique<const MemSpecDDR3>(memSpecConfig);
    else if (memoryType == "DDR4")
        memSpec = std::make_unique<const MemSpecDDR4>(memSpecConfig);
    else if (memoryType == "LPDDR4")
        memSpec = std::make_unique<const MemSpecLPDDR4>(memSpecConfig);
    else if (memoryType == "WIDEIO_SDR")
        memSpec = std::make_unique<const MemSpecWideIO>(memSpecConfig);
    else if (memoryType == "WIDEIO2")
        memSpec = std::make_unique<const MemSpecWideIO2>(memSpecConfig);
    else if (memoryType == "HBM2")
        memSpec = std::make_unique<const MemSpecHBM2>(memSpecConfig);
    else if (memoryType == "GDDR5")
        memSpec = std::make_unique<const MemSpecGDDR5>(memSpecConfig);
    else if (memoryType == "GDDR5X")
        memSpec = std::make_unique<const MemSpecGDDR5X>(memSpecConfig);
    else if (memoryType == "GDDR6")
        memSpec = std::make_unique<const MemSpecGDDR6>(memSpecConfig);
    else if (memoryType == "STT-MRAM")
        memSpec = std::make_unique<const MemSpecSTTMRAM>(memSpecConfig);
#ifdef DDR5_SIM
    else if (memoryType == "DDR5")
        memSpec = std::make_unique<const MemSpecDDR5>(memSpecConfig);
#endif
#ifdef LPDDR5_SIM
    else if (memoryType == "LPDDR5")
        memSpec = std::make_unique<const MemSpecLPDDR5>(memSpecConfig);
#endif
#ifdef HBM3_SIM
    else if (memoryType == "HBM3")
        memSpec = std::make_unique<const MemSpecHBM3>(memSpecConfig);
#endif
    else
        SC_REPORT_FATAL("Configuration", "Unsupported DRAM type");
}

} // namespace DRAMSys
