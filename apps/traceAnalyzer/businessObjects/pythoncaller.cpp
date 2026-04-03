/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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
 *    Robert Gernhardt
 *    Matthias Jung
 *    Felipe S. Prado
 *    Derek Christ
 */

#include "pythoncaller.h"

#include <array>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace PythonCaller
{

static std::string runCommand(std::string const& command)
{
    std::string cmd = command + " 2>&1";
    static constexpr std::size_t BUFFER_SIZE = 256;
    std::array<char, BUFFER_SIZE> buffer{};
    std::string result;

    std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
        throw std::runtime_error("popen() failed");

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    if (!result.empty() && result.back() == '\n')
        result.pop_back();

    int exitCode = pclose(pipe.release());
    if (exitCode != 0)
        throw std::runtime_error("Python error: " + result);

    return result;
}

std::string generatePlots(std::string const& pathToTrace)
{
    try
    {
        auto res = runCommand("dramsys_plots " + pathToTrace);
        return res;
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << "\n";
    }
    return {};
}

std::vector<std::string> availableMetrics(std::string const& pathToTrace)
{
    try
    {
        auto jsonString = runCommand("dramsys_metrics --json --list-metrics " + pathToTrace);

        std::vector<std::string> result;
        nlohmann::json j = nlohmann::json::parse(jsonString);

        if (!j.contains("metrics") || !j["metrics"].is_array())
            return result;

        for (const auto& item : j["metrics"])
        {
            result.push_back(item.get<std::string>());
        }
        return result;
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << "\n";
    }
    return {};
}

TraceCalculatedMetrics evaluateMetrics(std::string const& pathToTrace,
                                       std::vector<std::string> const& selectedMetrics)
{
    try
    {
        TraceCalculatedMetrics metrics;
        metrics.traceName = pathToTrace.c_str();
        std::ostringstream metricsStream;
        for (auto const& metric : selectedMetrics)
        {
            metricsStream << metric << " ";
        }

        auto jsonString = runCommand("dramsys_metrics --json " + pathToTrace + " --metric " +
                                     metricsStream.str());
        nlohmann::json j = nlohmann::json::parse(jsonString);

        if (!j.contains("metrics") || !j["metrics"].is_object())
            return metrics;

        for (auto const& [key, value] : j["metrics"].items())
        {
            if (value.is_number())
            {
                metrics.calculatedMetrics.push_back(TraceCalculatedMetrics::CalculatedMetric{
                    key.c_str(), QString::number(value.get<double>(), 'f')});
            }
            else if (value.is_array())
            {
                QString result;
                for (size_t i = 0; i < value.size(); ++i)
                {
                    result += QString::number(value[i].get<double>(), 'f');
                    if (i + 1 < value.size())
                        result += " ";
                }
                metrics.calculatedMetrics.push_back(
                    TraceCalculatedMetrics::CalculatedMetric{key.c_str(), result});
            }
        }

        return metrics;
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << "\n";
    }
    return {};
}

std::string dumpVcd(std::string const& pathToTrace, std::string const& outputFilePath)
{
    try
    {
        auto res = runCommand("dramsys_vcd_export " + pathToTrace + " " + outputFilePath);
        return res;
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << "\n";
    }
    return {};
}

} // namespace PythonCaller
