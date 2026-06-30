/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 *    Lukas Steiner
 */

#include "RefreshManagerDualBank.h"

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerIF.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

RefreshManagerDualBank::RefreshManagerDualBank(
    const McConfig& config,
    const MemSpec& memSpec,
    ControllerVector<Bank, BankMachine*>& bankMachinesOnRank,
    PowerDownManagerIF& powerDownManager,
    Rank rank) :
    memSpec(memSpec),
    powerDownManager(powerDownManager),
    maxPostponed(static_cast<int>(config.refreshMaxPostponed * memSpec.banksPerRank / 2)),
    maxPulledin(-static_cast<int>(config.refreshMaxPulledin * memSpec.banksPerRank / 2))
{
    timeForNextTrigger = getTimeForFirstTrigger(
        memSpec.tCK, memSpec.getRefreshIntervalDB(), rank, memSpec.ranksPerChannel);

    allBankMachines.insert(allBankMachines.end(), bankMachinesOnRank.begin(), bankMachinesOnRank.end());
    remainingBankMachines = allBankMachines;
    setUpDummy(refreshPayload, 0, rank);
}

ReadyCommand RefreshManagerDualBank::getNextCommand()
{
    return {nextCommand, &refreshPayload, SC_ZERO_TIME};
}

void RefreshManagerDualBank::evaluate()
{
    nextCommand = Command::NOP;

    if (sc_time_stamp() >= timeForNextTrigger)
    {
        powerDownManager.triggerInterruption();
        if (!sleeping)
        {
            if (sc_time_stamp() >= timeForNextTrigger + memSpec.getRefreshIntervalDB())
            {
                timeForNextTrigger += memSpec.getRefreshIntervalDB();
                state = State::Regular;
            }

            if (state == State::Regular)
            {
                if (flexibilityCounter == maxPostponed) // forced refresh
                {
                    if (!skipSelection)
                    {
                        if (auto idleBankPair = searchIdleBankPair())
                        {
                            currentIterator = idleBankPair.value();
                        }
                        else
                        {
                            currentIterator = searchLegalBankPair();
                        }
                        (*currentIterator.first)->block();
                        (*currentIterator.second)->block();
                        skipSelection = true;
                    }

                    if ((*currentIterator.first)->isActivated())
                    {
                        nextCommand = Command::PREPB;
                        ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.first)->getBankGroup());
                        ControllerExtension::setBank(refreshPayload, (*currentIterator.first)->getBank());
                    }
                    else if ((*currentIterator.second)->isActivated())
                    {
                        nextCommand = Command::PREPB;
                        ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.second)->getBankGroup());
                        ControllerExtension::setBank(refreshPayload, (*currentIterator.second)->getBank());
                    }
                    else
                    {
                        nextCommand = Command::REFDB;
                        ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.first)->getBankGroup());
                        ControllerExtension::setBank(refreshPayload, (*currentIterator.first)->getBank());
                        ControllerExtension::setDualBank(refreshPayload, (*currentIterator.second)->getBank());
                    }
                }
                else // postponable regular refresh
                {
                    if (auto idleBankPair = searchIdleBankPair()) // do refresh
                    {
                        currentIterator = idleBankPair.value();

                        if ((*currentIterator.first)->isActivated())
                        {
                            nextCommand = Command::PREPB;
                            ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.first)->getBankGroup());
                            ControllerExtension::setBank(refreshPayload, (*currentIterator.first)->getBank());
                        }
                        else if ((*currentIterator.second)->isActivated())
                        {
                            nextCommand = Command::PREPB;
                            ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.second)->getBankGroup());
                            ControllerExtension::setBank(refreshPayload, (*currentIterator.second)->getBank());
                        }
                        else
                        {
                            nextCommand = Command::REFDB;
                            ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.first)->getBankGroup());
                            ControllerExtension::setBank(refreshPayload, (*currentIterator.first)->getBank());
                            ControllerExtension::setDualBank(refreshPayload, (*currentIterator.second)->getBank());
                        }
                    }
                    else // postpone refresh
                    {
                        flexibilityCounter++;
                        timeForNextTrigger += memSpec.getRefreshIntervalDB();
                    }
                }
            }
            else // if (state == RmState::Pulledin)
            {
                if (auto idleBankPair = searchIdleBankPair())
                {
                    currentIterator = idleBankPair.value();
                    if ((*currentIterator.first)->isActivated())
                    {
                        nextCommand = Command::PREPB;
                        ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.first)->getBankGroup());
                        ControllerExtension::setBank(refreshPayload, (*currentIterator.first)->getBank());
                    }
                    else if ((*currentIterator.second)->isActivated())
                    {
                        nextCommand = Command::PREPB;
                        ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.second)->getBankGroup());
                        ControllerExtension::setBank(refreshPayload, (*currentIterator.second)->getBank());
                    }
                    else
                    {
                        nextCommand = Command::REFDB;
                        ControllerExtension::setBankGroup(refreshPayload, (*currentIterator.first)->getBankGroup());
                        ControllerExtension::setBank(refreshPayload, (*currentIterator.first)->getBank());
                        ControllerExtension::setDualBank(refreshPayload, (*currentIterator.second)->getBank());
                    }
                }
                else
                {
                    state = State::Regular;
                    timeForNextTrigger += memSpec.getRefreshIntervalDB();
                }
            }
        }
    }
}

