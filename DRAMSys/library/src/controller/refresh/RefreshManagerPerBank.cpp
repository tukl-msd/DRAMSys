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

#include "RefreshManagerPerBank.h"
#include "../../configuration/Configuration.h"
#include "../../common/utils.h"
#include "../../common/dramExtensions.h"

using namespace tlm;

RefreshManagerPerBank::RefreshManagerPerBank(std::vector<BankMachine *> &bankMachinesOnRank,
        PowerDownManagerIF *powerDownManager, Rank rank, CheckerIF *checker)
    : bankMachinesOnRank(bankMachinesOnRank), powerDownManager(powerDownManager), rank(rank), checker(checker)
{
    Configuration &config = Configuration::getInstance();
    memSpec = config.memSpec;
    timeForNextTrigger = getTimeForFirstTrigger(memSpec->getRefreshIntervalPB(),
                                                rank, memSpec->numberOfRanks);

    refreshPayloads = std::vector<tlm_generic_payload>(memSpec->banksPerRank);
    for (unsigned bankID = 0; bankID < memSpec->banksPerRank; bankID++)
    {
        setUpDummy(refreshPayloads[bankID], 0, rank, bankMachinesOnRank[bankID]->getBankGroup(),
                   bankMachinesOnRank[bankID]->getBank());
        allBankMachines.push_back(bankMachinesOnRank[bankID]);
    }
    remainingBankMachines = allBankMachines;
    currentBankMachine = *remainingBankMachines.begin();

    maxPostponed = static_cast<int>(config.refreshMaxPostponed * memSpec->banksPerRank);
    maxPulledin = -static_cast<int>(config.refreshMaxPulledin * memSpec->banksPerRank);
}

CommandTuple::Type RefreshManagerPerBank::getNextCommand()
{
    return CommandTuple::Type(nextCommand, 
            &refreshPayloads[currentBankMachine->getBank().ID() % memSpec->banksPerRank], 
            std::max(timeToSchedule, sc_time_stamp()));
}

sc_time RefreshManagerPerBank::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (sc_time_stamp() >= timeForNextTrigger)
    {
        powerDownManager->triggerInterruption();
        if (sleeping)
            return timeToSchedule;

        if (sc_time_stamp() >= timeForNextTrigger + memSpec->getRefreshIntervalPB())
        {
            timeForNextTrigger += memSpec->getRefreshIntervalPB();
            state = State::Regular;
        }

        if (state == State::Regular)
        {
            bool forcedRefresh = (flexibilityCounter == maxPostponed);
            bool allBanksBusy = true;

            if (!skipSelection)
            {
                currentIterator = remainingBankMachines.begin();
                currentBankMachine = *remainingBankMachines.begin();

                for (auto it = remainingBankMachines.begin(); it != remainingBankMachines.end(); it++)
                {
                    if ((*it)->isIdle())
                    {
                        currentIterator = it;
                        currentBankMachine = *it;
                        allBanksBusy = false;
                        break;
                    }
                }
            }

            if (allBanksBusy && !forcedRefresh)
            {
                flexibilityCounter++;
                timeForNextTrigger += memSpec->getRefreshIntervalPB();
                return timeForNextTrigger;
            }
            else
            {
                if (currentBankMachine->getState() == BankMachine::State::Activated)
                    nextCommand = Command::PRE;
                else
                {
                    nextCommand = Command::REFB;

                    if (forcedRefresh)
                    {
                        currentBankMachine->block();
                        skipSelection = true;
                    }
                }

                timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank,
                        currentBankMachine->getBankGroup(), currentBankMachine->getBank());
                return timeToSchedule;
            }
        }
        else // if (state == RmState::Pulledin)
        {
            bool allBanksBusy = true;

            for (auto it = remainingBankMachines.begin(); it != remainingBankMachines.end(); it++)
            {
                if ((*it)->isIdle())
                {
                    currentIterator = it;
                    currentBankMachine = *it;
                    allBanksBusy = false;
                    break;
                }
            }

            if (allBanksBusy)
            {
                state = State::Regular;
                timeForNextTrigger += memSpec->getRefreshIntervalPB();
                return timeForNextTrigger;
            }
            else
            {
                if (currentBankMachine->getState() == BankMachine::State::Activated)
                    nextCommand = Command::PRE;
                else
                    nextCommand = Command::REFB;
                timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank,
                        currentBankMachine->getBankGroup(), currentBankMachine->getBank());
                return timeToSchedule;
            }
        }
    }
    else
        return timeForNextTrigger;
}

void RefreshManagerPerBank::updateState(Command command)
{
    switch (command)
    {
    case Command::REFB:
        skipSelection = false;
        remainingBankMachines.erase(currentIterator);
        if (remainingBankMachines.empty())
            remainingBankMachines = allBankMachines;

        if (state == State::Pulledin)
            flexibilityCounter--;
        else
            state = State::Pulledin;

        if (flexibilityCounter == maxPulledin)
        {
            state = State::Regular;
            timeForNextTrigger += memSpec->getRefreshIntervalPB();
        }
        break;
    case Command::REFA:
        // Refresh command after SREFEX
        state = State::Regular; // TODO: check if this assignment is necessary
        timeForNextTrigger = sc_time_stamp() + memSpec->getRefreshIntervalPB();
        sleeping = false;
        break;
    case Command::PDEA: case Command::PDEP:
        sleeping = true;
        break;
    case Command::SREFEN:
        sleeping = true;
        timeForNextTrigger = sc_max_time();
        break;
    case Command::PDXA: case Command::PDXP:
        sleeping = false;
        break;
    default:
        break;
    }
}
