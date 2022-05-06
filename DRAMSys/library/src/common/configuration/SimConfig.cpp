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

#include "SimConfig.h"

namespace DRAMSysConfiguration
{

void to_json(json &j, const SimConfig &c)
{
    j = json{{"AddressOffset", c.addressOffset},
             {"CheckTLM2Protocol", c.checkTLM2Protocol},
             {"DatabaseRecording", c.databaseRecording},
             {"Debug", c.debug},
             {"EnableWindowing", c.enableWindowing},
             {"ErrorCSVFile", c.errorCsvFile},
             {"ErrorChipSeed", c.errorChipSeed},
             {"PowerAnalysis", c.powerAnalysis},
             {"SimulationName", c.simulationName},
             {"SimulationProgressBar", c.simulationProgressBar},
             {"StoreMode", c.storeMode},
             {"ThermalSimulation", c.thermalSimulation},
             {"UseMalloc", c.useMalloc},
             {"WindowSize", c.windowSize}};
}

void from_json(const json &j, SimConfig &c)
{
    json j_simconfig = get_config_json(j, simConfigPath, "simconfig");

    if (j_simconfig.contains("AddressOffset"))
        j_simconfig.at("AddressOffset").get_to(c.addressOffset);

    if (j_simconfig.contains("CheckTLM2Protocol"))
        j_simconfig.at("CheckTLM2Protocol").get_to(c.checkTLM2Protocol);

    if (j_simconfig.contains("DatabaseRecording"))
        j_simconfig.at("DatabaseRecording").get_to(c.databaseRecording);

    if (j_simconfig.contains("Debug"))
        j_simconfig.at("Debug").get_to(c.debug);

    if (j_simconfig.contains("EnableWindowing"))
        j_simconfig.at("EnableWindowing").get_to(c.enableWindowing);

    if (j_simconfig.contains("ErrorCSVFile"))
        j_simconfig.at("ErrorCSVFile").get_to(c.errorCsvFile);

    if (j_simconfig.contains("ErrorChipSeed"))
        j_simconfig.at("ErrorChipSeed").get_to(c.errorChipSeed);

    if (j_simconfig.contains("PowerAnalysis"))
        j_simconfig.at("PowerAnalysis").get_to(c.powerAnalysis);

    if (j_simconfig.contains("SimulationName"))
        j_simconfig.at("SimulationName").get_to(c.simulationName);

    if (j_simconfig.contains("SimulationProgressBar"))
        j_simconfig.at("SimulationProgressBar").get_to(c.simulationProgressBar);

    if (j_simconfig.contains("StoreMode"))
        j_simconfig.at("StoreMode").get_to(c.storeMode);

    if (j_simconfig.contains("ThermalSimulation"))
        j_simconfig.at("ThermalSimulation").get_to(c.thermalSimulation);

    if (j_simconfig.contains("UseMalloc"))
        j_simconfig.at("UseMalloc").get_to(c.useMalloc);

    if (j_simconfig.contains("WindowSize"))
        j_simconfig.at("WindowSize").get_to(c.windowSize);

    invalidateEnum(c.storeMode);
}

void from_dump(const std::string &dump, SimConfig &c)
{
    json json_simconfig = json::parse(dump).at("simconfig");
    json_simconfig.get_to(c);
}

std::string dump(const SimConfig &c, unsigned int indentation)
{
    json json_simconfig;
    json_simconfig["simconfig"] = c;
    return json_simconfig.dump(indentation);
}

} // namespace DRAMSysConfiguration
