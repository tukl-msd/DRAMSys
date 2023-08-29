/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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

#include "DRAMSysConfiguration.h"

#include <fstream>
#include <iostream>

namespace DRAMSys::Config
{

Configuration from_path(std::string_view path, std::string_view resourceDirectory)
{
    std::ifstream file(path.data());

    enum class SubConfig
    {
        MemSpec,
        AddressMapping,
        McConfig,
        SimConfig,
        TraceSetup,
        Unkown
    } current_sub_config;

    // This custom parser callback is responsible to swap out the paths to the sub-config json files
    // with the actual json data.
    std::function<bool(int depth, nlohmann::detail::parse_event_t event, json_t& parsed)>
        parser_callback;
    parser_callback = [&parser_callback, &current_sub_config, resourceDirectory](
                          int depth, nlohmann::detail::parse_event_t event, json_t& parsed) -> bool
    {
        using nlohmann::detail::parse_event_t;

        if (depth != 2)
            return true;

        if (event == parse_event_t::key)
        {
            assert(parsed.is_string());

            if (parsed == MemSpec::KEY)
                current_sub_config = SubConfig::MemSpec;
            else if (parsed == AddressMapping::KEY)
                current_sub_config = SubConfig::AddressMapping;
            else if (parsed == McConfig::KEY)
                current_sub_config = SubConfig::McConfig;
            else if (parsed == SimConfig::KEY)
                current_sub_config = SubConfig::SimConfig;
            else if (parsed == TraceSetupConstants::KEY)
                current_sub_config = SubConfig::TraceSetup;
            else
                current_sub_config = SubConfig::Unkown;
        }

        // In case we have an value (string) instead of an object, replace the value with the loaded
        // json object.
        if (event == parse_event_t::value && current_sub_config != SubConfig::Unkown)
        {
            // Replace name of json file with actual json data
            auto parse_json = [&parser_callback,
                               resourceDirectory](std::string_view base_dir,
                                                  std::string_view sub_config_key,
                                                  const std::string& filename) -> json_t
            {
                std::filesystem::path path(resourceDirectory);
                path /= base_dir;
                path /= filename;

                std::ifstream json_file(path);

                if (!json_file.is_open())
                    throw std::runtime_error("Failed to open file " + std::string(path));

                json_t json =
                    json_t::parse(json_file, parser_callback, true, true).at(sub_config_key);
                return json;
            };

            if (current_sub_config == SubConfig::MemSpec)
                parsed = parse_json(MemSpec::SUB_DIR, MemSpec::KEY, parsed);
            else if (current_sub_config == SubConfig::AddressMapping)
                parsed = parse_json(AddressMapping::SUB_DIR, AddressMapping::KEY, parsed);
            else if (current_sub_config == SubConfig::McConfig)
                parsed = parse_json(McConfig::SUB_DIR, McConfig::KEY, parsed);
            else if (current_sub_config == SubConfig::SimConfig)
                parsed = parse_json(SimConfig::SUB_DIR, SimConfig::KEY, parsed);
            else if (current_sub_config == SubConfig::TraceSetup)
                parsed = parse_json(TraceSetupConstants::SUB_DIR, TraceSetupConstants::KEY, parsed);
        }

        return true;
    };

    if (file.is_open())
    {
        json_t simulation = json_t::parse(file, parser_callback, true, true).at(Configuration::KEY);
        return simulation.get<DRAMSys::Config::Configuration>();
    }
    throw std::runtime_error("Failed to open file " + std::string(path));
}

} // namespace DRAMSys::Config
