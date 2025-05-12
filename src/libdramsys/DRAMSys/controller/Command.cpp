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
 *    Lukas Steiner
 *    Marco Mörz
 */

#include "Command.h"

#include <array>

using namespace tlm;

namespace DRAMSys
{

bool phaseHasDataStrobe(tlm::tlm_phase phase)
{
    return (phase >= BEGIN_RD && phase <= BEGIN_MWRA);
}

bool isPowerDownEntryPhase(tlm::tlm_phase phase)
{
    return (phase >= BEGIN_PDNA && phase <= BEGIN_SREF);
}

bool isPowerDownExitPhase(tlm::tlm_phase phase)
{
    return (phase >= END_PDNA && phase <= END_SREF);
}

bool isFixedCommandPhase(tlm::tlm_phase phase)
{
    return (phase >= BEGIN_NOP && phase <= BEGIN_RFMAB);
}

bool isRefreshCommandPhase(tlm::tlm_phase phase)
{
    return (phase == BEGIN_REFPB || phase == BEGIN_REFP2B || phase == BEGIN_REFSB ||
            phase == BEGIN_REFAB || phase == BEGIN_RFMPB || phase == BEGIN_RFMP2B ||
            phase == BEGIN_RFMSB || phase == BEGIN_RFMAB);
}

Command::Command(Command::Type type) : type(type)
{
}

Command::Command(tlm_phase phase)
{
    assert(phase >= BEGIN_NOP && phase <= END_SREF); // TODO < END_ENUM && type >= 0
    static constexpr std::array<Command::Type, Command::Type::END_ENUM> commandOfPhase = {
        Command::NOP,    // 0
        Command::RD,     // 1
        Command::WR,     // 2
        Command::MWR,    // 3
        Command::RDA,    // 4
        Command::WRA,    // 5
        Command::MWRA,   // 6
        Command::ACT,    // 7
        Command::PREPB,  // 8
        Command::REFPB,  // 9
        Command::RFMPB,  // 10
        Command::REFP2B, // 11
        Command::RFMP2B, // 12
        Command::PRESB,  // 13
        Command::REFSB,  // 14
        Command::RFMSB,  // 15
        Command::PREAB,  // 16
        Command::REFAB,  // 17
        Command::RFMAB,  // 18
        Command::PDEA,   // 19
        Command::PDEP,   // 20
        Command::SREFEN, // 21
        Command::PDXA,   // 22
        Command::PDXP,   // 23
        Command::SREFEX  // 24
    };
    type = commandOfPhase[phase - BEGIN_NOP];
}

std::string Command::toString() const
{
    assert(type >= 0 && type < Command::END_ENUM); // TODO < END_ENUM && type >= 0
    static std::array<std::string, Command::Type::END_ENUM> stringOfCommand = {
        "NOP",    // 0
        "RD",     // 1
        "WR",     // 2
        "MWR",    // 3
        "RDA",    // 4
        "WRA",    // 5
        "MWRA",   // 6
        "ACT",    // 7
        "PREPB",  // 8
        "REFPB",  // 9
        "RFMPB",  // 10
        "REFP2B", // 11
        "RFMP2B", // 12
        "PRESB",  // 13
        "REFSB",  // 14
        "RFMSB",  // 15
        "PREAB",  // 16
        "REFAB",  // 17
        "RFMAB",  // 18
        "PDEA",   // 19
        "PDEP",   // 20
        "SREFEN", // 21
        "PDXA",   // 22
        "PDXP",   // 23
        "SREFEX"  // 24
    };
    return stringOfCommand[type];
}

unsigned Command::numberOfCommands()
{
    return Type::END_ENUM;
}

tlm_phase Command::toPhase() const
{
    assert(type >= 0 && type < Command::END_ENUM);
    static std::array<tlm_phase, Command::Type::END_ENUM> phaseOfCommand = {
        BEGIN_NOP,    // 0
        BEGIN_RD,     // 1
        BEGIN_WR,     // 2
        BEGIN_MWR,    // 3
        BEGIN_RDA,    // 4
        BEGIN_WRA,    // 5
        BEGIN_MWRA,   // 6
        BEGIN_ACT,    // 7
        BEGIN_PREPB,  // 8
        BEGIN_REFPB,  // 9
        BEGIN_RFMPB,  // 10
        BEGIN_REFP2B, // 11
        BEGIN_RFMP2B, // 12
        BEGIN_PRESB,  // 13
        BEGIN_REFSB,  // 14
        BEGIN_RFMSB,  // 15
        BEGIN_PREAB,  // 16
        BEGIN_REFAB,  // 17
        BEGIN_RFMAB,  // 18
        BEGIN_PDNA,   // 19
        BEGIN_PDNP,   // 20
        BEGIN_SREF,   // 21
        END_PDNA,     // 22
        END_PDNP,     // 23
        END_SREF      // 24
    };
    return phaseOfCommand[type];
}

DRAMPower::CmdType phaseToDRAMPowerCommand(tlm_phase phase)
{
    // TODO missing DSMEN, DSMEX
    assert(phase >= BEGIN_NOP && phase <= END_SREF);
    static std::array<DRAMPower::CmdType, Command::Type::END_ENUM> phaseOfCommand = {
        DRAMPower::CmdType::NOP,    // 0
        DRAMPower::CmdType::RD,     // 1
        DRAMPower::CmdType::WR,     // 2
        DRAMPower::CmdType::NOP,    // 3
        DRAMPower::CmdType::RDA,    // 4
        DRAMPower::CmdType::WRA,    // 5
        DRAMPower::CmdType::NOP,    // 6
        DRAMPower::CmdType::ACT,    // 7
        DRAMPower::CmdType::PRE,    // 8, PREPB
        DRAMPower::CmdType::REFB,   // 9, REFPB
        DRAMPower::CmdType::NOP,    // 10, RFMPB
        DRAMPower::CmdType::REFP2B, // 11, REFP2B
        DRAMPower::CmdType::NOP,    // 12, RFMP2B
        DRAMPower::CmdType::PRESB,  // 13, PRESB
        DRAMPower::CmdType::REFSB,  // 14, REFSB
        DRAMPower::CmdType::NOP,    // 15, RFMSB
        DRAMPower::CmdType::PREA,   // 16, PREAB
        DRAMPower::CmdType::REFA,   // 17, REFAB
        DRAMPower::CmdType::NOP,    // 18, RFMAB
        DRAMPower::CmdType::PDEA,   // 19
        DRAMPower::CmdType::PDEP,   // 20
        DRAMPower::CmdType::SREFEN, // 21
        DRAMPower::CmdType::PDXA,   // 22
        DRAMPower::CmdType::PDXP,   // 23
        DRAMPower::CmdType::SREFEX  // 24
    };
    return phaseOfCommand[phase - BEGIN_NOP];
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
    return (type <= Command::MWRA);
}

bool Command::isRasCommand() const
{
    assert(type >= Command::NOP && type <= Command::SREFEX);
    return (type >= Command::ACT);
}

} // namespace DRAMSys
