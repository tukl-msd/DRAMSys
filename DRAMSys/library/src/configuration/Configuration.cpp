/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
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
#include "memspec/MemSpecDDR3.h"
#include "memspec/MemSpecDDR4.h"
#include "memspec/MemSpecDDR5.h"
#include "memspec/MemSpecWideIO.h"
#include "memspec/MemSpecLPDDR4.h"
#include "memspec/MemSpecLPDDR5.h"
#include "memspec/MemSpecWideIO2.h"
#include "memspec/MemSpecHBM2.h"
#include "memspec/MemSpecGDDR5.h"
#include "memspec/MemSpecGDDR5X.h"
#include "memspec/MemSpecGDDR6.h"
#include "memspec/MemSpecSTTMRAM.h"

using namespace sc_core;

enum sc_time_unit string2TimeUnit(const std::string &s)
{
    if (s == "s")
        return SC_SEC;
    else if (s == "ms")
        return SC_MS;
    else if (s == "us")
        return SC_US;
    else if (s == "ns")
        return SC_NS;
    else if (s == "ps")
        return SC_PS;
    else if (s == "fs")
        return SC_FS;
    else {
        SC_REPORT_FATAL("Configuration",
                        ("Could not convert to enum sc_time_unit: " + s).c_str());
        throw;
    }
}

void Configuration::loadSimConfig(const DRAMSysConfiguration::SimConfig &simConfig)
{
    if (const auto& _addressOffset = simConfig.addressOffset)
        addressOffset = *_addressOffset;

    if (const auto& _checkTLM2Protocol = simConfig.checkTLM2Protocol)
        checkTLM2Protocol = *_checkTLM2Protocol;

    if (const auto& _databaseRecording = simConfig.databaseRecording)
        databaseRecording = *_databaseRecording;

    if (const auto& _debug = simConfig.debug)
        debug = *_debug;

    if (const auto& _enableWindowing = simConfig.enableWindowing)
        enableWindowing = *_enableWindowing;

    if (const auto& _powerAnalysis = simConfig.powerAnalysis)
        powerAnalysis = *_powerAnalysis;

    if (const auto& _simulationName = simConfig.simulationName)
        simulationName = *_simulationName;

    if (const auto& _simulationProgressBar = simConfig.simulationProgressBar)
        simulationProgressBar = *_simulationProgressBar;

    if (const auto& _thermalSimulation = simConfig.thermalSimulation)
        thermalSimulation = *_thermalSimulation;

    if (const auto& _useMalloc = simConfig.useMalloc)
        useMalloc = *_useMalloc;

    if (const auto& _windowSize = simConfig.windowSize)
        windowSize = *_windowSize;

    if (windowSize == 0)
            SC_REPORT_FATAL("Configuration", "Minimum window size is 1");

    if (const auto& _errorCsvFile = simConfig.errorCsvFile)
        errorCSVFile = *_errorCsvFile;

    if (const auto& _errorChipSeed = simConfig.errorChipSeed)
        errorChipSeed = *_errorChipSeed;

    if (const auto& _storeMode = simConfig.storeMode)
        storeMode = [=] {
            if (_storeMode == DRAMSysConfiguration::StoreMode::NoStorage)
                return StoreMode::NoStorage;
            else if (_storeMode == DRAMSysConfiguration::StoreMode::Store)
                return StoreMode::Store;
            else
                return StoreMode::ErrorModel;
        }();
}

