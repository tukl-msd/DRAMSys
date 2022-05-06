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

#include <systemc>
#include <tlm>
#include "../common/dramExtensions.h"
#include "Command.h"
#include "scheduler/SchedulerIF.h"
#include "checker/CheckerIF.h"
#include "../configuration/memspec/MemSpec.h"
#include "../configuration/Configuration.h"

class BankMachine
{
public:
    virtual ~BankMachine() = default;
    virtual sc_core::sc_time start() = 0;
    CommandTuple::Type getNextCommand();
    void updateState(Command);
    void block();

    Rank getRank() const;
    BankGroup getBankGroup() const;
    Bank getBank() const;
    Row getOpenRow() const;
    bool isIdle() const;
    bool isActivated() const;
    bool isPrecharged() const;
    uint64_t getRefreshManagementCounter() const;

protected:
    enum class State {Precharged, Activated} state = State::Precharged;
    BankMachine(const Configuration& config, const SchedulerIF& scheduler, const CheckerIF& checker, Bank bank);
    const MemSpec& memSpec;
    tlm::tlm_generic_payload *currentPayload = nullptr;
    const SchedulerIF& scheduler;
    const CheckerIF& checker;
    Command nextCommand = Command::NOP;
    Row openRow;
    sc_core::sc_time timeToSchedule = sc_core::sc_max_time();
    const Bank bank;
    const BankGroup bankgroup;
    const Rank rank;
    bool blocked = false;
    bool sleeping = false;
    unsigned refreshManagementCounter = 0;
    const bool refreshManagement = false;
    bool keepTrans = false;
};

class BankMachineOpen final : public BankMachine
{
public:
    BankMachineOpen(const Configuration& config, const SchedulerIF& scheduler, const CheckerIF& checker, Bank bank);
    sc_core::sc_time start() override;
};

class BankMachineClosed final : public BankMachine
{
public:
    BankMachineClosed(const Configuration& config, const SchedulerIF& scheduler, const CheckerIF& checker, Bank bank);
    sc_core::sc_time start() override;
};

class BankMachineOpenAdaptive final : public BankMachine
{
public:
    BankMachineOpenAdaptive(const Configuration& config, const SchedulerIF& scheduler, const CheckerIF& checker,
                            Bank bank);
    sc_core::sc_time start() override;
};

class BankMachineClosedAdaptive final : public BankMachine
{
public:
    BankMachineClosedAdaptive(const Configuration& config, const SchedulerIF& scheduler, const CheckerIF& checker,
                              Bank bank);
    sc_core::sc_time start() override;
};

#endif // BANKMACHINE_H
