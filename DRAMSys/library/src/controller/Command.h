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
 *    Matthias Jung
 */
#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <array>
#include <systemc.h>
#include <tlm.h>
#include "../common/third_party/DRAMPower/src/MemCommand.h"

// DO NOT CHANGE THE ORDER!
DECLARE_EXTENDED_PHASE(BEGIN_RD);    // 5
DECLARE_EXTENDED_PHASE(BEGIN_WR);    // 6
DECLARE_EXTENDED_PHASE(BEGIN_RDA);   // 7
DECLARE_EXTENDED_PHASE(BEGIN_WRA);   // 8
DECLARE_EXTENDED_PHASE(BEGIN_ACT);   // 9
DECLARE_EXTENDED_PHASE(BEGIN_PRE);   // 10
DECLARE_EXTENDED_PHASE(BEGIN_REFB);  // 11
DECLARE_EXTENDED_PHASE(BEGIN_PRESB); // 12
DECLARE_EXTENDED_PHASE(BEGIN_REFSB); // 13
DECLARE_EXTENDED_PHASE(BEGIN_PREA);  // 14
DECLARE_EXTENDED_PHASE(BEGIN_REFA);  // 15
DECLARE_EXTENDED_PHASE(BEGIN_PDNA);  // 16
DECLARE_EXTENDED_PHASE(END_PDNA);    // 17
DECLARE_EXTENDED_PHASE(BEGIN_PDNP);  // 18
DECLARE_EXTENDED_PHASE(END_PDNP);    // 19
DECLARE_EXTENDED_PHASE(BEGIN_SREF);  // 20
DECLARE_EXTENDED_PHASE(END_SREF);    // 21

DECLARE_EXTENDED_PHASE(END_RD);      // 22
DECLARE_EXTENDED_PHASE(END_WR);      // 23
DECLARE_EXTENDED_PHASE(END_RDA);     // 24
DECLARE_EXTENDED_PHASE(END_WRA);     // 25
DECLARE_EXTENDED_PHASE(END_ACT);     // 26
DECLARE_EXTENDED_PHASE(END_PRE);     // 27
DECLARE_EXTENDED_PHASE(END_REFB);    // 28
DECLARE_EXTENDED_PHASE(END_PRESB);   // 29
DECLARE_EXTENDED_PHASE(END_REFSB);   // 30
DECLARE_EXTENDED_PHASE(END_PREA);    // 31
DECLARE_EXTENDED_PHASE(END_REFA);    // 32

enum Command
{
    NOP,
    RD,
    WR,
    RDA,
    WRA,
    ACT,
    PRE,
    REFB,
    PRESB,
    REFSB,
    PREA,
    REFA,
    PDEA,
    PDXA,
    PDEP,
    PDXP,
    SREFEN,
    SREFEX
};

std::string commandToString(Command);
tlm::tlm_phase commandToPhase(Command);
Command phaseToCommand(tlm::tlm_phase);
DRAMPower::MemCommand::cmds phaseToDRAMPowerCommand(tlm::tlm_phase);
bool phaseNeedsEnd(tlm::tlm_phase);
tlm::tlm_phase getEndPhase(tlm::tlm_phase);
unsigned numberOfCommands();
bool isBankCommand(Command);
bool isGroupCommand(Command);
bool isRankCommand(Command);
bool isCasCommand(Command);
bool isRasCommand(Command);

struct CommandTuple 
{ 
    using Type = std::tuple<::Command, tlm::tlm_generic_payload *, sc_time>;
    enum Accessor 
    {
        Command = 0,
        Payload = 1,
        Timestamp = 2
    };
};

using ReadyCommands = std::vector<CommandTuple::Type>;

#endif // COMMAND_H
