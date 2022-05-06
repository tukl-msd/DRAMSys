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

#ifndef DRAMSYSCONFIGURATION_THERMALCONFIG_H
#define DRAMSYSCONFIGURATION_THERMALCONFIG_H

#include <nlohmann/json.hpp>

namespace DRAMSysConfiguration
{
using json = nlohmann::json;

const std::string thermalConfigPath = "configs/thermalsim";

enum class TemperatureScale
{
    Celsius,
    Fahrenheit,
    Kelvin,
    Invalid = -1,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TemperatureScale, {{TemperatureScale::Invalid, nullptr},
                                                {TemperatureScale::Celsius, "Celsius"},
                                                {TemperatureScale::Fahrenheit, "Fahrenheit"},
                                                {TemperatureScale::Kelvin, "Kelvin"}})

enum class ThermalSimUnit
{
    Seconds,
    Milliseconds,
    Microseconds,
    Nanoseconds,
    Picoseconds,
    Femtoseconds,
    Invalid = -1,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ThermalSimUnit, {{ThermalSimUnit::Invalid, nullptr},
                                              {ThermalSimUnit::Seconds, "s"},
                                              {ThermalSimUnit::Milliseconds, "ms"},
                                              {ThermalSimUnit::Microseconds, "us"},
                                              {ThermalSimUnit::Nanoseconds, "ns"},
                                              {ThermalSimUnit::Picoseconds, "ps"},
                                              {ThermalSimUnit::Femtoseconds, "fs"}})

struct DramDieChannel
{
    std::string identifier;
    double init_pow;
    double threshold;
};

void to_json(json &j, const DramDieChannel &c);
void from_json(const json &j, DramDieChannel &c);

struct PowerInfo
{
    std::vector<DramDieChannel> channels;
};

void to_json(json &j, const PowerInfo &c);
void from_json(const json &j, PowerInfo &c);

struct ThermalConfig
{
    TemperatureScale temperatureScale;
    int staticTemperatureDefaultValue;
    double thermalSimPeriod;
    ThermalSimUnit thermalSimUnit;
    PowerInfo powerInfo;
    std::string iceServerIp;
    unsigned int iceServerPort;
    unsigned int simPeriodAdjustFactor;
    unsigned int nPowStableCyclesToIncreasePeriod;
    bool generateTemperatureMap;
    bool generatePowerMap;
};

void to_json(json &j, const ThermalConfig &c);
void from_json(const json &j, ThermalConfig &c);

void from_dump(const std::string &dump, ThermalConfig &c);
std::string dump(const ThermalConfig &c, unsigned int indentation = -1);

} // namespace Configuration

#endif // DRAMSYSCONFIGURATION_THERMALCONFIG_H
