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
 *    Eder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 *    Luiza Correa
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <systemc.h>
#include <string>
#include <cstdint>
#include "memspec/MemSpec.h"
#include "TemperatureSimConfig.h"
#include "../common/utils.h"

#include "../error/eccbaseclass.h"

class Configuration
{
public:
    static Configuration &getInstance()
    {
        static Configuration _instance;
        return _instance;
    }
private:
    Configuration() {}
    Configuration(const Configuration &);
    Configuration & operator = (const Configuration &);

public:
    static std::string memspecUri;
    static std::string mcconfigUri;
    std::string pathToResources;

    // MCConfig:
    enum class PagePolicy {Open, Closed, OpenAdaptive, ClosedAdaptive} pagePolicy;
    enum class Scheduler {Fifo, FrFcfs, FrFcfsGrp} scheduler;
    enum class SchedulerBuffer {Bankwise, ReadWrite, Shared} schedulerBuffer;
    enum class CmdMux {Oldest, Strict} cmdMux;
    enum class RespQueue {Fifo, Reorder} respQueue;
    enum class Arbiter {Simple, Fifo, Reorder} arbiter;
    unsigned int requestBufferSize = 8;
    enum class RefreshPolicy {NoRefresh, AllBank, PerBank, SameBank} refreshPolicy;
    unsigned int refreshMaxPostponed = 0;
    unsigned int refreshMaxPulledin = 0;
    enum class PowerDownPolicy {NoPowerDown, Staggered} powerDownPolicy;
    unsigned int powerDownTimeout = 3;
    unsigned int maxActiveTransactions = 64;
    sc_time arbitrationDelayFw = SC_ZERO_TIME;
    sc_time arbitrationDelayBw = SC_ZERO_TIME;
    sc_time thinkDelayFw = SC_ZERO_TIME;
    sc_time thinkDelayBw = SC_ZERO_TIME;
    sc_time phyDelayFw = SC_ZERO_TIME;
    sc_time phyDelayBw = SC_ZERO_TIME;

    // SimConfig
    std::string simulationName = "default";
    bool databaseRecording = false;
    bool powerAnalysis = false;
    bool enableWindowing = false;
    unsigned int windowSize = 1000;
    bool debug = false;
    bool thermalSimulation = false;
    bool simulationProgressBar = false;
    bool checkTLM2Protocol = false;
    enum class ECCMode {Disabled, Hamming} eccMode;
    ECCBaseClass *pECC = nullptr;
    bool useMalloc = false;
    unsigned long long int addressOffset = 0;

    // MemSpec (from DRAM-Power)
    const MemSpec *memSpec;

    void setParameter(std::string name, nlohmann::json value);

    //Configs for Seed, csv file and StorageMode
    unsigned int errorChipSeed;
    std::string errorCSVFile = "not defined.";
    enum class StoreMode {NoStorage, Store, ErrorModel} storeMode;

    // Temperature Simulation related
    TemperatureSimConfig temperatureSim;

    unsigned int adjustNumBytesAfterECC(unsigned bytes);
    void setPathToResources(std::string path);

    void loadMCConfig(Configuration &config, std::string amconfigUri);
    void loadSimConfig(Configuration &config, std::string simconfigUri);
    void loadMemSpec(Configuration &config, std::string memspecUri);
    void loadTemperatureSimConfig(Configuration &config, std::string simconfigUri);
};

#endif // CONFIGURATION_H

