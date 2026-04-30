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
        "SREFEX", // 26
    };
    return stringOfCommand[type];
}

unsigned Command::numberOfCommands()
{
    return END_ENUM;
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

[[nodiscard]] DRAMPower::CmdType Command::toDRAMPowerCommand() const
{
    static std::array<DRAMPower::CmdType, Command::Type::END_ENUM> lut = {
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
        DRAMPower::CmdType::REFDB,  // 13, REFDB
        DRAMPower::CmdType::NOP,    // 14, RFMDB
        DRAMPower::CmdType::PRESB,  // 15, PRESB
        DRAMPower::CmdType::REFSB,  // 16, REFSB
        DRAMPower::CmdType::NOP,    // 17, RFMSB
        DRAMPower::CmdType::PREA,   // 18, PREAB
        DRAMPower::CmdType::REFA,   // 19, REFAB
        DRAMPower::CmdType::NOP,    // 20, RFMAB
        DRAMPower::CmdType::PDEA,   // 21
        DRAMPower::CmdType::PDEP,   // 22
        DRAMPower::CmdType::SREFEN, // 23
        DRAMPower::CmdType::PDXA,   // 24
        DRAMPower::CmdType::PDXP,   // 25
        DRAMPower::CmdType::SREFEX, // 26
    };

    return lut[this->type];
}

} // namespace DRAMSys
