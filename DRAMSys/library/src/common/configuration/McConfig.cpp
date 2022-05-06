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

#include "McConfig.h"
#include <iostream>
namespace DRAMSysConfiguration
{

void to_json(json &j, const McConfig &c)
{
    j = json{{"PagePolicy", c.pagePolicy},
             {"Scheduler", c.scheduler},
             {"HighWatermark", c.highWatermark},
             {"LowWatermark", c.lowWatermark},
             {"SchedulerBuffer", c.schedulerBuffer},
             {"RequestBufferSize", c.requestBufferSize},
             {"CmdMux", c.cmdMux},
             {"RespQueue", c.respQueue},
             {"RefreshPolicy", c.refreshPolicy},
             {"RefreshMaxPostponed", c.refreshMaxPostponed},
             {"RefreshMaxPulledin", c.refreshMaxPulledin},
             {"PowerDownPolicy", c.powerDownPolicy},
             {"Arbiter", c.arbiter},
             {"MaxActiveTransactions", c.maxActiveTransactions},
             {"RefreshManagment", c.refreshManagement},
             {"ArbitrationDelayFw", c.arbitrationDelayFw},
             {"ArbitrationDelayBw", c.arbitrationDelayBw},
             {"ThinkDelayFw", c.thinkDelayFw},
             {"ThinkDelayBw", c.thinkDelayBw},
             {"PhyDelayFw", c.phyDelayFw},
             {"PhyDelayBw", c.phyDelayBw}};

    remove_null_values(j);
}

void from_json(const json &j, McConfig &c)
{
    json j_mcconfig = get_config_json(j, mcConfigPath, "mcconfig");

    if (j_mcconfig.contains("PagePolicy"))
        j_mcconfig.at("PagePolicy").get_to(c.pagePolicy);

    if (j_mcconfig.contains("Scheduler"))
        j_mcconfig.at("Scheduler").get_to(c.scheduler);

    if (j_mcconfig.contains("HighWatermark"))
        j_mcconfig.at("HighWatermark").get_to(c.highWatermark);

    if (j_mcconfig.contains("LowWatermark"))
        j_mcconfig.at("LowWatermark").get_to(c.lowWatermark);

    if (j_mcconfig.contains("SchedulerBuffer"))
        j_mcconfig.at("SchedulerBuffer").get_to(c.schedulerBuffer);

    if (j_mcconfig.contains("RequestBufferSize"))
        j_mcconfig.at("RequestBufferSize").get_to(c.requestBufferSize);

    if (j_mcconfig.contains("CmdMux"))
        j_mcconfig.at("CmdMux").get_to(c.cmdMux);

    if (j_mcconfig.contains("RespQueue"))
        j_mcconfig.at("RespQueue").get_to(c.respQueue);

    if (j_mcconfig.contains("RefreshPolicy"))
        j_mcconfig.at("RefreshPolicy").get_to(c.refreshPolicy);

    if (j_mcconfig.contains("RefreshMaxPostponed"))
        j_mcconfig.at("RefreshMaxPostponed").get_to(c.refreshMaxPostponed);

    if (j_mcconfig.contains("RefreshMaxPulledin"))
        j_mcconfig.at("RefreshMaxPulledin").get_to(c.refreshMaxPulledin);

    if (j_mcconfig.contains("PowerDownPolicy"))
        j_mcconfig.at("PowerDownPolicy").get_to(c.powerDownPolicy);

    if (j_mcconfig.contains("Arbiter"))
        j_mcconfig.at("Arbiter").get_to(c.arbiter);

    if (j_mcconfig.contains("MaxActiveTransactions"))
        j_mcconfig.at("MaxActiveTransactions").get_to(c.maxActiveTransactions);

    if (j_mcconfig.contains("RefreshManagment"))
        j_mcconfig.at("RefreshManagment").get_to(c.refreshManagement);

    if (j_mcconfig.contains("ArbitrationDelayFw"))
        j_mcconfig.at("ArbitrationDelayFw").get_to(c.arbitrationDelayFw);

    if (j_mcconfig.contains("ArbitrationDelayBw"))
        j_mcconfig.at("ArbitrationDelayBw").get_to(c.arbitrationDelayBw);

    if (j_mcconfig.contains("ThinkDelayFw"))
        j_mcconfig.at("ThinkDelayFw").get_to(c.thinkDelayFw);

    if (j_mcconfig.contains("ThinkDelayBw"))
        j_mcconfig.at("ThinkDelayBw").get_to(c.thinkDelayBw);

    if (j_mcconfig.contains("PhyDelayFw"))
        j_mcconfig.at("PhyDelayFw").get_to(c.phyDelayFw);

    if (j_mcconfig.contains("PhyDelayBw"))
        j_mcconfig.at("PhyDelayBw").get_to(c.phyDelayBw);

    invalidateEnum(c.pagePolicy);
    invalidateEnum(c.scheduler);
    invalidateEnum(c.schedulerBuffer);
    invalidateEnum(c.cmdMux);
    invalidateEnum(c.respQueue);
    invalidateEnum(c.refreshPolicy);
    invalidateEnum(c.respQueue);
    invalidateEnum(c.powerDownPolicy);
    invalidateEnum(c.arbiter);
}

void to_json(json &j, const RefreshPolicy &r)
{
    if (r == RefreshPolicy::NoRefresh)
        j = "NoRefresh";
    else if (r == RefreshPolicy::AllBank)
        j = "AllBank";
    else if (r == RefreshPolicy::PerBank)
        j = "PerBank";
    else if (r == RefreshPolicy::Per2Bank)
        j = "Per2Bank";
    else if (r == RefreshPolicy::SameBank)
        j = "SameBank";
    else
        j = nullptr;
}

void from_json(const json &j, RefreshPolicy &r)
{
    if (j == "NoRefresh")
        r = RefreshPolicy::NoRefresh;
    else if (j == "AllBank" || j == "Rankwise")
        r = RefreshPolicy::AllBank;
    else if (j == "PerBank" || j == "Bankwise")
        r = RefreshPolicy::PerBank;
    else if (j == "SameBank" || j == "Groupwise")
        r = RefreshPolicy::SameBank;
    else if (j == "Per2Bank")
        r = RefreshPolicy::Per2Bank;
    else
        r = RefreshPolicy::Invalid;
}

void from_dump(const std::string &dump, McConfig &c)
{
    json json_mcconfig = json::parse(dump).at("mcconfig");
    json_mcconfig.get_to(c);
}

std::string dump(const McConfig &c, unsigned int indentation)
{
    json json_mcconfig;
    json_mcconfig["mcconfig"] = c;
    return json_mcconfig.dump(indentation);
}

} // namespace DRAMSysConfiguration
