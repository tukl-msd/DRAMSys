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
 *    Lukas Steiner
 */

#include <array>
#include "Command.h"

using namespace tlm;
using namespace DRAMPower;

Command::Command(Command::Type type) : type(type) {}

Command::Command(tlm_phase phase)
{
    assert(phase >= BEGIN_NOP && phase <= END_SREF);
    static constexpr std::array<Command::Type, Command::Type::END_ENUM> commandOfPhase =
            {
                    Command::NOP,    // 0
                    Command::RD,     // 1
                    Command::WR,     // 2
                    Command::RDA,    // 3
                    Command::WRA,    // 4
                    Command::ACT,    // 5
                    Command::PREPB,  // 6
                    Command::REFPB,  // 7
                    Command::RFMPB,  // 8
                    Command::REFP2B, // 9
                    Command::RFMP2B, // 10
                    Command::PRESB,  // 11
                    Command::REFSB,  // 12
                    Command::RFMSB,  // 13
                    Command::PREAB,  // 14
                    Command::REFAB,  // 15
                    Command::RFMAB,  // 16
                    Command::PDEA,   // 17
                    Command::PDEP,   // 18
                    Command::SREFEN, // 19
                    Command::PDXA,   // 20
                    Command::PDXP,   // 21
                    Command::SREFEX  // 22
            };
    type = commandOfPhase[phase - BEGIN_NOP];
}

std::string Command::toString() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    static std::array<std::string, Command::Type::END_ENUM> stringOfCommand =
            {
                    "NOP",      // 0
                    "RD",       // 1
                    "WR",       // 2
                    "RDA",      // 3
                    "WRA",      // 4
                    "ACT",      // 5
                    "PREPB",    // 6
                    "REFPB",    // 7
                    "RFMPB",    // 8
                    "REFP2B",   // 9
                    "RFMP2B",   // 10
                    "PRESB",    // 11
                    "REFSB",    // 12
                    "RFMSB",    // 13
                    "PREAB",    // 14
                    "REFAB",    // 15
                    "RFMAB",    // 16
                    "PDEA",     // 17
                    "PDEP",     // 18
                    "SREFEN",   // 19
                    "PDXA",     // 20
                    "PDXP",     // 21
                    "SREFEX"    // 22
            };
    return stringOfCommand[type];
}

unsigned Command::numberOfCommands()
{
    return Type::END_ENUM;
}

tlm_phase Command::toPhase() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    static std::array<tlm_phase, Command::Type::END_ENUM> phaseOfCommand =
            {
                    BEGIN_NOP,              // 0
                    BEGIN_RD,               // 1
                    BEGIN_WR,               // 2
                    BEGIN_RDA,              // 3
                    BEGIN_WRA,              // 4
                    BEGIN_ACT,              // 5
                    BEGIN_PREPB,            // 6
                    BEGIN_REFPB,            // 7
                    BEGIN_RFMPB,            // 8
                    BEGIN_REFP2B,           // 9
                    BEGIN_RFMP2B,           // 10
                    BEGIN_PRESB,            // 11
                    BEGIN_REFSB,            // 12
                    BEGIN_RFMSB,            // 13
                    BEGIN_PREAB,            // 14
                    BEGIN_REFAB,            // 15
                    BEGIN_RFMAB,            // 16
                    BEGIN_PDNA,             // 17
                    BEGIN_PDNP,             // 18
                    BEGIN_SREF,             // 19
                    END_PDNA,               // 20
                    END_PDNP,               // 21
                    END_SREF                // 22
            };
    return phaseOfCommand[type];
}

MemCommand::cmds phaseToDRAMPowerCommand(tlm_phase phase)
{
    // TODO: add correct phases when DRAMPower supports DDR5 same bank refresh
    assert(phase >= BEGIN_NOP && phase <= END_SREF);
    static std::array<MemCommand::cmds, Command::Type::END_ENUM> phaseOfCommand =
            {
                    MemCommand::NOP,        // 0
                    MemCommand::RD,         // 1
                    MemCommand::WR,         // 2
                    MemCommand::RDA,        // 3
                    MemCommand::WRA,        // 4
                    MemCommand::ACT,        // 5
                    MemCommand::PRE,        // 6, PREPB
                    MemCommand::REFB,       // 7, REFPB
                    MemCommand::NOP,        // 8, RFMPB
                    MemCommand::NOP,        // 9, REFP2B
                    MemCommand::NOP,        // 10, RFMP2B
                    MemCommand::NOP,        // 11, PRESB
                    MemCommand::NOP,        // 12, REFSB
                    MemCommand::NOP,        // 13, RFMSB
                    MemCommand::PREA,       // 14, PREAB
                    MemCommand::REF,        // 15, REFAB
                    MemCommand::NOP,        // 16, RFMAB
                    MemCommand::PDN_S_ACT,  // 17
                    MemCommand::PDN_S_PRE,  // 18
                    MemCommand::SREN,       // 19
                    MemCommand::PUP_ACT,    // 20
                    MemCommand::PUP_PRE,    // 21
                    MemCommand::SREX        // 22
            };
    return phaseOfCommand[phase - BEGIN_NOP];
}

bool phaseNeedsEnd(tlm_phase phase)
{
    return (phase >= BEGIN_NOP && phase <= BEGIN_RFMAB);
}

tlm_phase getEndPhase(tlm_phase phase)
{
    assert(phase >= BEGIN_NOP && phase <= BEGIN_RFMAB);
    return (phase + Command::Type::END_ENUM);
}

bool Command::isBankCommand() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    return (type <= Command::RFMPB);
}

bool Command::is2BankCommand() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    return (type >= Command::REFP2B && type <= Command::RFMP2B);
}

bool Command::isGroupCommand() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    return (type >= Command::PRESB && type <= Command::RFMSB);
}

bool Command::isRankCommand() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    return (type >= Command::PREAB);
}

bool Command::isCasCommand() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    return (type <= Command::WRA);
}

bool Command::isRasCommand() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    return (type >= Command::ACT);
}