void Configuration::loadTemperatureSimConfig(const DRAMSysConfiguration::ThermalConfig &thermalConfig)
{
    temperatureSim.temperatureScale = [=] {
        if (thermalConfig.temperatureScale == DRAMSysConfiguration::TemperatureScale::Celsius)
            return TemperatureSimConfig::TemperatureScale::Celsius;
        else if (thermalConfig.temperatureScale == DRAMSysConfiguration::TemperatureScale::Fahrenheit)
            return TemperatureSimConfig::TemperatureScale::Fahrenheit;
        else
            return TemperatureSimConfig::TemperatureScale::Kelvin;
    }();

    temperatureSim.staticTemperatureDefaultValue = thermalConfig.staticTemperatureDefaultValue;
    temperatureSim.thermalSimPeriod = thermalConfig.thermalSimPeriod;

    temperatureSim.thermalSimUnit = [=] {
        if (thermalConfig.thermalSimUnit == DRAMSysConfiguration::ThermalSimUnit::Seconds)
            return sc_core::SC_SEC;
        else if (thermalConfig.thermalSimUnit == DRAMSysConfiguration::ThermalSimUnit::Milliseconds)
            return sc_core::SC_MS;
        else if (thermalConfig.thermalSimUnit == DRAMSysConfiguration::ThermalSimUnit::Microseconds)
            return sc_core::SC_US;
        else if (thermalConfig.thermalSimUnit == DRAMSysConfiguration::ThermalSimUnit::Nanoseconds)
            return sc_core::SC_NS;
        else if (thermalConfig.thermalSimUnit == DRAMSysConfiguration::ThermalSimUnit::Picoseconds)
            return sc_core::SC_PS;
        else
            return sc_core::SC_FS;
    }();

    for (const auto &channel : thermalConfig.powerInfo.channels)
    {
        temperatureSim.powerInitialValues.push_back(channel.init_pow);
        temperatureSim.powerThresholds.push_back(channel.threshold);
    }

    temperatureSim.iceServerIp = thermalConfig.iceServerIp;
    temperatureSim.iceServerPort = thermalConfig.iceServerPort;
    temperatureSim.simPeriodAdjustFactor = thermalConfig.simPeriodAdjustFactor;
    temperatureSim.nPowStableCyclesToIncreasePeriod = thermalConfig.nPowStableCyclesToIncreasePeriod;
    temperatureSim.generateTemperatureMap = thermalConfig.generateTemperatureMap;
    temperatureSim.generatePowerMap = thermalConfig.generatePowerMap;

    temperatureSim.showTemperatureSimConfig();
}

