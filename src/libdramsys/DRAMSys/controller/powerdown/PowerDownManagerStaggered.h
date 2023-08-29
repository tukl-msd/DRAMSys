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

#ifndef POWERDOWNMANAGERSTAGGERED_H
#define POWERDOWNMANAGERSTAGGERED_H

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/controller/checker/CheckerIF.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerIF.h"

#include <systemc>

namespace DRAMSys
{

class BankMachine;

class PowerDownManagerStaggered final : public PowerDownManagerIF
{
public:
    PowerDownManagerStaggered(ControllerVector<Bank, BankMachine*>& bankMachinesOnRank, Rank rank);

    void triggerEntry() override;
    void triggerExit() override;
    void triggerInterruption() override;

    CommandTuple::Type getNextCommand() override;
    void update(Command command) override;
    void evaluate() override;

private:
    enum class State
    {
        Idle,
        ActivePdn,
        PrechargePdn,
        SelfRefresh,
        ExtraRefresh
    } state = State::Idle;
    tlm::tlm_generic_payload powerDownPayload;
    ControllerVector<Bank, BankMachine*>& bankMachinesOnRank;
    Command nextCommand = Command::NOP;

    bool controllerIdle = true;
    bool entryTriggered = true;
    bool exitTriggered = false;
    bool enterSelfRefresh = false;
};

} // namespace DRAMSys

#endif // POWERDOWNMANAGERSTAGGERED_H
