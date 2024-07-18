/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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

#include "RefreshManagerPer2Bank.h"

#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerIF.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

RefreshManagerPer2Bank::RefreshManagerPer2Bank(
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
        memSpec.tCK, memSpec.getRefreshIntervalP2B(), rank, memSpec.ranksPerChannel);

    // each bank pair has one payload (e.g. 0-8, 1-9, 2-10, 3-11, ...)
    for (unsigned outerID = 0; outerID < memSpec.banksPerRank;
         outerID += (memSpec.getPer2BankOffset() * 2))
    {
        for (unsigned bankID = outerID; bankID < (outerID + memSpec.getPer2BankOffset()); bankID++)
        {
            Bank firstBank = Bank(bankID);
            Bank secondBank = Bank(bankID + memSpec.getPer2BankOffset());
            setUpDummy(refreshPayloads[bankMachinesOnRank[firstBank]],
                       0,
                       rank,
                       bankMachinesOnRank[firstBank]->getBankGroup(),
                       bankMachinesOnRank[firstBank]->getBank());
            setUpDummy(refreshPayloads[bankMachinesOnRank[secondBank]],
                       0,
                       rank,
                       bankMachinesOnRank[secondBank]->getBankGroup(),
                       bankMachinesOnRank[secondBank]->getBank());
            allBankMachines.push_back(
                {bankMachinesOnRank[firstBank], bankMachinesOnRank[secondBank]});
        }
    }

    remainingBankMachines = allBankMachines;
    currentIterator = remainingBankMachines.begin();
    currentRefreshPayload = &refreshPayloads.at(currentIterator->front());
}

CommandTuple::Type RefreshManagerPer2Bank::getNextCommand()
{
    return {nextCommand, currentRefreshPayload, SC_ZERO_TIME};
}

void RefreshManagerPer2Bank::evaluate()
{
    nextCommand = Command::NOP;

    if (sc_time_stamp() >= timeForNextTrigger)
    {
        powerDownManager.triggerInterruption();
        if (sleeping)
            return;

        if (sc_time_stamp() >= timeForNextTrigger + memSpec.getRefreshIntervalP2B())
        {
            timeForNextTrigger += memSpec.getRefreshIntervalP2B();
            state = State::Regular;
        }

        if (state == State::Regular)
        {
            bool forcedRefresh = (flexibilityCounter == maxPostponed);
            bool allBankPairsBusy = true;

            if (!skipSelection)
            {
                currentIterator = remainingBankMachines.begin();
                for (auto bankIt = remainingBankMachines.begin();
                     bankIt != remainingBankMachines.end();
                     bankIt++)
                {
                    bool pairIsBusy = false;
                    for (const auto* pairIt : *bankIt)
                    {
                        if (!pairIt->isIdle())
                        {
                            pairIsBusy = true;
                            break;
                        }
                    }
                    if (!pairIsBusy)
                    {
                        allBankPairsBusy = false;
                        currentIterator = bankIt;
                        break;
                    }
                }
            }

            if (allBankPairsBusy && !forcedRefresh)
            {
                flexibilityCounter++;
                timeForNextTrigger += memSpec.getRefreshIntervalP2B();
                return;
            }

            nextCommand = Command::REFP2B;
            currentRefreshPayload = &refreshPayloads.at(currentIterator->front());
            for (auto* it : *currentIterator)
            {
                if (it->isActivated())
                {
                    nextCommand = Command::PREPB;
                    currentRefreshPayload = &refreshPayloads.at(it);
                    break;
                }
            }

            // TODO: banks should already be blocked for precharge and selection should be skipped
            if (nextCommand == Command::REFP2B && forcedRefresh)
            {
                for (auto* it : *currentIterator)
                    it->block();
                skipSelection = true;
            }
            return;
        }

        // if (state == RmState::Pulledin)
        bool allBankPairsBusy = true;

        currentIterator = remainingBankMachines.begin();
        for (auto bankIt = remainingBankMachines.begin(); bankIt != remainingBankMachines.end();
             bankIt++)
        {
            bool pairIsBusy = false;
            for (const auto* pairIt : *bankIt)
            {
                if (!pairIt->isIdle())
                {
                    pairIsBusy = true;
                    break;
                }
            }
            if (!pairIsBusy)
            {
                allBankPairsBusy = false;
                currentIterator = bankIt;
                break;
            }
        }

        if (allBankPairsBusy)
        {
            state = State::Regular;
            timeForNextTrigger += memSpec.getRefreshIntervalP2B();
            return;
        }

        nextCommand = Command::REFP2B;
        currentRefreshPayload = &refreshPayloads.at(currentIterator->front());
        for (auto* it : *currentIterator)
        {
            if (it->isActivated())
            {
                nextCommand = Command::PREPB;
                currentRefreshPayload = &refreshPayloads.at(it);
                break;
            }
        }
        return;
    }
}

void RefreshManagerPer2Bank::update(Command command)
{
    switch (command)
    {
    case Command::REFP2B:
        skipSelection = false;
        remainingBankMachines.erase(currentIterator);
        if (remainingBankMachines.empty())
            remainingBankMachines = allBankMachines;
        currentIterator = remainingBankMachines.begin();

        if (state == State::Pulledin)
            flexibilityCounter--;
        else
            state = State::Pulledin;

        if (flexibilityCounter == maxPulledin)
        {
            state = State::Regular;
            timeForNextTrigger += memSpec.getRefreshIntervalP2B();
        }
        break;
    case Command::REFAB:
        // Refresh command after SREFEX
        state = State::Regular; // TODO: check if this assignment is necessary
        timeForNextTrigger = sc_time_stamp() + memSpec.getRefreshIntervalP2B();
        sleeping = false;
        remainingBankMachines = allBankMachines;
        currentIterator = remainingBankMachines.begin();
        skipSelection = false;
        break;
    case Command::PDEA:
    case Command::PDEP:
        sleeping = true;
        break;
    case Command::SREFEN:
        sleeping = true;
        timeForNextTrigger = scMaxTime;
        break;
    case Command::PDXA:
    case Command::PDXP:
        sleeping = false;
        break;
    default:
        break;
    }
}

sc_time RefreshManagerPer2Bank::getTimeForNextTrigger()
{
    return timeForNextTrigger;
}

} // namespace DRAMSys