void RefreshManagerDualBank::update(Command command)
{
    switch (command)
    {
    case Command::REFDB:
        skipSelection = false;
        remainingBankMachines.erase(currentIterator.first);
        remainingBankMachines.erase(currentIterator.second);
        if (remainingBankMachines.empty())
            remainingBankMachines = allBankMachines;

        if (state == State::Pulledin)
            flexibilityCounter--;
        else
            state = State::Pulledin;

        if (flexibilityCounter == maxPulledin)
        {
            state = State::Regular;
            timeForNextTrigger += memSpec.getRefreshIntervalDB();
        }
        break;
    case Command::REFAB:
        // Refresh command after SREFEX
        state = State::Regular; // TODO: check if this assignment is necessary
        timeForNextTrigger = sc_time_stamp() + memSpec.getRefreshIntervalDB();
        sleeping = false;
        remainingBankMachines = allBankMachines;
        skipSelection = false;
        break;
    case Command::PDEA:
    case Command::PDEP:
        sleeping = true;
        break;
    case Command::SREFEN:
        sleeping = true;
        timeForNextTrigger = sc_max_time();
        break;
    case Command::PDXA:
    case Command::PDXP:
        sleeping = false;
        break;
    default:
        break;
    }
}

sc_time RefreshManagerDualBank::getTimeForNextTrigger()
{
    return timeForNextTrigger;
}

void RefreshManagerDualBank::serialize(std::ostream& stream) const
{
    stream.write(reinterpret_cast<char const*>(&timeForNextTrigger), sizeof(timeForNextTrigger));
}

void RefreshManagerDualBank::deserialize(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(&timeForNextTrigger), sizeof(timeForNextTrigger));
}

std::optional<std::pair<std::list<BankMachine*>::iterator, std::list<BankMachine*>::iterator>>
RefreshManagerDualBank::searchIdleBankPair()
{
    auto firstBankIt =
        std::find_if(remainingBankMachines.begin(),
                     remainingBankMachines.end(),
                     [](const BankMachine* bankMachine) { return bankMachine->isIdle(); });

    if (firstBankIt == remainingBankMachines.end())
        return std::nullopt;

    const BankMachine* firstBankMachine = *firstBankIt;

    auto secondBankIt = std::find_if(
        remainingBankMachines.begin(),
        remainingBankMachines.end(),
        [firstBankMachine, this](const BankMachine* secondBankMachine)
        {
            return secondBankMachine->isIdle() && secondBankMachine != firstBankMachine &&
                   static_cast<std::size_t>(secondBankMachine->getBank()) % memSpec.banksPerGroup ==
                       static_cast<std::size_t>(firstBankMachine->getBank()) %
                           memSpec.banksPerGroup;
        });

    if (secondBankIt == remainingBankMachines.end())
        return std::nullopt;

    return std::make_pair(firstBankIt, secondBankIt);
}

std::pair<std::list<BankMachine*>::iterator, std::list<BankMachine*>::iterator>
RefreshManagerDualBank::searchLegalBankPair()
{
    auto firstBankIt = remainingBankMachines.begin();
    const BankMachine* firstBankMachine = *firstBankIt;

    auto secondBankIt = std::find_if(
        remainingBankMachines.begin(),
        remainingBankMachines.end(),
        [firstBankMachine, this](const BankMachine* secondBankMachine)
        {
            return secondBankMachine != firstBankMachine &&
                   static_cast<std::size_t>(secondBankMachine->getBank()) % memSpec.banksPerGroup ==
                       static_cast<std::size_t>(firstBankMachine->getBank()) %
                           memSpec.banksPerGroup;
        });

    return std::make_pair(firstBankIt, secondBankIt);
}

} // namespace DRAMSys
