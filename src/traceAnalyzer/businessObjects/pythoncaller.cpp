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
#include <exception>
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>

std::string PythonCaller::generatePlots(std::string_view pathToTrace)
{
    try
    {
        pybind11::module_ metricsModule = pybind11::module_::import("plots");
        auto result = metricsModule.attr("generatePlots")(pathToTrace).cast<std::string>();
        return result;
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << std::endl;
    }

    return {};
}

std::vector<std::string> PythonCaller::availableMetrics(std::string_view pathToTrace)
{
    try
    {
        pybind11::module_ metricsModule = pybind11::module_::import("metrics");
        pybind11::list result = metricsModule.attr("getMetrics")(pathToTrace);
        return result.cast<std::vector<std::string>>();
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << std::endl;
    }

    return {};
}

TraceCalculatedMetrics PythonCaller::evaluateMetrics(std::string_view pathToTrace,
                                                     std::vector<long> selectedMetrics)
{
    TraceCalculatedMetrics metrics(pathToTrace.data());

    try
    {
        pybind11::module_ metricsModule = pybind11::module_::import("metrics");
        pybind11::list result =
            metricsModule.attr("calculateMetrics")(pathToTrace, selectedMetrics);
        auto metricList = result.cast<std::vector<pybind11::tuple>>();

        for (auto metricPair : metricList)
        {
            std::string name = metricPair[0].cast<std::string>();
            double value = metricPair[1].cast<double>();
            metrics.addCalculatedMetric({name, value});
        }
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << std::endl;
    }

    return metrics;
}

std::string PythonCaller::dumpVcd(std::string_view pathToTrace)
{
    try
    {
        pybind11::module_ vcdModule = pybind11::module_::import("vcdExport");
        pybind11::str result = vcdModule.attr("dumpVcd")(pathToTrace);
        return result.cast<std::string>();
    }
    catch (std::exception const& err)
    {
        std::cout << err.what() << std::endl;
    }

    return {};
}
