/*
 * Copyright (c) 2023, RPTU Kaiserslautern-Landau
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

#pragma once

#include <DRAMSys/DRAMSys.h>
#include <DRAMSys/common/MemoryManager.h>
#include <DRAMSys/configuration/json/DRAMSysConfiguration.h>
#include <DRAMSys/initiators/request/RequestIssuer.h>
#include <DRAMSys/statistics/Group.h>
#include <DRAMSys/statistics/Stat.h>
#include <DRAMSys/statistics/StatProvider.h>

class Simulator : public sc_core::sc_module, public DRAMSys::Statistics::StatProvider
{
public:
    Simulator(sc_core::sc_module_name const& name, DRAMSys::Config::Configuration configuration, std::filesystem::path baseConfig);
 
     void run();
 
    void updateStats() override;
    void resetStats() override;
    DRAMSys::Statistics::Group const& getStatGroup() const override { return stats; }

private:
    std::unique_ptr<DRAMSys::Initiators::RequestIssuer>
    instantiateInitiator(const DRAMSys::Config::Initiator& initiator);

    bool storageEnabled;
    DRAMSys::MemoryManager memoryManager;

    DRAMSys::Config::Configuration configuration;

    std::unique_ptr<DRAMSys::DRAMSys> dramSys;
    std::vector<std::unique_ptr<DRAMSys::Initiators::RequestIssuer>> initiators;

    std::function<void()> terminateInitiator;
    std::function<void()> finishTransaction;

    unsigned int terminatedInitiators = 0;
    uint64_t totalTransactions{};
    uint64_t transactionsFinished = 0;

    std::filesystem::path baseConfig;

    std::chrono::high_resolution_clock::time_point startTime;
    sc_core::sc_time lastSimTime = sc_core::SC_ZERO_TIME;

    class Stats : public DRAMSys::Statistics::Group {
    public:
        DRAMSys::Statistics::ScalarStat& wallclockTime;
        DRAMSys::Statistics::ScalarStat& simulationTicks;
        DRAMSys::Statistics::ScalarStat& simulationTime;

        Stats(Simulator const& simulator);
    } stats;
};
