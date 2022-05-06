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
 *    Derek Christ
 */

#ifndef TEMPERATURECONTROLLER_H
#define TEMPERATURECONTROLLER_H

#include <string>

#include <systemc>
#include "../common/DebugManager.h"
#include "../common/utils.h"
#include "../configuration/Configuration.h"

#ifdef THERMALSIM
#include "IceWrapper.h"
#endif

class TemperatureController : sc_core::sc_module
{
public:
    TemperatureController(const TemperatureController&) = delete;
    TemperatureController& operator=(const TemperatureController&) = delete;

    SC_HAS_PROCESS(TemperatureController);
    TemperatureController() = default;
    TemperatureController(const sc_core::sc_module_name& name, const Configuration& config) : sc_core::sc_module(name)
    {
        temperatureScale = config.temperatureSim.temperatureScale;
        dynamicTempSimEnabled = config.thermalSimulation;
        staticTemperature = config.temperatureSim.staticTemperatureDefaultValue;

        if (dynamicTempSimEnabled)
        {
#ifdef THERMALSIM
            // Connect to the server
            std::string ip = config.temperatureSim.iceServerIp;
            unsigned int port = config.temperatureSim.iceServerPort;
            thermalSimulation = new IceWrapper(ip, port);
            PRINTDEBUGMESSAGE(name(), "Dynamic temperature simulation. Server @ "
                              + ip + ":" + std::to_string(port));
#else
            SC_REPORT_FATAL(sc_module::name(),
                            "DRAMSys was build without support to dynamic temperature simulation. Check the README file for further details.");
#endif
            // Initial power dissipation values (got from config)
            currentPowerValues = config.temperatureSim.powerInitialValues;
            lastPowerValues = currentPowerValues;

            // Substantial changes in power will trigger adjustments in the simulaiton period. Get the thresholds from config.
            powerThresholds = config.temperatureSim.powerThresholds;
            decreaseSimPeriod = false;
            periodAdjustFactor = config.temperatureSim.simPeriodAdjustFactor;
            nPowStableCyclesToIncreasePeriod = config.temperatureSim.nPowStableCyclesToIncreasePeriod;
            cyclesSinceLastPeriodAdjust = 0;

            // Get the target period for the thermal simulation from config.
            targetPeriod = config.temperatureSim.thermalSimPeriod;
            period = targetPeriod;
            t_unit = config.temperatureSim.thermalSimUnit;

            genTempMap = config.temperatureSim.generateTemperatureMap;
            temperatureMapFile = "temperature_map";
            std::system("rm -f temperature_map*");

            genPowerMap = config.temperatureSim.generatePowerMap;
            powerMapFile = "power_map";
            std::system("rm -f power_map*");

            SC_THREAD(temperatureThread);
        }
        else
        {
            PRINTDEBUGMESSAGE(sc_module::name(), "Static temperature simulation. Temperature set to " +
                              std::to_string(staticTemperature));
        }
    }

    double getTemperature(int deviceId, float currentPower);

private:
    TemperatureSimConfig::TemperatureScale temperatureScale;
    double temperatureConvert(double tKelvin);

    double staticTemperature;

    bool dynamicTempSimEnabled;

#ifdef THERMALSIM
    IceWrapper *thermalSimulation;
#endif
    std::vector<float> temperaturesBuffer;
    std::vector<float> temperatureValues;

    std::vector<float> currentPowerValues;
    std::vector<float> lastPowerValues;
    std::vector<float> powerThresholds;

    double targetPeriod;
    double period;
    enum sc_core::sc_time_unit t_unit;
    void temperatureThread();
    void updateTemperatures();
    double adjustThermalSimPeriod();
    void checkPowerThreshold(int deviceId);
    bool decreaseSimPeriod;
    unsigned int periodAdjustFactor;
    unsigned int cyclesSinceLastPeriodAdjust;
    unsigned int nPowStableCyclesToIncreasePeriod;

    bool genTempMap;
    std::string temperatureMapFile;
    bool genPowerMap;
    std::string powerMapFile;
};

#endif // TEMPERATURECONTROLLER_H

