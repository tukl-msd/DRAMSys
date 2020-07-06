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
 */

#ifndef TEMPERATURESIMCONFIG_H
#define TEMPERATURESIMCONFIG_H

#include <systemc.h>
#include <iostream>
#include <string>
#include "../common/DebugManager.h"
#include "../common/utils.h"

struct TemperatureSimConfig
{
    // Temperature Scale
    std::string temperatureScale;
    std::string pathToResources;

    void setPathToResources(std::string path)
    {
        pathToResources = path;
    }

    // Static Temperature Simulation parameters
    int staticTemperatureDefaultValue;

    // Thermal Simulation parameters
    double thermalSimPeriod;
    enum sc_time_unit thermalSimUnit;
    std::string iceServerIp;
    unsigned int iceServerPort;
    unsigned int simPeriodAdjustFactor;
    unsigned int nPowStableCyclesToIncreasePeriod;
    bool generateTemperatureMap;
    bool generatePowerMap;

    // Power related information
    std::string powerInfoFile;
    std::vector<float> powerInitialValues;
    std::vector<float> powerThresholds;

    void parsePowerInfoFile()
    {
        PRINTDEBUGMESSAGE("TemperatureSimConfig", "Power Info File: " + powerInfoFile);

        powerInfoFile = pathToResources
                + "/configs/thermalsim/"
                + powerInfoFile;

        // Load the JSON file into memory and parse it
        nlohmann::json powInfoElem = parseJSON(powerInfoFile);

        if (powInfoElem["powerInfo"].empty())
        {
            // Invalid file
            std::string errormsg = "Invalid Power Info File " + powerInfoFile;
            PRINTDEBUGMESSAGE("TemperatureSimConfig", errormsg);
            SC_REPORT_FATAL("Temperature Sim Config", errormsg.c_str());
        }
        else
        {
            for (auto it : powInfoElem["powerInfo"].items())
            {
                // Load initial power values for all devices
                auto value= it.value();
                float pow = value["init_pow"];
                powerInitialValues.push_back(pow);

                // Load power thresholds for all devices
                //Changes in power dissipation that exceed the threshods
                //will make the thermal simulation to be executed more often)
                float thr = value["threshold"];
                powerThresholds.push_back(thr);
            }
        }
        showTemperatureSimConfig();
    }

    void showTemperatureSimConfig()
    {
        int i __attribute__((unused)) = 0;
        for (auto e __attribute__((unused)) : powerInitialValues)
        {
            PRINTDEBUGMESSAGE("TemperatureSimConfig", "powerInitialValues["
                              + std::to_string(i++) + "]: " + std::to_string(e));
        }
        i = 0;
        for (auto e __attribute__((unused)) : powerThresholds)
        {
            PRINTDEBUGMESSAGE("TemperatureSimConfig", "powerThreshold["
                              + std::to_string(i++) + "]: " + std::to_string(e));
        }
    }
};

#endif // TEMPERATURESIMCONFIG_H

