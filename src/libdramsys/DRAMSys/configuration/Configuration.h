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
 *    Eder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 *    Luiza Correa
 *    Derek Christ
 *    Thomas Psota
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "DRAMSys/config/DRAMSysConfiguration.h"
#include "DRAMSys/configuration/memspec/MemSpec.h"

#include <string>
#include <systemc>

namespace DRAMSys
{

class Configuration
{
public:
    // MCConfig:
    enum class PagePolicy
    {
        Open,
        Closed,
        OpenAdaptive,
        ClosedAdaptive
    } pagePolicy = PagePolicy::Open;
    enum class Scheduler
    {
        Fifo,
        FrFcfs,
        FrFcfsGrp,
        GrpFrFcfs,
        GrpFrFcfsWm
    } scheduler = Scheduler::FrFcfs;
    enum class SchedulerBuffer
    {
        Bankwise,
        ReadWrite,
        Shared
    } schedulerBuffer = SchedulerBuffer::Bankwise;
    unsigned int lowWatermark = 0;
    unsigned int highWatermark = 0;
    enum class CmdMux
    {
        Oldest,
        Strict
    } cmdMux = CmdMux::Oldest;
    enum class RespQueue
    {
        Fifo,
        Reorder
    } respQueue = RespQueue::Fifo;
    enum class Arbiter
    {
        Simple,
        Fifo,
        Reorder
    } arbiter = Arbiter::Simple;
    unsigned int requestBufferSize = 8;
    enum class RefreshPolicy
    {
        NoRefresh,
        PerBank,
        Per2Bank,
        SameBank,
        AllBank
    } refreshPolicy = RefreshPolicy::AllBank;
    unsigned int refreshMaxPostponed = 0;
    unsigned int refreshMaxPulledin = 0;
    enum class PowerDownPolicy
    {
        NoPowerDown,
        Staggered
    } powerDownPolicy = PowerDownPolicy::NoPowerDown;
    unsigned int maxActiveTransactions = 64;
    bool refreshManagement = false;
    sc_core::sc_time arbitrationDelayFw = sc_core::SC_ZERO_TIME;
    sc_core::sc_time arbitrationDelayBw = sc_core::SC_ZERO_TIME;
    sc_core::sc_time thinkDelayFw = sc_core::SC_ZERO_TIME;
    sc_core::sc_time thinkDelayBw = sc_core::SC_ZERO_TIME;
    sc_core::sc_time phyDelayFw = sc_core::SC_ZERO_TIME;
    sc_core::sc_time phyDelayBw = sc_core::SC_ZERO_TIME;
    sc_core::sc_time blockingReadDelay = sc_core::SC_ZERO_TIME;
    sc_core::sc_time blockingWriteDelay = sc_core::SC_ZERO_TIME;

    // SimConfig
    std::string simulationName = "default";
    bool databaseRecording = false;
    bool powerAnalysis = false;
    bool enableWindowing = false;
    unsigned int windowSize = 1000;
    bool debug = false;
    bool simulationProgressBar = false;
    bool checkTLM2Protocol = false;
    bool useMalloc = false;
    unsigned long long int addressOffset = 0;

    enum class StoreMode
    {
        NoStorage,
        Store
    } storeMode = StoreMode::NoStorage;

    // MemSpec (from DRAM-Power)
    std::unique_ptr<const MemSpec> memSpec;

    void loadMCConfig(const DRAMSys::Config::McConfig& mcConfig);
    void loadSimConfig(const DRAMSys::Config::SimConfig& simConfig);
    void loadMemSpec(const DRAMSys::Config::MemSpec& memSpec);
};

} // namespace DRAMSys

#endif // CONFIGURATION_H
