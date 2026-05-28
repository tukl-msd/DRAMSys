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

#include "JsonFormat.h"

#include "DRAMSys/statistics/Group.h"
#include "DRAMSys/statistics/Stat.h"
#include "DRAMSys/statistics/StatProvider.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace DRAMSys::Statistics
{

static json formatGroup(Statistics::Group const& group)
{
    json j = json::object();

    for (const auto& stat : group.stats)
    {
        if (auto* scalarStat = dynamic_cast<ScalarStat*>(stat.get()))
        {
            if (scalarStat->quantity == Quantity::Count)
                j[stat->name] = static_cast<uint64_t>(scalarStat->value);
            else
                j[stat->name] = scalarStat->value;
        }
        else if (auto* vectorStat = dynamic_cast<VectorStat*>(stat.get()))
        {
            for (std::size_t i = 0; i < vectorStat->values.size(); i++)
            {
                if (vectorStat->quantity == Quantity::Count)
                    j[vectorStat->name][i] = static_cast<uint64_t>(vectorStat->values[i]);
                else
                    j[vectorStat->name][i] = vectorStat->values[i];
            }
        }
    }

    for (const auto* subGroup : group.subGroups)
    {
        j[subGroup->name] = formatGroup(*subGroup);
    }

    return j;
}

void JsonFormat::collectStats(sc_core::sc_object* obj, nlohmann::json& j)
{
    if (auto* provider = dynamic_cast<StatProvider*>(obj))
    {
        provider->updateStats();
        auto const& group = provider->getStatGroup();

        j[group.name] = formatGroup(group);

        for (auto* child : obj->get_child_objects())
            collectStats(child, j[group.name]);
    }
}

} // namespace DRAMSys::Statistics
