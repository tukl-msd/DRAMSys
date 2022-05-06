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

#include "ThermalConfig.h"
#include "util.h"

namespace DRAMSysConfiguration
{

void to_json(json &j, const ThermalConfig &c)
{
    j = json{{"TemperatureScale", c.temperatureScale},
             {"StaticTemperatureDefaultValue", c.staticTemperatureDefaultValue},
             {"ThermalSimPeriod", c.thermalSimPeriod},
             {"ThermalSimUnit", c.thermalSimUnit},
             {"PowerInfoFile", c.powerInfo},
             {"IceServerIp", c.iceServerIp},
             {"IceServerPort", c.iceServerPort},
             {"SimPeriodAdjustFactor", c.simPeriodAdjustFactor},
             {"NPowStableCyclesToIncreasePeriod", c.nPowStableCyclesToIncreasePeriod},
             {"GenerateTemperatureMap", c.generateTemperatureMap},
             {"GeneratePowerMap", c.generatePowerMap}};
}

void from_json(const json &j, ThermalConfig &c)
{
    json j_thermalsim = get_config_json(j, thermalConfigPath, "thermalsimconfig");

    j_thermalsim.at("TemperatureScale").get_to(c.temperatureScale);
    j_thermalsim.at("StaticTemperatureDefaultValue").get_to(c.staticTemperatureDefaultValue);
    j_thermalsim.at("ThermalSimPeriod").get_to(c.thermalSimPeriod);
    j_thermalsim.at("ThermalSimUnit").get_to(c.thermalSimUnit);
    j_thermalsim.at("PowerInfoFile").get_to(c.powerInfo);
    j_thermalsim.at("IceServerIp").get_to(c.iceServerIp);
    j_thermalsim.at("IceServerPort").get_to(c.iceServerPort);
    j_thermalsim.at("SimPeriodAdjustFactor").get_to(c.simPeriodAdjustFactor);
    j_thermalsim.at("NPowStableCyclesToIncreasePeriod").get_to(c.nPowStableCyclesToIncreasePeriod);
    j_thermalsim.at("GenerateTemperatureMap").get_to(c.generateTemperatureMap);
    j_thermalsim.at("GeneratePowerMap").get_to(c.generatePowerMap);
}

void to_json(json &j, const DramDieChannel &c)
{
    j = json{{"init_pow", c.init_pow}, {"threshold", c.threshold}};
}

void from_json(const json &j, DramDieChannel &c)
{
    j.at("init_pow").get_to(c.init_pow);
    j.at("threshold").get_to(c.threshold);
}

void to_json(json &j, const PowerInfo &c)
{
    j = json{};

    for (const auto &channel : c.channels)
    {
        j.emplace(channel.identifier, channel);
    }
}

void from_json(const json &j, PowerInfo &c)
{
    json j_powerinfo = get_config_json(j, thermalConfigPath, "powerInfo");

    for (const auto &entry : j_powerinfo.items())
    {
        DramDieChannel channel;
        j_powerinfo.at(entry.key()).get_to(channel);
        channel.identifier = entry.key();

        c.channels.push_back(channel);
    }
}

void from_dump(const std::string &dump, ThermalConfig &c)
{
    json json_thermalconfig = json::parse(dump).at("thermalconfig");
    json_thermalconfig.get_to(c);
}

std::string dump(const ThermalConfig &c, unsigned int indentation)
{
    json json_thermalconfig;
    json_thermalconfig["thermalconfig"] = c;
    return json_thermalconfig.dump(indentation);
}

} // namespace DRAMSysConfiguration
