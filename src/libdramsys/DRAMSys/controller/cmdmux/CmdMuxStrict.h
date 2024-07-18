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

#ifndef CMDMUXSTRICT_H
#define CMDMUXSTRICT_H

#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/controller/cmdmux/CmdMuxIF.h"

namespace DRAMSys
{

class CmdMuxStrict : public CmdMuxIF
{
public:
    explicit CmdMuxStrict(const MemSpec& memSpec);
    CommandTuple::Type selectCommand(const ReadyCommands& readyCommands) override;

private:
    uint64_t nextPayloadID = 1;
    const MemSpec& memSpec;
    const sc_core::sc_time scMaxTime = sc_core::sc_max_time();
};

class CmdMuxStrictRasCas : public CmdMuxIF
{
public:
    explicit CmdMuxStrictRasCas(const MemSpec& memSpec);
    CommandTuple::Type selectCommand(const ReadyCommands& readyCommands) override;

private:
    uint64_t nextPayloadID = 1;
    const MemSpec& memSpec;
    ReadyCommands readyRasCommands;
    ReadyCommands readyCasCommands;
    ReadyCommands readyRasCasCommands;
    const sc_core::sc_time scMaxTime = sc_core::sc_max_time();
};

} // namespace DRAMSys

#endif // CMDMUXSTRICT_H
