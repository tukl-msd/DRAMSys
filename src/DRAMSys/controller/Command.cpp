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
    return (phase == BEGIN_REFPB || phase == BEGIN_REFP2B || phase == BEGIN_REFDB ||
        phase == BEGIN_REFSB || phase == BEGIN_REFAB || phase == BEGIN_RFMPB ||
        phase == BEGIN_RFMP2B || phase == BEGIN_RFMDB || phase == BEGIN_RFMSB ||
        phase == BEGIN_RFMAB);
}

Command::Command(Type type) : type(type)
{
}

Command::Command(tlm_phase phase)
{
    assert(phase >= BEGIN_NOP && phase <= END_SREF); // TODO < END_ENUM && type >= 0
    static constexpr std::array<Type, END_ENUM> commandOfPhase = {
        NOP,    // 0
        RD,     // 1
        WR,     // 2
        MWR,    // 3
        RDA,    // 4
        WRA,    // 5
        MWRA,   // 6
        ACT,    // 7
        PREPB,  // 8
        REFPB,  // 9
        RFMPB,  // 10
        REFP2B, // 11
        RFMP2B, // 12
        REFDB,  // 13
        RFMDB,  // 14
        PRESB,  // 15
        REFSB,  // 16
        RFMSB,  // 17
        PREAB,  // 18
        REFAB,  // 19
        RFMAB,  // 20
        PDEA,   // 21
        PDEP,   // 22
        SREFEN, // 23
        PDXA,   // 24
        PDXP,   // 25
        SREFEX  // 26
    };
    type = commandOfPhase[phase - BEGIN_NOP];
}

std::string Command::toString() const
{
    assert(type >= 0 && type < END_ENUM); // TODO < END_ENUM && type >= 0
    static std::array<std::string, END_ENUM> stringOfCommand = {
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
        "REFDB",  // 13
        "RFMDB",  // 14
        "PRESB",  // 15
        "REFSB",  // 16
        "RFMSB",  // 17
        "PREAB",  // 18
        "REFAB",  // 19
        "RFMAB",  // 20
        "PDEA",   // 21
        "PDEP",   // 22
        "SREFEN", // 23
        "PDXA",   // 24
        "PDXP",   // 25
        "SREFEX"  // 26
    };
    return stringOfCommand[type];
}

unsigned Command::numberOfCommands()
{
    return END_ENUM;
}

tlm_phase Command::toPhase() const
{
    assert(type >= 0 && type < END_ENUM);
    static std::array<tlm_phase, END_ENUM> phaseOfCommand = {
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
        BEGIN_REFDB,  // 13
        BEGIN_RFMDB,  // 14
        BEGIN_PRESB,  // 15
        BEGIN_REFSB,  // 16
        BEGIN_RFMSB,  // 17
        BEGIN_PREAB,  // 18
        BEGIN_REFAB,  // 19
        BEGIN_RFMAB,  // 20
        BEGIN_PDNA,   // 21
        BEGIN_PDNP,   // 22
        BEGIN_SREF,   // 23
        END_PDNA,     // 24
        END_PDNP,     // 25
        END_SREF      // 26
    };
    return phaseOfCommand[type];
}

bool Command::isBankCommand() const
{
    assert(type >= NOP && type <= SREFEX);
    return (type <= RFMPB);
}

bool Command::is2BankCommand() const
{
    assert(type >= NOP && type <= SREFEX);
    return (type >= REFP2B && type <= RFMP2B);
}

bool Command::isDualBankCommand() const
{
    assert(type >= NOP && type <= SREFEX);
    return (type >= REFDB && type <= RFMDB);
}

bool Command::isGroupCommand() const
{
    assert(type >= NOP && type <= SREFEX);
    return (type >= PRESB && type <= RFMSB);
}

bool Command::isRankCommand() const
{
    assert(type >= NOP && type <= SREFEX);
    return (type >= PREAB);
}

bool Command::isCasCommand() const
{
    assert(type >= NOP && type <= SREFEX);
    return (type <= MWRA);
}

bool Command::isRasCommand() const
{
    assert(type >= NOP && type <= SREFEX);
    return (type >= ACT);
}

} // namespace DRAMSys