void Configuration::loadMCConfig(const DRAMSysConfiguration::McConfig &mcConfig)
{
    if (const auto& _pagePolicy = mcConfig.pagePolicy)
        pagePolicy = [=] {
            if (_pagePolicy == DRAMSysConfiguration::PagePolicy::Open)
                return PagePolicy::Open;
            else if (_pagePolicy == DRAMSysConfiguration::PagePolicy::OpenAdaptive)
                return PagePolicy::OpenAdaptive;
            else if (_pagePolicy == DRAMSysConfiguration::PagePolicy::Closed)
                return PagePolicy::Closed;
            else
                return PagePolicy::ClosedAdaptive;
        }();

    if (const auto& _scheduler = mcConfig.scheduler)
        scheduler = [=] {
           if (_scheduler == DRAMSysConfiguration::Scheduler::Fifo)
               return Scheduler::Fifo;
           else if (_scheduler == DRAMSysConfiguration::Scheduler::FrFcfs)
               return Scheduler::FrFcfs;
           else if (_scheduler == DRAMSysConfiguration::Scheduler::FrFcfsGrp)
               return Scheduler::FrFcfsGrp;
           else if (_scheduler == DRAMSysConfiguration::Scheduler::GrpFrFcfs)
               return Scheduler::GrpFrFcfs;
           else
               return Scheduler::GrpFrFcfsWm;
        }();

    if (const auto& _highWatermark = mcConfig.highWatermark)
        highWatermark = *mcConfig.highWatermark;

    if (const auto& _lowWatermark = mcConfig.lowWatermark)
        lowWatermark = *mcConfig.lowWatermark;

    if (const auto& _schedulerBuffer = mcConfig.schedulerBuffer)
        schedulerBuffer = [=] {
            if (_schedulerBuffer == DRAMSysConfiguration::SchedulerBuffer::Bankwise)
                return SchedulerBuffer::Bankwise;
            else if (_schedulerBuffer == DRAMSysConfiguration::SchedulerBuffer::ReadWrite)
                return SchedulerBuffer::ReadWrite;
            else
                return SchedulerBuffer::Shared;
        }();

    if (const auto& _requestBufferSize = mcConfig.requestBufferSize)
        requestBufferSize = *mcConfig.requestBufferSize;

    if (requestBufferSize == 0)
        SC_REPORT_FATAL("Configuration", "Minimum request buffer size is 1!");

    if (const auto& _cmdMux = mcConfig.cmdMux)
        cmdMux = [=] {
            if (_cmdMux == DRAMSysConfiguration::CmdMux::Oldest)
                return CmdMux::Oldest;
            else
                return CmdMux::Strict;
        }();

    if (const auto& _respQueue = mcConfig.respQueue)
        respQueue = [=] {
            if (_respQueue == DRAMSysConfiguration::RespQueue::Fifo)
                return RespQueue::Fifo;
            else
                return RespQueue::Reorder;
        }();

    if (const auto& _refreshPolicy = mcConfig.refreshPolicy)
        refreshPolicy = [=] {
            if (_refreshPolicy == DRAMSysConfiguration::RefreshPolicy::NoRefresh)
                return RefreshPolicy::NoRefresh;
            else if (_refreshPolicy == DRAMSysConfiguration::RefreshPolicy::AllBank)
                return RefreshPolicy::AllBank;
            else if (_refreshPolicy == DRAMSysConfiguration::RefreshPolicy::PerBank)
                return RefreshPolicy::PerBank;
            else if (_refreshPolicy == DRAMSysConfiguration::RefreshPolicy::Per2Bank)
                return RefreshPolicy::Per2Bank;
            else // if (policy == DRAMSysConfiguration::RefreshPolicy::SameBank)
                return RefreshPolicy::SameBank;
        }();

    if (const auto& _refreshMaxPostponed = mcConfig.refreshMaxPostponed)
        refreshMaxPostponed = *_refreshMaxPostponed;

    if (const auto& _refreshMaxPulledin = mcConfig.refreshMaxPulledin)
        refreshMaxPulledin = *_refreshMaxPulledin;

    if (const auto& _powerDownPolicy = mcConfig.powerDownPolicy)
        powerDownPolicy = [=] {
            if (_powerDownPolicy == DRAMSysConfiguration::PowerDownPolicy::NoPowerDown)
                return PowerDownPolicy::NoPowerDown;
            else
                return PowerDownPolicy::Staggered;
        }();

    if (const auto& _arbiter = mcConfig.arbiter)
        arbiter = [=] {
            if (_arbiter == DRAMSysConfiguration::Arbiter::Simple)
                return Arbiter::Simple;
            else if (_arbiter == DRAMSysConfiguration::Arbiter::Fifo)
                return Arbiter::Fifo;
            else
                return Arbiter::Reorder;
        }();

    if (const auto& _maxActiveTransactions = mcConfig.maxActiveTransactions)
         maxActiveTransactions = *_maxActiveTransactions;

    if (const auto& _refreshManagement = mcConfig.refreshManagement)
         refreshManagement = *_refreshManagement;

    if (const auto& _arbitrationDelayFw = mcConfig.arbitrationDelayFw)
    {
         arbitrationDelayFw = std::round(sc_time(*_arbitrationDelayFw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _arbitrationDelayBw = mcConfig.arbitrationDelayBw)
    {
         arbitrationDelayBw = std::round(sc_time(*_arbitrationDelayBw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _thinkDelayFw = mcConfig.thinkDelayFw)
    {
         thinkDelayFw = std::round(sc_time(*_thinkDelayFw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _thinkDelayBw = mcConfig.thinkDelayBw)
    {
         thinkDelayBw = std::round(sc_time(*_thinkDelayBw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _phyDelayFw = mcConfig.phyDelayFw)
    {
         phyDelayFw = std::round(sc_time(*_phyDelayFw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }

    if (const auto& _phyDelayBw = mcConfig.phyDelayBw)
    {
         phyDelayBw = std::round(sc_time(*_phyDelayBw, SC_NS) / memSpec->tCK) * memSpec->tCK;
    }
}

void Configuration::loadMemSpec(const DRAMSysConfiguration::MemSpec &memSpecConfig)
{
    std::string memoryType = memSpecConfig.memoryType;

    if (memoryType == "DDR3")
        memSpec = std::make_unique<const MemSpecDDR3>(memSpecConfig);
    else if (memoryType == "DDR4")
        memSpec = std::make_unique<const MemSpecDDR4>(memSpecConfig);
    else if (memoryType == "DDR5")
        memSpec = std::make_unique<const MemSpecDDR5>(memSpecConfig);
    else if (memoryType == "LPDDR4")
        memSpec = std::make_unique<const MemSpecLPDDR4>(memSpecConfig);
    else if (memoryType == "LPDDR5")
        memSpec = std::make_unique<const MemSpecLPDDR5>(memSpecConfig);
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
    else
        SC_REPORT_FATAL("Configuration", "Unsupported DRAM type");
}
