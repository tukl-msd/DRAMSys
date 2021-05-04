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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Éder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 *    Luiza Correa
 */

#include <cassert>

#include "Configuration.h"
#include "memspec/MemSpecDDR3.h"
#include "memspec/MemSpecDDR4.h"
#include "memspec/MemSpecDDR5.h"
#include "memspec/MemSpecWideIO.h"
#include "memspec/MemSpecLPDDR4.h"
#include "memspec/MemSpecWideIO2.h"
#include "memspec/MemSpecHBM2.h"
#include "memspec/MemSpecGDDR5.h"
#include "memspec/MemSpecGDDR5X.h"
#include "memspec/MemSpecGDDR6.h"

using json = nlohmann::json;

std::string Configuration::memspecUri = "";
std::string Configuration::mcconfigUri = "";

enum sc_time_unit string2TimeUnit(std::string s)
{
    if (s == "s")
        return SC_SEC;
    else if (s == "ms")
        return SC_MS;
    else if (s == "us")
        return SC_US;
    else if (s == "ns")
        return SC_NS;
    else if (s == "ps")
        return SC_PS;
    else if (s == "fs")
        return SC_FS;
    else {
        SC_REPORT_FATAL("Configuration",
                        ("Could not convert to enum sc_time_unit: " + s).c_str());
        throw;
    }
}

