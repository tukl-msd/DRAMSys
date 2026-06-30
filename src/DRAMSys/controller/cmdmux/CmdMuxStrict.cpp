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

#include "CmdMuxStrict.h"

#include <systemc>

using namespace sc_core;

namespace DRAMSys
{

CmdMuxStrict::CmdMuxStrict(const MemSpec& memSpec) : memSpec(memSpec)
{
}

std::optional<ReadyCommand> CmdMuxStrict::selectCommand(const ReadyCommands& readyCommands) const
{
    auto result = readyCommands.cend();
    uint64_t lastPayloadID = UINT64_MAX;
    uint64_t newPayloadID = 0;
    sc_time lastTimestamp = sc_max_time();
    sc_time newTimestamp;

    for (auto it = readyCommands.cbegin(); it != readyCommands.cend(); it++)
    {
        if (it->command.isCasCommand() && newPayloadID != nextPayloadID)
            continue;

        newTimestamp = it->readyTime + memSpec.getCommandLength(it->command);
        newPayloadID = ControllerExtension::getChannelPayloadID(*it->trans);

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

    if (result == readyCommands.cend())
        return std::nullopt;

    return *result;
}

void CmdMuxStrict::update(Command command)
{
    if (command.isCasCommand())
        nextPayloadID++;
}

CmdMuxStrictRasCas::CmdMuxStrictRasCas(const MemSpec& memSpec) : memSpec(memSpec)
{
}

std::optional<ReadyCommand>
CmdMuxStrictRasCas::selectCommand(const ReadyCommands& readyCommands) const
{
    ReadyCommands readyRasCommands;
    ReadyCommands readyCasCommands;

    for (auto it : readyCommands)
    {
        if (it.command.isRasCommand())
            readyRasCommands.emplace_back(it);
        else
            readyCasCommands.emplace_back(it);
    }

    uint64_t lastPayloadID = UINT64_MAX;
    uint64_t newPayloadID = 0;
    sc_time lastTimestamp = sc_max_time();
    sc_time newTimestamp;

    auto resultRas = readyRasCommands.cend();

    for (auto it = readyRasCommands.cbegin(); it != readyRasCommands.cend(); it++)
    {
        newTimestamp = it->readyTime + memSpec.getCommandLength(it->command);
        newPayloadID = ControllerExtension::getChannelPayloadID(*it->trans);

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

    auto resultCas = readyCasCommands.cend();

    for (auto it = readyCasCommands.cbegin(); it != readyCasCommands.cend(); it++)
    {
        newPayloadID = ControllerExtension::getChannelPayloadID(*it->trans);

        if (newPayloadID == nextPayloadID)
        {
            resultCas = it;
            break;
        }
    }

    ReadyCommands readyRasCasCommands;

    if (resultRas != readyRasCommands.cend())
        readyRasCasCommands.emplace_back(*resultRas);
    if (resultCas != readyCasCommands.cend())
        readyRasCasCommands.emplace_back(*resultCas);

    lastPayloadID = UINT64_MAX;
    lastTimestamp = sc_max_time();

    auto result = readyCommands.cend();

    for (auto it = readyRasCasCommands.cbegin(); it != readyRasCasCommands.cend(); it++)
    {
        newTimestamp = it->readyTime;
        newPayloadID = ControllerExtension::getChannelPayloadID(*it->trans);

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

    if (result == readyCommands.cend())
        return std::nullopt;

    return *result;
}

void CmdMuxStrictRasCas::update(Command command)
{
    if (command.isCasCommand())
        nextPayloadID++;
}

} // namespace DRAMSys
