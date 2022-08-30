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

#include <DRAMSysConfiguration.h>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

DRAMSysConfiguration::AddressMapping getAddressMapping()
{
    return DRAMSysConfiguration::AddressMapping{{{0, 1}},
                                                {{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}},
                                                {{16}},
                                                {{13, 14, 15}},
                                                {{17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}},
                                                {{33}},
                                                {{}},
                                                {{}}};
}

DRAMSysConfiguration::McConfig getMcConfig()
{
    return DRAMSysConfiguration::McConfig{DRAMSysConfiguration::PagePolicy::Open,
                                          DRAMSysConfiguration::Scheduler::FrFcfs,
                                          DRAMSysConfiguration::SchedulerBuffer::Bankwise,
                                          8,
                                          DRAMSysConfiguration::CmdMux::Oldest,
                                          DRAMSysConfiguration::RespQueue::Fifo,
                                          DRAMSysConfiguration::RefreshPolicy::AllBank,
                                          0,
                                          0,
                                          DRAMSysConfiguration::PowerDownPolicy::NoPowerDown,
                                          DRAMSysConfiguration::Arbiter::Simple,
                                          128,
                                          {}};
}

DRAMSysConfiguration::SimConfig getSimConfig()
{
    return DRAMSysConfiguration::SimConfig{
        0,   false, true,     false, false, {"error.csv"},
        42,  false, {"ddr5"}, true,  DRAMSysConfiguration::StoreMode::NoStorage,        false, false,
        1000};
}

DRAMSysConfiguration::ThermalConfig getThermalConfig()
{
    std::vector<DRAMSysConfiguration::DramDieChannel> channels {{"dram_die_channel0", 0.0, 1.0}, {"dram_die_channel1", 0.0, 1.0}, {"dram_die_channel2", 0.0, 1.0}, {"dram_die_channel3", 0.0, 1.0}};

    return DRAMSysConfiguration::ThermalConfig{
        DRAMSysConfiguration::TemperatureScale::Celsius,
        89,
        100,
        DRAMSysConfiguration::ThermalSimUnit::Microseconds,
        DRAMSysConfiguration::PowerInfo{channels},
        "127.0.0.1",
        118800,
        10,
        5,
        true,
        true};
}

DRAMSysConfiguration::TracePlayer getTracePlayer()
{
    DRAMSysConfiguration::TracePlayer player;
    player.clkMhz = 100;
    player.name = "mytrace.stl";

    return player;
}

DRAMSysConfiguration::TraceGenerator getTraceGeneratorOneState()
{
    DRAMSysConfiguration::TraceGenerator gen;
    gen.clkMhz = 100;
    gen.name = "MyTestGen";

    DRAMSysConfiguration::TraceGeneratorTrafficState state0;
    state0.numRequests = 1000;
    state0.rwRatio = 0.5;
    state0.addressDistribution = DRAMSysConfiguration::AddressDistribution::Random;
    state0.addressIncrement = {};
    state0.minAddress = {};
    state0.maxAddress = {};
    state0.clksPerRequest = {};

    gen.states.emplace(0, state0);

    return gen;
}

DRAMSysConfiguration::TraceGenerator getTraceGeneratorMultipleStates()
{
    DRAMSysConfiguration::TraceGenerator gen;

    gen.clkMhz = 100;
    gen.name = "MyTestGen";
    gen.maxPendingReadRequests = 8;

    DRAMSysConfiguration::TraceGeneratorTrafficState state0;
    state0.numRequests = 1000;
    state0.rwRatio = 0.5;
    state0.addressDistribution = DRAMSysConfiguration::AddressDistribution::Sequential;
    state0.addressIncrement = 256;
    state0.minAddress = {};
    state0.maxAddress = 1024;
    state0.clksPerRequest = {};

    DRAMSysConfiguration::TraceGeneratorTrafficState state1;
    state1.numRequests = 100;
    state1.rwRatio = 0.75;
    state1.addressDistribution = DRAMSysConfiguration::AddressDistribution::Sequential;
    state1.addressIncrement = 512;
    state1.minAddress = 1024;
    state1.maxAddress = 2048;
    state1.clksPerRequest = {};

    gen.states.emplace(0, state0);
    gen.states.emplace(1, state1);

    DRAMSysConfiguration::TraceGeneratorStateTransition transistion0{1, 1.0};

    gen.transitions.emplace(0, transistion0);

    return gen;
}

DRAMSysConfiguration::TraceHammer getTraceHammer()
{
    DRAMSysConfiguration::TraceHammer hammer;

    hammer.clkMhz = 100;
    hammer.name = "MyTestHammer";
    hammer.numRequests = 4000;
    hammer.rowIncrement = 2097152;

    return hammer;
}

DRAMSysConfiguration::TraceSetup getTraceSetup()
{
    using namespace DRAMSysConfiguration;

    std::vector<std::variant<TracePlayer, TraceGenerator, TraceHammer>> initiators;
    initiators.emplace_back(getTracePlayer());
    initiators.emplace_back(getTraceGeneratorOneState());
    initiators.emplace_back(getTraceGeneratorMultipleStates());
    initiators.emplace_back(getTraceHammer());

    return DRAMSysConfiguration::TraceSetup{initiators};
}

DRAMSysConfiguration::Configuration getConfig(const DRAMSysConfiguration::MemSpec &memSpec)
{
    return DRAMSysConfiguration::Configuration{
        getAddressMapping(),
        getMcConfig(),
        memSpec,
        getSimConfig(),
        "std::string_simulationId",
        getThermalConfig(),
        // {{}, false}, works too
        getTraceSetup(),
    };
}

int main()
{
    DRAMSysConfiguration::Configuration conf = DRAMSysConfiguration::from_path("ddr5.json");
    std::ofstream fileout("myjson.json");
    json j_my;
    j_my["simulation"] = getConfig(conf.memSpec); // just copy memspec over
    fileout << j_my.dump(4);

    std::ifstream file2("hbm2.json");
    json hbm2_j = json::parse(file2, nullptr, false);
    json hbm2_config = hbm2_j.at("simulation");
    DRAMSysConfiguration::Configuration hbm2conf = hbm2_config.get<DRAMSysConfiguration::Configuration>();
    std::ofstream filehbm2("myhbm2.json");
    json j_myhbm2;
    j_myhbm2["simulation"] = hbm2conf;
    filehbm2 << j_myhbm2.dump(4);

    std::ifstream file3("myjson.json");
    json ddr5_old = json::parse(file3, nullptr, false);
    json ddr5_old_conf = ddr5_old.at("simulation");
    DRAMSysConfiguration::Configuration ddr5_old_config = ddr5_old_conf.get<DRAMSysConfiguration::Configuration>();
    std::ofstream fileoldout("myjson2.json");
    json j_oldconfconv;
    j_oldconfconv["simulation"] = ddr5_old_config;
    fileoldout << j_oldconfconv.dump(4);
}