void Configuration::setParameter(std::string name, nlohmann::json value)
{
    // MCConfig
    if (name == "PagePolicy")
    {
        if (value == "Open")
            pagePolicy = PagePolicy::Open;
        else if (value == "Closed")
            pagePolicy = PagePolicy::Closed;
        else if (value == "OpenAdaptive")
            pagePolicy = PagePolicy::OpenAdaptive;
        else if (value == "ClosedAdaptive")
            pagePolicy = PagePolicy::ClosedAdaptive;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported page policy!");
    }
    else if (name == "Scheduler")
    {
        if (value == "Fifo")
            scheduler = Scheduler::Fifo;
        else if (value == "FrFcfs")
            scheduler = Scheduler::FrFcfs;
        else if (value == "FrFcfsGrp")
            scheduler = Scheduler::FrFcfsGrp;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported scheduler!");
    }
    else if (name == "SchedulerBuffer")
    {
        if (value == "Bankwise")
            schedulerBuffer = SchedulerBuffer::Bankwise;
        else if (value == "ReadWrite")
            schedulerBuffer = SchedulerBuffer::ReadWrite;
        else if (value == "Shared")
            schedulerBuffer = SchedulerBuffer::Shared;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported scheduler buffer!");
    }
    else if (name == "RequestBufferSize")
    {
        requestBufferSize = value;
        if (requestBufferSize == 0)
            SC_REPORT_FATAL("Configuration", "Minimum request buffer size is 1!");
    }
    else if (name == "CmdMux")
    {
        if (value == "Oldest")
            cmdMux = CmdMux::Oldest;
        else if (value == "Strict")
            cmdMux = CmdMux::Strict;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported cmd mux!");
    }
    else if (name == "RespQueue")
    {
        if (value == "Fifo")
            respQueue = RespQueue::Fifo;
        else if (value == "Reorder")
            respQueue = RespQueue::Reorder;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported response queue!");
    }
    else if (name == "Arbiter")
    {
        if (value == "Simple")
            arbiter = Arbiter::Simple;
        else if (value == "Fifo")
            arbiter = Arbiter::Fifo;
        else if (value == "Reorder")
            arbiter = Arbiter::Reorder;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported arbiter!");
    }
    else if (name == "RefreshPolicy")
    {
        if (value == "NoRefresh")
            refreshPolicy = RefreshPolicy::NoRefresh;
        else if (value == "AllBank" || value == "Rankwise")
            refreshPolicy = RefreshPolicy::AllBank;
        else if (value == "PerBank" || value == "Bankwise")
            refreshPolicy = RefreshPolicy::PerBank;
        else if (value == "SameBank" || value == "Groupwise")
            refreshPolicy = RefreshPolicy::SameBank;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported refresh policy!");
    }
    else if (name == "RefreshMaxPostponed")
        refreshMaxPostponed = value;
    else if (name == "RefreshMaxPulledin")
        refreshMaxPulledin = value;
    else if (name == "PowerDownPolicy")
    {
        if (value == "NoPowerDown")
            powerDownPolicy = PowerDownPolicy::NoPowerDown;
        else if (value == "Staggered")
            powerDownPolicy = PowerDownPolicy::Staggered;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported power down policy!");
    }
    else if (name == "PowerDownTimeout")
        powerDownTimeout = value;
    else if (name == "MaxActiveTransactions")
        maxActiveTransactions = value;
    else if (name == "ArbitrationDelayFw")
        arbitrationDelayFw = std::round(sc_time(value, SC_NS) / memSpec->tCK) * memSpec->tCK;
    else if (name == "ArbitrationDelayBw")
        arbitrationDelayBw = std::round(sc_time(value, SC_NS) / memSpec->tCK) * memSpec->tCK;
    else if (name == "ThinkDelayFw")
        thinkDelayFw = std::round(sc_time(value, SC_NS) / memSpec->tCK) * memSpec->tCK;
    else if (name == "ThinkDelayBw")
        thinkDelayBw = std::round(sc_time(value, SC_NS) / memSpec->tCK) * memSpec->tCK;
    else if (name == "PhyDelayFw")
        phyDelayFw = std::round(sc_time(value, SC_NS) / memSpec->tCK) * memSpec->tCK;
    else if (name == "PhyDelayBw")
        phyDelayBw = std::round(sc_time(value, SC_NS) / memSpec->tCK) * memSpec->tCK;
    //SimConfig------------------------------------------------
    else if (name == "SimulationName")
        simulationName = value;
    else if (name == "DatabaseRecording")
        databaseRecording = value;
    else if (name == "PowerAnalysis")
        powerAnalysis = value;
    else if (name == "EnableWindowing")
        enableWindowing = value;
    else if (name == "WindowSize")
    {
        windowSize = value;
        if (windowSize == 0)
            SC_REPORT_FATAL("Configuration", "Minimum window size is 1");
    }
    else if (name == "Debug")
        debug = value;
    else if (name == "ThermalSimulation")
        thermalSimulation = value;
    else if (name == "SimulationProgressBar")
        simulationProgressBar = value;
    else if (name == "AddressOffset")
        addressOffset = value;
    else if (name == "UseMalloc")
        useMalloc = value;
    else if (name == "CheckTLM2Protocol")
        checkTLM2Protocol = value;
    else if (name == "ECCControllerMode")
    {
        if (value == "Disabled")
            eccMode = ECCMode::Disabled;
        else if (value == "Hamming")
            eccMode = ECCMode::Hamming;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported ECC mode!");
    }
    // Specification for ErrorChipSeed, ErrorCSVFile path and StoreMode
    else if (name == "ErrorChipSeed")
        errorChipSeed = value;
    else if (name == "ErrorCSVFile")
        errorCSVFile = value;
    else if (name == "StoreMode")
    {
        if (value == "NoStorage")
            storeMode = StoreMode::NoStorage;
        else if (value == "Store")
            storeMode = StoreMode::Store;
        else if (value == "ErrorModel")
            storeMode = StoreMode::ErrorModel;
        else
            SC_REPORT_FATAL("Configuration", "Unsupported store mode!");
    }

    // Temperature Simulation related
    else if (name == "TemperatureScale")
    {
        if (value != "Celsius" && value != "Fahrenheit" && value != "Kelvin")
            SC_REPORT_FATAL("Configuration",
                            ("Invalid value for parameter " + name + ".").c_str());
        temperatureSim.temperatureScale = value;
    }
    else if (name == "StaticTemperatureDefaultValue")
        temperatureSim.staticTemperatureDefaultValue = value;
    else if (name == "ThermalSimPeriod")
        temperatureSim.thermalSimPeriod = value;
    else if (name == "ThermalSimUnit")
        temperatureSim.thermalSimUnit = string2TimeUnit(value);
    else if (name == "PowerInfoFile")
    {
        temperatureSim.powerInfoFile = value;
        temperatureSim.parsePowerInfoFile();
    }
    else if (name == "IceServerIp")
        temperatureSim.iceServerIp = value;
    else if (name == "IceServerPort")
        temperatureSim.iceServerPort = value;
    else if (name == "SimPeriodAdjustFactor")
        temperatureSim.simPeriodAdjustFactor = value;
    else if (name == "NPowStableCyclesToIncreasePeriod")
        temperatureSim.nPowStableCyclesToIncreasePeriod = value;
    else if (name == "GenerateTemperatureMap")
        temperatureSim.generateTemperatureMap = value;
    else if (name == "GeneratePowerMap")
        temperatureSim.generatePowerMap = value;
    else
        SC_REPORT_FATAL("Configuration",
                        ("Parameter " + name + " not defined in Configuration").c_str());
}

