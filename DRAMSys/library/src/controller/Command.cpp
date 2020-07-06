/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
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
 */

#include "Command.h"
#include <systemc.h>

using namespace tlm;
using namespace DRAMPower;

std::string commandToString(Command command)
{  
    assert(command >= Command::NOP && command <= Command::SREFEX);
    static std::array<std::string, 16> stringOfCommand =
            {"NOP",
            "RD",
            "WR",
            "RDA",
            "WRA",
            "PRE",
            "ACT",
            "REFB",
            "PREA",
            "REFA",
            "PDEA",
            "PDXA",
            "PDEP",
            "PDXP",
            "SREFEN",
            "SREFEX"};
    return stringOfCommand[command];
}

unsigned numberOfCommands()
{
    return 16;
}

tlm_phase commandToPhase(Command command)
{
    assert(command >= Command::NOP && command <= Command::SREFEX);
    static std::array<tlm_phase, 16> phaseOfCommand =
            {UNINITIALIZED_PHASE,
            BEGIN_RD,
            BEGIN_WR,
            BEGIN_RDA,
            BEGIN_WRA,
            BEGIN_PRE,
            BEGIN_ACT,
            BEGIN_REFB,
            BEGIN_PREA,
            BEGIN_REFA,
            BEGIN_PDNA,
            END_PDNA,
            BEGIN_PDNP,
            END_PDNP,
            BEGIN_SREF,
            END_SREF};
    return phaseOfCommand[command];
}

Command phaseToCommand(tlm_phase phase)
{
    assert(phase >= BEGIN_RD && phase <= END_SREF);
    static std::array<Command, 16> commandOfPhase =
            {Command::RD,
            Command::WR,
            Command::RDA,
            Command::WRA,
            Command::PRE,
            Command::ACT,
            Command::REFB,
            Command::PREA,
            Command::REFA,
            Command::PDEA,
            Command::PDXA,
            Command::PDEP,
            Command::PDXP,
            Command::SREFEN,
            Command::SREFEX};
    return commandOfPhase[phase - BEGIN_RD];
}

MemCommand::cmds phaseToDRAMPowerCommand(tlm_phase phase)
{
    assert(phase >= BEGIN_RD && phase <= END_SREF);
    static std::array<MemCommand::cmds, 16> phaseOfCommand =
            {MemCommand::RD,
            MemCommand::WR,
            MemCommand::RDA,
            MemCommand::WRA,
            MemCommand::PRE,
            MemCommand::ACT,
            MemCommand::REFB,
            MemCommand::PREA,
            MemCommand::REF,
            MemCommand::PDN_S_ACT,
            MemCommand::PUP_ACT,
            MemCommand::PDN_S_PRE,
            MemCommand::PUP_PRE,
            MemCommand::SREN,
            MemCommand::SREX};
    return phaseOfCommand[phase - BEGIN_RD];
}

bool phaseNeedsEnd(tlm_phase phase)
{
    return (phase >= BEGIN_RD && phase <= BEGIN_REFA);
}

tlm_phase getEndPhase(tlm_phase phase)
{
    assert(phase >= BEGIN_RD && phase <= BEGIN_REFA);
    return (phase + 15);
}

bool isBankCommand(Command command)
{
    assert(command >= Command::NOP && command <= Command::SREFEX);
    return (command <= Command::REFB);
}

bool isRankCommand(Command command)
{
    assert(command >= Command::NOP && command <= Command::SREFEX);
    return (command >= Command::PREA);
}

bool isCasCommand(Command command)
{
    assert(command >= Command::NOP && command <= Command::SREFEX);
    return (command <= Command::WRA);
}

bool isRasCommand(Command command)
{
    assert(command >= Command::NOP && command <= Command::SREFEX);
    return (command >= Command::PRE);
}
