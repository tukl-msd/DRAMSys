/*
 * Copyright (c) 2020, RPTU Kaiserslautern-Landau
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

#include "RefreshManagerSameBank.h"

#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerIF.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

RefreshManagerSameBank::RefreshManagerSameBank(
    const McConfig& config,
    const MemSpec& memSpec,
    ControllerVector<Bank, BankMachine*>& bankMachinesOnRank,
    PowerDownManagerIF& powerDownManager,
    Rank rank) :
    memSpec(memSpec),
    powerDownManager(powerDownManager),
    maxPostponed(static_cast<int>(config.refreshMaxPostponed * memSpec.banksPerGroup)),
    maxPulledin(-static_cast<int>(config.refreshMaxPulledin * memSpec.banksPerGroup)),
    refreshManagement(config.refreshManagement)
{
    timeForNextTrigger = getTimeForFirstTrigger(
        memSpec.tCK, memSpec.getRefreshIntervalSB(), rank, memSpec.ranksPerChannel);

    // each same-bank group has one payload (e.g. 0-4-8-12-16-20-24-28)
    refreshPayloads = std::vector<tlm_generic_payload>(memSpec.banksPerGroup);
    for (unsigned bankID = 0; bankID < memSpec.banksPerGroup; bankID++)
    {
        // rank 0: bank group 0, bank 0 - 3; rank 1: bank group 8, bank 32 - 35
        setUpDummy(refreshPayloads[bankID],
                   0,
                   rank,
                   bankMachinesOnRank[Bank(bankID)]->getBankGroup(),
                   bankMachinesOnRank[Bank(bankID)]->getBank());
        allBankMachines.emplace_back(std::vector<BankMachine*>(memSpec.groupsPerRank));
    }

    // allBankMachines: ((0-4-8-12-16-20-24-28), (1-5-9-13-17-21-25-29), ...)
    auto it = allBankMachines.begin();
    for (unsigned bankID = 0; bankID < memSpec.banksPerGroup; bankID++)
    {
        for (unsigned groupID = 0; groupID < memSpec.groupsPerRank; groupID++)
            (*it)[groupID] = bankMachinesOnRank[Bank(groupID * memSpec.banksPerGroup + bankID)];
        it++;
    }

    remainingBankMachines = allBankMachines;
    currentIterator = remainingBankMachines.begin();
}

CommandTuple::Type RefreshManagerSameBank::getNextCommand()
{
    return {nextCommand,
            &refreshPayloads[static_cast<std::size_t>(currentIterator->front()->getBank()) %
                             memSpec.banksPerGroup],
            SC_ZERO_TIME};
}

void RefreshManagerSameBank::evaluate()
{
    nextCommand = Command::NOP;

    if (sc_time_stamp() >= timeForNextTrigger)
    {
        powerDownManager.triggerInterruption();
        if (sleeping)
            return;

        if (sc_time_stamp() >= timeForNextTrigger + memSpec.getRefreshIntervalSB())
        {
            timeForNextTrigger += memSpec.getRefreshIntervalSB();
            state = State::Regular;
        }

        if (state == State::Regular)
        {
            bool forcedRefresh = (flexibilityCounter == maxPostponed);
            bool allGroupsBusy = true;

            if (!skipSelection)
            {
                currentIterator = remainingBankMachines.begin();
                for (auto bankIt = remainingBankMachines.begin();
                     bankIt != remainingBankMachines.end();
                     bankIt++)
                {
                    bool groupIsBusy = false;
                    for (const auto* groupIt : *bankIt)
                    {
                        if (!groupIt->isIdle())
                        {
                            groupIsBusy = true;
                            break;
                        }
                    }
                    if (!groupIsBusy)
                    {
                        allGroupsBusy = false;
                        currentIterator = bankIt;
                        break;
                    }
                }
            }

            if (allGroupsBusy && !forcedRefresh)
            {
                flexibilityCounter++;
                timeForNextTrigger += memSpec.getRefreshIntervalSB();
            }
            else
            {
                nextCommand = Command::REFSB;
                for (const auto* it : *currentIterator)
                {
                    if (it->isActivated())
                    {
                        nextCommand = Command::PRESB;
                        break;
                    }
                }

                // TODO: banks should already be blocked for precharge and selection should be
                // skipped only check for forced refresh, also block for PRESB
                if (nextCommand == Command::REFSB && forcedRefresh)
                {
                    for (auto* it : *currentIterator)
                        it->block();
                    skipSelection = true;
                }
                return;
            }
        }
        else // if (state == RmState::Pulledin)
        {
            bool allGroupsBusy = true;

            currentIterator = remainingBankMachines.begin();
            for (auto bankIt = remainingBankMachines.begin(); bankIt != remainingBankMachines.end();
                 bankIt++)
            {
                bool groupIsBusy = false;
                for (const auto* groupIt : *bankIt)
                {
                    if (!groupIt->isIdle())
                    {
                        groupIsBusy = true;
                        break;
                    }
                }
                if (!groupIsBusy)
                {
                    allGroupsBusy = false;
                    currentIterator = bankIt;
                    break;
                }
            }

            if (allGroupsBusy)
            {
                state = State::Regular;
                timeForNextTrigger += memSpec.getRefreshIntervalSB();
            }
            else
            {
                nextCommand = Command::REFSB;
                for (const auto* it : *currentIterator)
                {
                    if (it->isActivated())
                    {
                        nextCommand = Command::PRESB;
                        break;
                    }
                }
                return;
            }
        }
    }

    if (refreshManagement)
    {
        bool mmtReached = false;
        std::vector<std::list<std::vector<BankMachine*>>::iterator> imtCandidates;

        for (auto bankIt = allBankMachines.begin(); bankIt != allBankMachines.end(); bankIt++)
        {
            for (const auto* groupIt : *bankIt)
            {
                if (groupIt->getRefreshManagementCounter() >= memSpec.getRAAMMT())
                {
                    mmtReached = true;
                    currentIterator = bankIt;
                    break;
                }
                if (groupIt->getRefreshManagementCounter() >= memSpec.getRAAIMT())
                {
                    imtCandidates.emplace_back(bankIt);
                }
            }
        }

        if (mmtReached)
        {
            nextCommand = Command::RFMSB;
            for (auto* groupIt : *currentIterator)
            {
                groupIt->block();
                if (groupIt->isActivated())
                    nextCommand = Command::PRESB;
            }
            return;
        }
        if (!imtCandidates.empty())
        {
            // search for IMT candidates and check if all banks idle
            bool allGroupsBusy = true;
            for (auto candidateIt : imtCandidates)
            {
                bool groupIsBusy = false;
                for (const auto* groupIt : *candidateIt)
                {
                    if (!groupIt->isIdle())
                    {
                        groupIsBusy = true;
                        break;
                    }
                }
                if (!groupIsBusy)
                {
                    allGroupsBusy = false;
                    currentIterator = candidateIt;
                    break;
                }
            }
            if (!allGroupsBusy)
            {
                nextCommand = Command::RFMSB;
                for (const auto* it : *currentIterator)
                {
                    if (it->isActivated())
                    {
                        nextCommand = Command::PRESB;
                        break;
                    }
                }
                return;
            }
        }
    }
}

void RefreshManagerSameBank::update(Command command)
{
    switch (command)
    {
    case Command::REFSB:
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
            timeForNextTrigger += memSpec.getRefreshIntervalSB();
        }
        break;
    case Command::REFAB:
        // Refresh command after SREFEX
        state = State::Regular; // TODO: check if this assignment is necessary
        timeForNextTrigger = sc_time_stamp() + memSpec.getRefreshIntervalSB();
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

sc_time RefreshManagerSameBank::getTimeForNextTrigger()
{
    return timeForNextTrigger;
}

} // namespace DRAMSys
