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

#include "RefreshManagerPerBank.h"

#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerIF.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

RefreshManagerPerBank::RefreshManagerPerBank(
    const McConfig& config,
    const MemSpec& memSpec,
    ControllerVector<Bank, BankMachine*>& bankMachinesOnRank,
    PowerDownManagerIF& powerDownManager,
    Rank rank) :
    memSpec(memSpec),
    powerDownManager(powerDownManager),
    maxPostponed(static_cast<int>(config.refreshMaxPostponed * memSpec.banksPerRank)),
    maxPulledin(-static_cast<int>(config.refreshMaxPulledin * memSpec.banksPerRank))
{
    timeForNextTrigger = getTimeForFirstTrigger(
        memSpec.tCK, memSpec.getRefreshIntervalPB(), rank, memSpec.ranksPerChannel);
    for (auto* it : bankMachinesOnRank)
    {
        setUpDummy(refreshPayloads[it], 0, rank, it->getBankGroup(), it->getBank());
        allBankMachines.push_back(it);
    }

    remainingBankMachines = allBankMachines;
    currentIterator = remainingBankMachines.begin();
}

CommandTuple::Type RefreshManagerPerBank::getNextCommand()
{
    return {nextCommand, &refreshPayloads.at(*currentIterator), SC_ZERO_TIME};
}

void RefreshManagerPerBank::evaluate()
{
    nextCommand = Command::NOP;

    if (sc_time_stamp() >= timeForNextTrigger)
    {
        powerDownManager.triggerInterruption();
        if (sleeping)
            return;

        if (sc_time_stamp() >= timeForNextTrigger + memSpec.getRefreshIntervalPB())
        {
            timeForNextTrigger += memSpec.getRefreshIntervalPB();
            state = State::Regular;
        }

        if (state == State::Regular)
        {
            bool forcedRefresh = (flexibilityCounter == maxPostponed);
            bool allBanksBusy = true;

            if (!skipSelection)
            {
                currentIterator = remainingBankMachines.begin();

                for (auto it = remainingBankMachines.begin(); it != remainingBankMachines.end();
                     it++)
                {
                    if ((*it)->isIdle())
                    {
                        currentIterator = it;
                        allBanksBusy = false;
                        break;
                    }
                }
            }

            if (allBanksBusy && !forcedRefresh)
            {
                flexibilityCounter++;
                timeForNextTrigger += memSpec.getRefreshIntervalPB();
                return;
            }

            // TODO: bank should already be blocked for precharge and selection should be skipped
            if ((*currentIterator)->isActivated())
                nextCommand = Command::PREPB;
            else
            {
                nextCommand = Command::REFPB;

                if (forcedRefresh)
                {
                    (*currentIterator)->block();
                    skipSelection = true;
                }
            }
            return;
        }

        // if (state == RmState::Pulledin)
        bool allBanksBusy = true;

        for (auto it = remainingBankMachines.begin(); it != remainingBankMachines.end(); it++)
        {
            if ((*it)->isIdle())
            {
                currentIterator = it;
                allBanksBusy = false;
                break;
            }
        }

        if (allBanksBusy)
        {
            state = State::Regular;
            timeForNextTrigger += memSpec.getRefreshIntervalPB();
            return;
        }

        if ((*currentIterator)->isActivated())
            nextCommand = Command::PREPB;
        else
            nextCommand = Command::REFPB;

        return;
    }
}

void RefreshManagerPerBank::update(Command command)
{
    switch (command)
    {
    case Command::REFPB:
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
            timeForNextTrigger += memSpec.getRefreshIntervalPB();
        }
        break;
    case Command::REFAB:
        // Refresh command after SREFEX
        state = State::Regular; // TODO: check if this assignment is necessary
        timeForNextTrigger = sc_time_stamp() + memSpec.getRefreshIntervalPB();
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

sc_time RefreshManagerPerBank::getTimeForNextTrigger()
{
    return timeForNextTrigger;
}

} // namespace DRAMSys
