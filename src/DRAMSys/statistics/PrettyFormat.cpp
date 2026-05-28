/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 * Author:
 *    Derek Christ
 */

#include "PrettyFormat.h"

#include "DRAMSys/statistics/Group.h"
#include "DRAMSys/statistics/Stat.h"
#include "DRAMSys/statistics/StatProvider.h"

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace DRAMSys::Statistics
{

static std::string formatScalar(Quantity quantity, double value)
{
    static constexpr unsigned GIGA = 1'000'000'000;
    static constexpr unsigned PERCENT = 100;
    switch (quantity)
    {
    case Quantity::Bandwidth:
        return fmt::format("{:>6.2f}", value / GIGA);
    case Quantity::Percentage:
        return fmt::format("{:.2f}", value * PERCENT);
    default:
        return fmt::format("{}", value);
    }
}

static std::string getUnit(Quantity quantity)
{
    switch (quantity)
    {
    case Quantity::Bandwidth:
        return "GB/s";
    case Quantity::Energy:
        return "J";
    case Quantity::Time:
        return "s";
    case Quantity::Percentage:
        return "%";
    default:
        return "";
    }
}

static void appendStat(std::string& out,
                       std::string_view context,
                       std::string_view name,
                       Quantity quantity,
                       std::string_view description,
                       double value)
{
    fmt::format_to(std::back_inserter(out),
                   "{:<72} {:>14} {:<6} # {}\n",
                   fmt::format("{}::{}:", context, name),
                   formatScalar(quantity, value),
                   getUnit(quantity),
                   description);
}

static std::string formatGroup(Statistics::Group const& group, std::string_view context)
{
    std::string out;
    fmt::format_to(std::back_inserter(out), "== {} ==\n", context);
    for (auto const& stat : group.stats)
    {
        if (auto* scalar = dynamic_cast<ScalarStat*>(stat.get()))
        {
            appendStat(out, context, stat->name, stat->quantity, stat->description, scalar->value);
        }
        else if (auto* vec = dynamic_cast<VectorStat*>(stat.get()))
        {
            for (std::size_t i = 0; i < vec->values.size(); ++i)
            {
                appendStat(out,
                           context,
                           fmt::format("{}[{}]", stat->name, i),
                           stat->quantity,
                           stat->description,
                           vec->values[i]);
            }
        }
    }
    for (auto const& sub : group.subGroups)
        out += formatGroup(*sub, fmt::format("{}.{}", context, sub->name));

    return out;
}

void PrettyFormat::collectStats(sc_core::sc_object* obj, std::ostream& os, std::string path)
{
    if (auto* provider = dynamic_cast<StatProvider*>(obj))
    {
        provider->updateStats();
        auto const& group = provider->getStatGroup();

        std::string childPath = path.empty() ? group.name : fmt::format("{}.{}", path, group.name);
        fmt::print(os, formatGroup(group, childPath));

        for (auto* child : obj->get_child_objects())
            collectStats(child, os, childPath);
    }
}

} // namespace DRAMSys::Statistics
