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

#include "PowerDownManagerStaggered.h"

#include "DRAMSys/controller/BankMachine.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

PowerDownManagerStaggered::PowerDownManagerStaggered(
    ControllerVector<Bank, BankMachine*>& bankMachinesOnRank, Rank rank) :
    bankMachinesOnRank(bankMachinesOnRank)
{
    setUpDummy(powerDownPayload, UINT64_MAX - 1, rank);
}

void PowerDownManagerStaggered::triggerEntry()
{
    controllerIdle = true;

    if (state == State::Idle)
        entryTriggered = true;
}

void PowerDownManagerStaggered::triggerExit()
{
    controllerIdle = false;
    enterSelfRefresh = false;
    entryTriggered = false;

    if (state != State::Idle)
        exitTriggered = true;
}

void PowerDownManagerStaggered::triggerInterruption()
{
    entryTriggered = false;

    if (state != State::Idle)
        exitTriggered = true;
}

CommandTuple::Type PowerDownManagerStaggered::getNextCommand()
{
    return {nextCommand, &powerDownPayload, SC_ZERO_TIME};
}

void PowerDownManagerStaggered::evaluate()
{
    nextCommand = Command::NOP;

    if (exitTriggered)
    {
        if (state == State::ActivePdn)
            nextCommand = Command::PDXA;
        else if (state == State::PrechargePdn)
            nextCommand = Command::PDXP;
        else if (state == State::SelfRefresh)
            nextCommand = Command::SREFEX;
        else if (state == State::ExtraRefresh)
            nextCommand = Command::REFAB;
    }
    else if (entryTriggered)
    {
        nextCommand = Command::PDEP;
        for (auto* it : bankMachinesOnRank)
        {
            if (it->isActivated())
            {
                nextCommand = Command::PDEA;
                break;
            }
        }
    }
    else if (enterSelfRefresh)
    {
        nextCommand = Command::SREFEN;
    }
}

void PowerDownManagerStaggered::update(Command command)
{
    switch (command)
    {
    case Command::PDEA:
        state = State::ActivePdn;
        entryTriggered = false;
        break;
    case Command::PDEP:
        state = State::PrechargePdn;
        entryTriggered = false;
        break;
    case Command::SREFEN:
        state = State::SelfRefresh;
        entryTriggered = false;
        enterSelfRefresh = false;
        break;
    case Command::PDXA:
        state = State::Idle;
        exitTriggered = false;
        break;
    case Command::PDXP:
        state = State::Idle;
        exitTriggered = false;
        if (controllerIdle)
            enterSelfRefresh = true;
        break;
    case Command::SREFEX:
        state = State::ExtraRefresh;
        break;
    case Command::REFAB:
        if (state == State::ExtraRefresh)
        {
            state = State::Idle;
            exitTriggered = false;
        }
        else if (controllerIdle)
            entryTriggered = true;
        break;
    case Command::REFPB:
    case Command::REFP2B:
    case Command::REFSB:
        if (controllerIdle)
            entryTriggered = true;
        break;
    default:
        break;
    }
}

} // namespace DRAMSys
