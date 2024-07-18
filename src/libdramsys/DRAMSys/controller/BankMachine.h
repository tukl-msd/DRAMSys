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

#ifndef BANKMACHINE_H
#define BANKMACHINE_H

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/controller/Command.h"
#include "DRAMSys/controller/ManagerIF.h"
#include "DRAMSys/controller/McConfig.h"
#include "DRAMSys/controller/checker/CheckerIF.h"
#include "DRAMSys/controller/scheduler/SchedulerIF.h"

#include <systemc>
#include <tlm>

namespace DRAMSys
{

class BankMachine : public ManagerIF
{
public:
    CommandTuple::Type getNextCommand() override;
    void update(Command command) override;
    void block();

    [[nodiscard]] Rank getRank() const;
    [[nodiscard]] BankGroup getBankGroup() const;
    [[nodiscard]] Bank getBank() const;
    [[nodiscard]] Row getOpenRow() const;
    [[nodiscard]] bool isIdle() const;
    [[nodiscard]] bool isActivated() const;
    [[nodiscard]] bool isPrecharged() const;
    [[nodiscard]] uint64_t getRefreshManagementCounter() const;

protected:
    enum class State
    {
        Precharged,
        Activated
    } state = State::Precharged;
    BankMachine(const McConfig& config, const MemSpec& memSpec, const SchedulerIF& scheduler, Bank bank);
    const MemSpec& memSpec;
    tlm::tlm_generic_payload* currentPayload = nullptr;
    const SchedulerIF& scheduler;
    Command nextCommand = Command::NOP;
    Row openRow = Row(0);
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
    BankMachineOpen(const McConfig& config, const MemSpec& memSpec, const SchedulerIF& scheduler, Bank bank);
    void evaluate() override;
};

class BankMachineClosed final : public BankMachine
{
public:
    BankMachineClosed(const McConfig& config, const MemSpec& memSpec, const SchedulerIF& scheduler, Bank bank);
    void evaluate() override;
};

class BankMachineOpenAdaptive final : public BankMachine
{
public:
    BankMachineOpenAdaptive(const McConfig& config, const MemSpec& memSpec, const SchedulerIF& scheduler, Bank bank);
    void evaluate() override;
};

class BankMachineClosedAdaptive final : public BankMachine
{
public:
    BankMachineClosedAdaptive(const McConfig& config, const MemSpec& memSpec, const SchedulerIF& scheduler, Bank bank);
    void evaluate() override;
};

} // namespace DRAMSys

#endif // BANKMACHINE_H
