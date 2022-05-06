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

#include <cmath>

#include "TemperatureController.h"
#include "../configuration/Configuration.h"

using namespace sc_core;

double TemperatureController::temperatureConvert(double tKelvin)
{
    if (temperatureScale == TemperatureSimConfig::TemperatureScale::Celsius) {
        return tKelvin - 273.15;
    } else if (temperatureScale == TemperatureSimConfig::TemperatureScale::Fahrenheit) {
        return (tKelvin - 273.15) * 1.8 + 32;
    }

    return tKelvin;
}

double TemperatureController::getTemperature(int deviceId, float currentPower)
{
    PRINTDEBUGMESSAGE(name(), "Temperature requested by device " + std::to_string(
                          deviceId) + " current power is " + std::to_string(currentPower));

    if (dynamicTempSimEnabled)
    {
        currentPowerValues.at(deviceId) = currentPower;
        checkPowerThreshold(deviceId);

        // FIXME: using the static temperature value until the vector of temperatures is filled
        if (temperatureValues.empty())
            return temperatureConvert(staticTemperature + 273.15);

        return temperatureConvert(temperatureValues.at(deviceId));
    }
    else
    {
        PRINTDEBUGMESSAGE(name(), "Temperature is " + std::to_string(staticTemperature));
        return staticTemperature;
    }
}

void TemperatureController::updateTemperatures()
{
#ifdef THERMALSIM
    thermalSimulation->sendPowerValues(&currentPowerValues);
    thermalSimulation->simulate();
    thermalSimulation->getTemperature(temperaturesBuffer, TDICE_OUTPUT_INSTANT_SLOT,
                                      TDICE_OUTPUT_TYPE_TFLPEL, TDICE_OUTPUT_QUANTITY_AVERAGE);

    std::string mapfile;
    sc_time ts = sc_time_stamp();
    if (genTempMap == true) {
        mapfile = temperatureMapFile + "_" + std::to_string(ts.to_default_time_units())
                  + ".txt";
        thermalSimulation->getTemperatureMap(mapfile);
    }
    if (genPowerMap == true) {
        mapfile = powerMapFile + "_" + std::to_string(ts.to_default_time_units()) +
                  ".txt";
        thermalSimulation->getPowerMap(mapfile);
    }
#endif
    // Save values just obtained for posterior use
    temperatureValues = temperaturesBuffer;
    // Clear the buffer, otherwise it will grow every request
    temperaturesBuffer.clear();
}

void TemperatureController::checkPowerThreshold(int deviceId)
{
    if (std::abs(lastPowerValues.at(deviceId) - currentPowerValues.at(
                     deviceId)) > powerThresholds.at(deviceId)) {
        decreaseSimPeriod = true;
    }
    lastPowerValues.at(deviceId) = currentPowerValues.at(deviceId);
}

double TemperatureController::adjustThermalSimPeriod()
{
    // Temperature Simulation Period Dynamic Adjustment
    //
    // 1. Adjustment is requierd when:
    //
    // 1.1. The power dissipation of one or more devices change considerably
    // (exceeds the configured threshold for that device in any direction,
    // i.e. increases or decreases substantially) during the current
    // simulaiton period.
    //
    // 1.1.1. The simulation period will be reduced by a factor of 'n' so the
    // simulation occurs 'n' times more often.
    //
    // 1.1.2. The step 1.1.1 will be repeated until the point that there are
    // no sustantial changes in power dissipation between two consecutive
    // executions of the thermal simulation, i.e. all changes for all devices
    // are less than the configured threshold.
    //
    // 1.2. The current simulation period differs from the target period
    // defined in the configuration by the user.
    //
    // 1.2.1 Provided a scenario in which power dissipation changes do not
    // exceed the thresholds, the situation period will be kept for a number
    // of simulation cycles 'nc' and after 'nc' the period will be increased
    // again in steps of 'n/2' until it achieves the desired value given by
    // configuration or the described in 1.1 occurs.

    if (decreaseSimPeriod)
    {
        period = period / periodAdjustFactor;
        cyclesSinceLastPeriodAdjust = 0;
        decreaseSimPeriod = false;
        PRINTDEBUGMESSAGE(name(), "Thermal Simulation period reduced to " + std::to_string(
                              period) + ". Target is " + std::to_string(targetPeriod));
    }
    else
    {
        if (period != targetPeriod) {
            cyclesSinceLastPeriodAdjust++;
            if (cyclesSinceLastPeriodAdjust >= nPowStableCyclesToIncreasePeriod) {
                cyclesSinceLastPeriodAdjust = 0;
                period = period * ((double)periodAdjustFactor / 2);
                if (period > targetPeriod)
                    period = targetPeriod;
                PRINTDEBUGMESSAGE(name(), "Thermal Simulation period increased to "
                                  + std::to_string(period) + ". Target is " + std::to_string(targetPeriod));
            }
        }
    }

    return period;
}

void TemperatureController::temperatureThread()
{
    while (true)
    {
        updateTemperatures();
        double p = adjustThermalSimPeriod();

        NDEBUG_UNUSED(int i) = 0;
        for (NDEBUG_UNUSED(auto t) : temperatureValues) {
            PRINTDEBUGMESSAGE(name(), "Temperature[" + std::to_string(i++)
                              + "] is " + std::to_string(t));
        }
        PRINTDEBUGMESSAGE(name(), "Thermal simulation period is " + std::to_string(p));

        wait(sc_time(p, t_unit));
    }
}
