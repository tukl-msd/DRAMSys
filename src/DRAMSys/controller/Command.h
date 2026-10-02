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
 *    Matthias Jung
 *    Lukas Steiner
 *    Marco Mörz
 */
#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <systemc>
#include <tlm>

#ifdef USE_DRAMPOWER
#include <DRAMPower/command/CmdType.h>
#endif

namespace DRAMSys
{

class Command
{
public:
    enum Type : uint8_t
    {
        NOP = 0, // 0
        RD,      // 1
        WR,      // 2
        MWR,     // 3
        RDA,     // 4
        WRA,     // 5
        MWRA,    // 6
        ACT,     // 7
        PREPB,   // 8
        REFPB,   // 9
        RFMPB,   // 10
        REFP2B,  // 11
        RFMP2B,  // 12
        REFDB,   // 13
        RFMDB,   // 14
        PRESB,   // 15
        REFSB,   // 16
        RFMSB,   // 17
        PREAB,   // 18
        REFAB,   // 19
        RFMAB,   // 20
        PDEA,    // 21
        PDEP,    // 22
        SREFEN,  // 23
        PDXA,    // 24
        PDXP,    // 25
        SREFEX,  // 26
        END_ENUM // 27, To mark the end of this enumeration
    };

private:
    Type type;

public:
    Command() = default;
    Command(Type type);

    [[nodiscard]] std::string toString() const;
    static unsigned numberOfCommands();
    [[nodiscard]] bool isBankCommand() const;
    [[nodiscard]] bool is2BankCommand() const;
    [[nodiscard]] bool isDualBankCommand() const;
    [[nodiscard]] bool isGroupCommand() const;
    [[nodiscard]] bool isRankCommand() const;
    [[nodiscard]] bool isCasCommand() const;
    [[nodiscard]] bool isRasCommand() const;

#ifdef USE_DRAMPOWER
    [[nodiscard]] DRAMPower::CmdType toDRAMPowerCommand() const;
#endif

    constexpr operator uint8_t() const { return type; }
};

struct ReadyCommand
{
    Command command;
    tlm::tlm_generic_payload *trans;
    sc_core::sc_time readyTime;
};

} // namespace DRAMSys

#endif // COMMAND_H
