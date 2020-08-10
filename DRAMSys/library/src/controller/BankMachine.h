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

#ifndef BANKMACHINE_H
#define BANKMACHINE_H

#include <systemc.h>
#include <tlm.h>
#include <utility>
#include "../common/dramExtensions.h"
#include "Controller.h"
#include "Command.h"
#include "scheduler/SchedulerIF.h"
#include "checker/CheckerIF.h"

class SchedulerIF;
class CheckerIF;

enum class BmState
{
    Precharged,
    Activated
};

class BankMachine
{
public:
    virtual ~BankMachine() {}
    virtual sc_time start() = 0;
    std::tuple<Command, tlm::tlm_generic_payload *, sc_time> getNextCommand();
    void updateState(Command);
    void block();

    Rank getRank();
    BankGroup getBankGroup();
    Bank getBank();
    Row getOpenRow();
    BmState getState();
    bool isIdle();

protected:
    BankMachine(SchedulerIF *, CheckerIF *, Bank);
    tlm::tlm_generic_payload *currentPayload = nullptr;
    SchedulerIF *scheduler;
    CheckerIF *checker;
    Command nextCommand = Command::NOP;
    BmState currentState = BmState::Precharged;
    Row currentRow;
    sc_time timeToSchedule = sc_max_time();
    Rank rank = Rank(0);
    BankGroup bankgroup = BankGroup(0);
    Bank bank;
    bool blocked = false;
    bool sleeping = false;
};

class BankMachineOpen final : public BankMachine
{
public:
    BankMachineOpen(SchedulerIF *, CheckerIF *, Bank);
    sc_time start();
};

class BankMachineClosed final : public BankMachine
{
public:
    BankMachineClosed(SchedulerIF *, CheckerIF *, Bank);
    sc_time start();
};

class BankMachineOpenAdaptive final : public BankMachine
{
public:
    BankMachineOpenAdaptive(SchedulerIF *, CheckerIF *, Bank);
    sc_time start();
};

class BankMachineClosedAdaptive final : public BankMachine
{
public:
    BankMachineClosedAdaptive(SchedulerIF *, CheckerIF *, Bank);
    sc_time start();
};

#endif // BANKMACHINE_H
