/*
 * Copyright (c) 2019, Technische Universität Kaiserslautern
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
#include "../../common/utils.h"

using namespace tlm;

PowerDownManagerStaggered::PowerDownManagerStaggered(Rank rank, CheckerIF *checker)
    : rank(rank), checker(checker)
{
    setUpDummy(powerDownPayload, UINT64_MAX, rank);
}

void PowerDownManagerStaggered::triggerEntry()
{
    controllerIdle = true;

    if (state == PdmState::Idle)
        entryTriggered = true;
}

void PowerDownManagerStaggered::triggerExit()
{
    controllerIdle = false;
    enterSelfRefresh = false;
    entryTriggered = false;

    if (state != PdmState::Idle)
        exitTriggered = true;
}

void PowerDownManagerStaggered::triggerInterruption()
{
    entryTriggered = false;

    if (state != PdmState::Idle)
        exitTriggered = true;
}

std::tuple<Command, tlm_generic_payload *, sc_time> PowerDownManagerStaggered::getNextCommand()
{
    return std::tuple<Command, tlm_generic_payload *, sc_time>(nextCommand, &powerDownPayload, timeToSchedule);
}

sc_time PowerDownManagerStaggered::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (exitTriggered)
    {
        if (state == PdmState::ActivePdn)
            nextCommand = Command::PDXA;
        else if (state == PdmState::PrechargePdn)
            nextCommand = Command::PDXP;
        else if (state == PdmState::SelfRefresh)
            nextCommand = Command::SREFEX;
        else if (state == PdmState::ExtraRefresh)
            nextCommand = Command::REFA;

        timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, BankGroup(0), Bank(0));
    }
    else if (entryTriggered)
    {
        if (activatedBanks != 0)
            nextCommand = Command::PDEA;
        else
            nextCommand = Command::PDEP;

        timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, BankGroup(0), Bank(0));
    }
    else if (enterSelfRefresh)
    {
        nextCommand = Command::SREFEN;
        timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, BankGroup(0), Bank(0));
    }

    return timeToSchedule;
}

void PowerDownManagerStaggered::updateState(Command command)
{
    switch (command)
    {
    case Command::ACT:
        activatedBanks++;
        break;
    case Command::PRE:
        activatedBanks--;
        break;
    case Command::PREA:
        activatedBanks = 0;
        break;
    case Command::PDEA:
        state = PdmState::ActivePdn;
        entryTriggered = false;
        break;
    case Command::PDEP:
        state = PdmState::PrechargePdn;
        entryTriggered = false;
        break;
    case Command::SREFEN:
        state = PdmState::SelfRefresh;
        entryTriggered = false;
        enterSelfRefresh = false;
        break;
    case Command::PDXA:
        state = PdmState::Idle;
        exitTriggered = false;
        break;
    case Command::PDXP:
        state = PdmState::Idle;
        exitTriggered = false;
        if (controllerIdle)
            enterSelfRefresh = true;
        break;
    case Command::SREFEX:
        state = PdmState::ExtraRefresh;
        break;
    case Command::REFA:
        if (state == PdmState::ExtraRefresh)
        {
            state = PdmState::Idle;
            exitTriggered = false;
        }
        else if (controllerIdle)
            entryTriggered = true;
        break;
    case Command::REFB:
        if (controllerIdle)
            entryTriggered = true;
        break;
    }
}
