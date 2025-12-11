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

#include <simulator/Simulator.h>

#include <benchmark/benchmark.h>
#include <filesystem>
#include <tuple>

namespace Simulation
{

template<class ...Args>
static void example_simulation(benchmark::State& state, Args&&... args)
{
    auto args_tuple = std::make_tuple(std::move(args)...);
    auto *rdbuf = std::cout.rdbuf(nullptr);

    for (auto _ : state)
    {
        sc_core::sc_get_curr_simcontext()->reset();

        std::filesystem::path configFile = std::get<0>(args_tuple);

        DRAMSys::Config::Configuration configuration =
            DRAMSys::Config::from_path(configFile.c_str());

        Simulator simulator(std::move(configuration), configFile);
        simulator.run();
    }

    std::cout.rdbuf(rdbuf);
}

BENCHMARK_CAPTURE(example_simulation, ddr3, std::string("configs/ddr3-example.json"));
BENCHMARK_CAPTURE(example_simulation, ddr4, std::string("configs/ddr4-example.json"));
BENCHMARK_CAPTURE(example_simulation, ddr5, std::string("configs/ddr5-example.json"));
BENCHMARK_CAPTURE(example_simulation, lpddr4, std::string("configs/lpddr4-example.json"));
BENCHMARK_CAPTURE(example_simulation, lpddr5, std::string("configs/lpddr5-example.json"));
BENCHMARK_CAPTURE(example_simulation, hbm2, std::string("configs/hbm2-example.json"));
BENCHMARK_CAPTURE(example_simulation, hbm3, std::string("configs/hbm3-example.json"));

} // namespace Simulation
