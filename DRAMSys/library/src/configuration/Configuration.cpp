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
        pagePolicy = value;
    else if (name == "Scheduler")
        scheduler = value;
    else if (name == "RequestBufferSize")
        requestBufferSize = value;
    else if (name == "CmdMux")
        cmdMux = value;
    else if (name == "RespQueue")
        respQueue = value;
    else if (name == "RefreshPolicy")
        refreshPolicy = value;
    else if (name == "RefreshMaxPostponed")
        refreshMaxPostponed = value;
    else if (name == "RefreshMaxPulledin")
        refreshMaxPulledin = value;
    else if (name == "PowerDownPolicy")
        powerDownPolicy = value;
    else if (name == "PowerDownTimeout")
        powerDownTimeout = value;
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
            SC_REPORT_FATAL("Configuration",
                            ("Invalid value for parameter " + name +
                             ". This parameter must be at least one.").c_str());
    }
    else if (name == "Debug")
        debug = value;
    else if (name == "ThermalSimulation")
        thermalSimulation = value;
    else if (name == "SimulationProgressBar")
        simulationProgressBar = value;
    else if (name == "AddressOffset")
    {
#ifdef DRAMSYS_GEM5
        addressOffset = value;
#else
        addressOffset = 0;
#endif
    }
    else if (name == "UseMalloc")
        useMalloc = value;
    else if (name == "CheckTLM2Protocol")
        checkTLM2Protocol = value;
    else if (name == "ECCControllerMode")
        ECCMode = value;
    // Specification for ErrorChipSeed, ErrorCSVFile path and StoreMode
    else if (name == "ErrorChipSeed")
        errorChipSeed = value;
    else if (name == "ErrorCSVFile")
        errorCSVFile = value;
    else if (name == "StoreMode")
        storeMode = value;
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

// Returns the total memory size in bytes
std::uint64_t Configuration::getSimMemSizeInBytes()
{
    // 1. Get number of banks, rows, columns and data width in bits for one die (or chip)
    std::string type = memSpec->memoryType;
    std::uint64_t ranks = memSpec->numberOfRanks;
    std::uint64_t bankgroups = memSpec->numberOfBankGroups;
    std::uint64_t banks = memSpec->numberOfBanks;
    std::uint64_t rows = memSpec->numberOfRows;
    std::uint64_t columns = memSpec->numberOfColumns;
    std::uint64_t bitWidth = memSpec->bitWidth;
    std::uint64_t devicesOnDIMM = memSpec->numberOfDevicesOnDIMM;
    // 2. Calculate size of one DRAM chip in bits
    std::uint64_t chipBitSize = banks * rows * columns * bitWidth;
    // 3. Calculate size of one DRAM chip in bytes
    std::uint64_t chipSize = chipBitSize / 8;
    // 4. Total memory size in Bytes of one DIMM (with only support of 1 rank on a DIMM)
    std::uint64_t memorySize = chipSize * memSpec->numberOfDevicesOnDIMM;

    std::cout << headline << std::endl;
    std::cout << "Per Channel Configuration:" << std::endl << std::endl;
    std::cout << " Memory type:           " << type                  << std::endl;
    std::cout << " Memory size in bytes:  " << memorySize            << std::endl;
    std::cout << " Number of ranks:       " << ranks                 << std::endl;
    std::cout << " Number of bankgroups:  " << bankgroups            << std::endl;
    std::cout << " Number of banks:       " << banks                 << std::endl;
    std::cout << " Number of rows:        " << rows                  << std::endl;
    std::cout << " Number of columns:     " << columns               << std::endl;
    std::cout << " Chip data bus width:   " << bitWidth              << std::endl;
    std::cout << " Chip size in bits:     " << chipBitSize           << std::endl;
    std::cout << " Chip Size in bytes:    " << chipSize              << std::endl;
    std::cout << " Devices/Chips on DIMM: " << devicesOnDIMM         << std::endl;
    std::cout << std::endl;

    assert(memorySize > 0);
    return memorySize;
}

// Returns the width of the data bus.
// All DRAM chips on a DIMM operate in lockstep,
// which constituing aggregate data bus width = chip's bus width * # locksteep-operated chips
// The bus width is given in bits, e.g., 64-bit data bus, 128-bit data bus, etc.
unsigned int Configuration::getDataBusWidth()
{
    return memSpec->bitWidth * memSpec->numberOfDevicesOnDIMM;
}

// Returns the number of bytes transfered in a burst
unsigned int Configuration::getBytesPerBurst()
{
    return (memSpec->burstLength * getDataBusWidth()) / 8;
}

// Changes the number of bytes depeding on the ECC Controller. This function is needed for modules which get data directly or indirectly from the ECC Controller
unsigned int Configuration::adjustNumBytesAfterECC(unsigned nBytes)
{
    // Manipulate the number of bytes only if there is an ECC Controller selected
    if (ECCMode == "Disabled")
        return nBytes;
    else if (ECCMode == "Hamming")
    {
        assert(pECC != nullptr);
        return pECC->AllocationSize(nBytes);
    }
    else
    {
        SC_REPORT_FATAL("Configuration", ("ECC mode " + ECCMode + " unsupported").c_str());
        return 0;
    }
}

void Configuration::loadSimConfig(Configuration &config, std::string simconfigUri)
{
    json doc = parseJSON(simconfigUri);
    for (auto& x : doc["simconfig"].items())
        config.setParameter(x.key(), x.value());
}

void Configuration::loadTemperatureSimConfig(Configuration &config, std::string thermalsimconfigUri)
{
    json doc = parseJSON(thermalsimconfigUri);
    for (auto& x : doc["thermalsimconfig"].items())
        config.setParameter(x.key(), x.value());
}

void Configuration::loadMCConfig(Configuration &config, std::string mcconfigUri)
{
    config.mcconfigUri = mcconfigUri;
    json doc = parseJSON(mcconfigUri);
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
