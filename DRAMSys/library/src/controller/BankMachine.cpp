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

#include "BankMachine.h"
#include "../configuration/Configuration.h"

using namespace tlm;

BankMachine::BankMachine(SchedulerIF *scheduler, CheckerIF *checker, Bank bank)
    : scheduler(scheduler), checker(checker), bank(bank)
{
    const MemSpec *memSpec = Configuration::getInstance().memSpec;
    rank = Rank(bank.ID() / memSpec->banksPerRank);
    bankgroup = BankGroup(bank.ID() / memSpec->banksPerGroup);
}

CommandTuple::Type BankMachine::getNextCommand()
{
    return CommandTuple::Type(nextCommand, currentPayload, 
            std::max(timeToSchedule, sc_time_stamp()));
}

void BankMachine::updateState(Command command)
{
    switch (command)
    {
    case Command::ACT:
        state = State::Activated;
        openRow = DramExtension::getRow(currentPayload);
        break;
    case Command::PRE: case Command::PREA: case Command::PRESB:
        state = State::Precharged;
        break;
    case Command::RD: case Command::WR:
        currentPayload = nullptr;
        break;
    case Command::RDA: case Command::WRA:
        state = State::Precharged;
        currentPayload = nullptr;
        break;
    case Command::PDEA: case Command::PDEP: case Command::SREFEN:
        sleeping = true;
        break;
    case Command::REFA: case Command::REFB: case Command::REFSB:
        sleeping = false;
        blocked = false;
        break;
    case Command::PDXA: case Command::PDXP:
        sleeping = false;
        break;
    default:
        break;
    }
}

void BankMachine::block()
{
    blocked = true;
}

Rank BankMachine::getRank()
{
    return rank;
}

BankGroup BankMachine::getBankGroup()
{
    return bankgroup;
}

Bank BankMachine::getBank()
{
    return bank;
}

Row BankMachine::getOpenRow()
{
    return openRow;
}

BankMachine::State BankMachine::getState()
{
    return state;
}

bool BankMachine::isIdle()
{
    return (currentPayload == nullptr);
}

BankMachineOpen::BankMachineOpen(SchedulerIF *scheduler, CheckerIF *checker, Bank bank)
    : BankMachine(scheduler, checker, bank) {}

sc_time BankMachineOpen::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (!(sleeping || blocked))
    {
        currentPayload = scheduler->getNextRequest(this);
        if (currentPayload != nullptr)
        {
            if (state == State::Precharged) // bank precharged
                nextCommand = Command::ACT;
            else if (state == State::Activated)
            {
                if (DramExtension::getRow(currentPayload) == openRow) // row hit
                {
                    if (currentPayload->get_command() == TLM_READ_COMMAND)
                        nextCommand = Command::RD;
                    else if (currentPayload->get_command() == TLM_WRITE_COMMAND)
                        nextCommand = Command::WR;
                    else
                        SC_REPORT_FATAL("BankMachine", "Wrong TLM command");
                }
                else // row miss
                    nextCommand = Command::PRE;
            }
            timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, bankgroup, bank);
        }
    }
    return timeToSchedule;
}

BankMachineClosed::BankMachineClosed(SchedulerIF *scheduler, CheckerIF *checker, Bank bank)
    : BankMachine(scheduler, checker, bank) {}

sc_time BankMachineClosed::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (!(sleeping || blocked))
    {
        currentPayload = scheduler->getNextRequest(this);
        if (currentPayload != nullptr)
        {
            if (state == State::Precharged) // bank precharged
                nextCommand = Command::ACT;
            else if (state == State::Activated)
            {
                if (currentPayload->get_command() == TLM_READ_COMMAND)
                    nextCommand = Command::RDA;
                else if (currentPayload->get_command() == TLM_WRITE_COMMAND)
                    nextCommand = Command::WRA;
                else
                    SC_REPORT_FATAL("BankMachine", "Wrong TLM command");
            }
            timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, bankgroup, bank);
        }
    }
    return timeToSchedule;
}

BankMachineOpenAdaptive::BankMachineOpenAdaptive(SchedulerIF *scheduler, CheckerIF *checker, Bank bank)
    : BankMachine(scheduler, checker, bank) {}

sc_time BankMachineOpenAdaptive::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (!(sleeping || blocked))
    {
        currentPayload = scheduler->getNextRequest(this);
        if (currentPayload != nullptr)
        {
            if (state == State::Precharged) // bank precharged
                nextCommand = Command::ACT;
            else if (state == State::Activated)
            {
                if (DramExtension::getRow(currentPayload) == openRow) // row hit
                {
                    if (scheduler->hasFurtherRequest(bank) && !scheduler->hasFurtherRowHit(bank, openRow))
                    {
                        if (currentPayload->get_command() == TLM_READ_COMMAND)
                            nextCommand = Command::RDA;
                        else if (currentPayload->get_command() == TLM_WRITE_COMMAND)
                            nextCommand = Command::WRA;
                        else
                            SC_REPORT_FATAL("BankMachine", "Wrong TLM command");
                    }
                    else
                    {
                        if (currentPayload->get_command() == TLM_READ_COMMAND)
                            nextCommand = Command::RD;
                        else if (currentPayload->get_command() == TLM_WRITE_COMMAND)
                            nextCommand = Command::WR;
                        else
                            SC_REPORT_FATAL("BankMachine", "Wrong TLM command");
                    }
                }
                else // row miss
                    nextCommand = Command::PRE;
            }
            timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, bankgroup, bank);
        }
    }
    return timeToSchedule;
}

BankMachineClosedAdaptive::BankMachineClosedAdaptive(SchedulerIF *scheduler, CheckerIF *checker, Bank bank)
    : BankMachine(scheduler, checker, bank) {}

sc_time BankMachineClosedAdaptive::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (!(sleeping || blocked))
    {
        currentPayload = scheduler->getNextRequest(this);
        if (currentPayload != nullptr)
        {
            if (state == State::Precharged && !blocked) // bank precharged
                nextCommand = Command::ACT;
            else if (state == State::Activated)
            {
                if (DramExtension::getRow(currentPayload) == openRow) // row hit
                {
                    if (scheduler->hasFurtherRowHit(bank, openRow))
                    {
                        if (currentPayload->get_command() == TLM_READ_COMMAND)
                            nextCommand = Command::RD;
                        else if (currentPayload->get_command() == TLM_WRITE_COMMAND)
                            nextCommand = Command::WR;
                        else
                            SC_REPORT_FATAL("BankMachine", "Wrong TLM command");
                    }
                    else
                    {
                        if (currentPayload->get_command() == TLM_READ_COMMAND)
                            nextCommand = Command::RDA;
                        else if (currentPayload->get_command() == TLM_WRITE_COMMAND)
                            nextCommand = Command::WRA;
                        else
                            SC_REPORT_FATAL("BankMachine", "Wrong TLM command");
                    }
                }
                else // row miss, should never happen
                    SC_REPORT_FATAL("BankMachine", "Should never be reached for this policy");
            }
            timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, bankgroup, bank);
        }
    }
    return timeToSchedule;
}
