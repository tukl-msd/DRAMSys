/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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
 * Author: Lukas Steiner
 */

#include "CmdMuxOldest.h"

#include <systemc>

using namespace sc_core;

namespace DRAMSys
{

CmdMuxOldest::CmdMuxOldest(const MemSpec& memSpec) : memSpec(memSpec)
{
}

CommandTuple::Type CmdMuxOldest::selectCommand(const ReadyCommands& readyCommands)
{
    auto result = readyCommands.cend();
    uint64_t lastPayloadID = UINT64_MAX;
    uint64_t newPayloadID = 0;
    sc_time lastTimestamp = scMaxTime;
    sc_time newTimestamp;

    for (auto it = readyCommands.cbegin(); it != readyCommands.cend(); it++)
    {
        newTimestamp = std::get<CommandTuple::Timestamp>(*it) +
                       memSpec.getCommandLength(std::get<CommandTuple::Command>(*it));
        newPayloadID =
            ControllerExtension::getChannelPayloadID(*std::get<CommandTuple::Payload>(*it));

        if (newTimestamp < lastTimestamp)
        {
            lastTimestamp = newTimestamp;
            lastPayloadID = newPayloadID;
            result = it;
        }
        else if ((newTimestamp == lastTimestamp) && (newPayloadID < lastPayloadID))
        {
            lastPayloadID = newPayloadID;
            result = it;
        }
    }

    if (result != readyCommands.cend() &&
        std::get<CommandTuple::Timestamp>(*result) == sc_time_stamp())
        return *result;
    return {Command::NOP, nullptr, scMaxTime};
}

CmdMuxOldestRasCas::CmdMuxOldestRasCas(const MemSpec& memSpec) : memSpec(memSpec)
{
    readyRasCommands.reserve(memSpec.banksPerChannel);
    readyCasCommands.reserve(memSpec.banksPerChannel);
    readyRasCasCommands.reserve(2);
}

CommandTuple::Type CmdMuxOldestRasCas::selectCommand(const ReadyCommands& readyCommands)
{
    readyRasCommands.clear();
    readyCasCommands.clear();

    for (auto it : readyCommands)
    {
        if (std::get<CommandTuple::Command>(it).isRasCommand())
            readyRasCommands.emplace_back(it);
        else
            readyCasCommands.emplace_back(it);
    }

    auto result = readyCommands.cend();
    auto resultRas = readyRasCommands.cend();
    auto resultCas = readyCasCommands.cend();

    uint64_t lastPayloadID = UINT64_MAX;
    uint64_t newPayloadID = 0;
    sc_time lastTimestamp = scMaxTime;
    sc_time newTimestamp;

    for (auto it = readyRasCommands.cbegin(); it != readyRasCommands.cend(); it++)
    {
        newTimestamp = std::get<CommandTuple::Timestamp>(*it) +
                       memSpec.getCommandLength(std::get<CommandTuple::Command>(*it));
        newPayloadID =
            ControllerExtension::getChannelPayloadID(*std::get<CommandTuple::Payload>(*it));

        if (newTimestamp < lastTimestamp)
        {
            lastTimestamp = newTimestamp;
            lastPayloadID = newPayloadID;
            resultRas = it;
        }
        else if ((newTimestamp == lastTimestamp) && (newPayloadID < lastPayloadID))
        {
            lastPayloadID = newPayloadID;
            resultRas = it;
        }
    }

    lastPayloadID = UINT64_MAX;
    lastTimestamp = scMaxTime;

    for (auto it = readyCasCommands.cbegin(); it != readyCasCommands.cend(); it++)
    {
        newTimestamp = std::get<CommandTuple::Timestamp>(*it) +
                       memSpec.getCommandLength(std::get<CommandTuple::Command>(*it));
        newPayloadID =
            ControllerExtension::getChannelPayloadID(*std::get<CommandTuple::Payload>(*it));

        if (newTimestamp < lastTimestamp)
        {
            lastTimestamp = newTimestamp;
            lastPayloadID = newPayloadID;
            resultCas = it;
        }
        else if ((newTimestamp == lastTimestamp) && (newPayloadID < lastPayloadID))
        {
            lastPayloadID = newPayloadID;
            resultCas = it;
        }
    }

    readyRasCasCommands.clear();

    if (resultRas != readyRasCommands.cend())
        readyRasCasCommands.emplace_back(*resultRas);
    if (resultCas != readyCasCommands.cend())
        readyRasCasCommands.emplace_back(*resultCas);

    lastPayloadID = UINT64_MAX;
    lastTimestamp = scMaxTime;

    for (auto it = readyRasCasCommands.cbegin(); it != readyRasCasCommands.cend(); it++)
    {
        newTimestamp = std::get<CommandTuple::Timestamp>(*it);
        newPayloadID =
            ControllerExtension::getChannelPayloadID(*std::get<CommandTuple::Payload>(*it));

        if (newTimestamp < lastTimestamp)
        {
            lastTimestamp = newTimestamp;
            lastPayloadID = newPayloadID;
            result = it;
        }
        else if ((newTimestamp == lastTimestamp) && (newPayloadID < lastPayloadID))
        {
            lastPayloadID = newPayloadID;
            result = it;
        }
    }

    if (result != readyCommands.cend() &&
        std::get<CommandTuple::Timestamp>(*result) == sc_time_stamp())
        return *result;
    return {Command::NOP, nullptr, scMaxTime};
}

} // namespace DRAMSys