void Configuration::setPathToResources(std::string path)
{
    pathToResources = path;
    temperatureSim.setPathToResources(path);
}

// Changes the number of bytes depeding on the ECC Controller. This function is needed for modules which get data directly or indirectly from the ECC Controller
unsigned int Configuration::adjustNumBytesAfterECC(unsigned nBytes)
{
    // Manipulate the number of bytes only if there is an ECC Controller selected
    if (eccMode == ECCMode::Disabled)
        return nBytes;
    else // if (eccMode == ECCMode::Hamming)
    {
        assert(pECC != nullptr);
        return pECC->AllocationSize(nBytes);
    }
}

void Configuration::loadSimConfig(Configuration &config, std::string simconfigUri)
{
    json doc = parseJSON(simconfigUri);
    if (doc["simconfig"].empty())
        SC_REPORT_FATAL("Configuration", "simconfig is empty.");
    for (auto& x : doc["simconfig"].items())
        config.setParameter(x.key(), x.value());
}

void Configuration::loadTemperatureSimConfig(Configuration &config, std::string thermalsimconfigUri)
{
    json doc = parseJSON(thermalsimconfigUri);
    if (doc["thermalsimconfig"].empty())
        SC_REPORT_FATAL("Configuration", "thermalsimconfig is empty.");
    for (auto& x : doc["thermalsimconfig"].items())
        config.setParameter(x.key(), x.value());
}

void Configuration::loadMCConfig(Configuration &config, std::string mcconfigUri)
{
    config.mcconfigUri = mcconfigUri;
    json doc = parseJSON(mcconfigUri);
    if (doc["mcconfig"].empty())
        SC_REPORT_FATAL("Configuration", "mcconfig is empty.");
    for (auto& x : doc["mcconfig"].items())
        config.setParameter(x.key(), x.value());
}

void Configuration::loadMemSpec(Configuration &config, std::string memspecUri)
{
    config.memspecUri = memspecUri;
    json doc = parseJSON(memspecUri);
    json jMemSpec = doc["memspec"];

    std::string memoryType = jMemSpec["memoryType"];

    if (memoryType == "DDR3")
        memSpec = new MemSpecDDR3(jMemSpec);
    else if (memoryType == "DDR4")
        memSpec = new MemSpecDDR4(jMemSpec);
    else if (memoryType == "DDR5")
        memSpec = new MemSpecDDR5(jMemSpec);
    else if (memoryType == "LPDDR4")
        memSpec = new MemSpecLPDDR4(jMemSpec);
    else if (memoryType == "WIDEIO_SDR")
        memSpec = new MemSpecWideIO(jMemSpec);
    else if (memoryType == "WIDEIO2")
        memSpec = new MemSpecWideIO2(jMemSpec);
    else if (memoryType == "HBM2")
        memSpec = new MemSpecHBM2(jMemSpec);
    else if (memoryType == "GDDR5")
        memSpec = new MemSpecGDDR5(jMemSpec);
    else if (memoryType == "GDDR5X")
        memSpec = new MemSpecGDDR5X(jMemSpec);
    else if (memoryType == "GDDR6")
        memSpec = new MemSpecGDDR6(jMemSpec);
    else
        SC_REPORT_FATAL("Configuration", "Unsupported DRAM type");
}
