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
 *    Eder F. Zulian
 *    Matthias Jung
 *    Luiza Correa
 *    Derek Christ
 */

#ifndef TEMPERATURESIMCONFIG_H
#define TEMPERATURESIMCONFIG_H

#include <string>
#include <vector>
#include <DRAMSysConfiguration.h>
#include <systemc>
#include <utility>
#include "../common/DebugManager.h"

struct TemperatureSimConfig
{
    // Temperature Scale
    enum class TemperatureScale {Celsius, Fahrenheit, Kelvin} temperatureScale;

    // Static Temperature Simulation parameters
    int staticTemperatureDefaultValue;

    // Thermal Simulation parameters
    double thermalSimPeriod;
    enum sc_core::sc_time_unit thermalSimUnit;
    std::string iceServerIp;
    unsigned int iceServerPort;
    unsigned int simPeriodAdjustFactor;
    unsigned int nPowStableCyclesToIncreasePeriod;
    bool generateTemperatureMap;
    bool generatePowerMap;

    // Power related information
    std::vector<float> powerInitialValues;
    std::vector<float> powerThresholds;

    void showTemperatureSimConfig()
    {
        NDEBUG_UNUSED(int i) = 0;
        for (NDEBUG_UNUSED(auto e) : powerInitialValues)
        {
            PRINTDEBUGMESSAGE("TemperatureSimConfig", "powerInitialValues["
                              + std::to_string(i++) + "]: " + std::to_string(e));
        }
        i = 0;
        for (NDEBUG_UNUSED(auto e) : powerThresholds)
        {
            PRINTDEBUGMESSAGE("TemperatureSimConfig", "powerThreshold["
                              + std::to_string(i++) + "]: " + std::to_string(e));
        }
    }
};

#endif // TEMPERATURESIMCONFIG_H

