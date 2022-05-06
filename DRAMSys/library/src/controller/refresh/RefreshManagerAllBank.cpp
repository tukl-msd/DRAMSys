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
 *         Matthias Jung
 */

#include "RefreshManagerAllBank.h"
#include "../BankMachine.h"
#include "../powerdown/PowerDownManagerIF.h"
#include "../../common/dramExtensions.h"
#include "../../configuration/Configuration.h"
#include "../../common/utils.h"

using namespace sc_core;
using namespace tlm;

RefreshManagerAllBank::RefreshManagerAllBank(const Configuration& config, std::vector<BankMachine*>& bankMachinesOnRank,
                                             PowerDownManagerIF& powerDownManager, Rank rank, const CheckerIF& checker)
    : bankMachinesOnRank(bankMachinesOnRank), powerDownManager(powerDownManager), checker(checker),
    memSpec(*config.memSpec), maxPostponed(static_cast<int>(config.refreshMaxPostponed)),
    maxPulledin(-static_cast<int>(config.refreshMaxPulledin)), refreshManagement(config.refreshManagement)
{
    timeForNextTrigger = getTimeForFirstTrigger(memSpec.tCK, memSpec.getRefreshIntervalAB(),
                                                rank, memSpec.ranksPerChannel);
    setUpDummy(refreshPayload, 0, rank);
}

CommandTuple::Type RefreshManagerAllBank::getNextCommand()
{
    return {nextCommand, &refreshPayload, std::max(timeToSchedule, sc_time_stamp())};
}

sc_time RefreshManagerAllBank::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (sc_time_stamp() >= timeForNextTrigger) // Normal refresh
    {
        powerDownManager.triggerInterruption();

        if (sleeping)
            return timeToSchedule;

        if (sc_time_stamp() >= timeForNextTrigger + memSpec.getRefreshIntervalAB())
        {
            timeForNextTrigger += memSpec.getRefreshIntervalAB();
            state = State::Regular;
        }

        if (state == State::Regular)
        {
            bool doRefresh = true;
            if (flexibilityCounter == maxPostponed) // forced refresh
            {
                for (auto* it : bankMachinesOnRank)
                    it->block();
            }
            else
            {
                for (const auto* it : bankMachinesOnRank)
                {
                    if (!it->isIdle())
                    {
                        doRefresh = false;
                        flexibilityCounter++;
                        timeForNextTrigger += memSpec.getRefreshIntervalAB();
                        break;
                    }
                }
            }

            if (doRefresh)
            {
                if (activatedBanks > 0)
                    nextCommand = Command::PREAB;
                else
                    nextCommand = Command::REFAB;

                timeToSchedule = checker.timeToSatisfyConstraints(nextCommand, refreshPayload);
                return timeToSchedule;
            }
        }
        else // if (state == RmState::Pulledin)
        {
            bool doRefresh = true;
            for (const auto* it : bankMachinesOnRank)
            {
                if (!it->isIdle())
                {
                    doRefresh = false;
                    state = State::Regular;
                    timeForNextTrigger += memSpec.getRefreshIntervalAB();
                    break;
                }
            }

            if (doRefresh)
            {
                assert(activatedBanks == 0);
                nextCommand = Command::REFAB;
                timeToSchedule = checker.timeToSatisfyConstraints(nextCommand, refreshPayload);
                return timeToSchedule;
            }
        }
    }

    if (refreshManagement)
    {
        uint64_t maxThreshold = 0;
        for (const auto* bankMachine : bankMachinesOnRank)
            maxThreshold = std::max(maxThreshold, bankMachine->getRefreshManagementCounter());

        bool refreshManagementRequired = true;

        if (maxThreshold >= memSpec.getRAAMMT())
        {
            for (auto* bankMachine : bankMachinesOnRank)
                bankMachine->block();
        }
        else if (maxThreshold >= memSpec.getRAAIMT())
        {
            for (const auto* bankMachine : bankMachinesOnRank)
            {
                if (!bankMachine->isIdle())
                {
                    refreshManagementRequired = false;
                    break;
                }
            }
        }
        else
        {
            refreshManagementRequired = false;
        }

        if (refreshManagementRequired)
        {
            if (activatedBanks > 0)
                nextCommand = Command::PREAB;
            else
                nextCommand = Command::RFMAB;

            timeToSchedule = checker.timeToSatisfyConstraints(nextCommand, refreshPayload);
            return timeToSchedule;
        }
    }

    return timeForNextTrigger;
}

void RefreshManagerAllBank::updateState(Command command)
{
    switch (command)
    {
        case Command::ACT:
            activatedBanks++;
            break;
        case Command::PREPB: case Command::RDA: case Command::WRA:
            activatedBanks--;
            break;
        case Command::PREAB:
            activatedBanks = 0;
            break;
        case Command::REFAB:
            if (sleeping)
            {
                // Refresh command after SREFEX
                state = State::Regular; // TODO: check if this assignment is necessary
                timeForNextTrigger = sc_time_stamp() + memSpec.getRefreshIntervalAB();
                sleeping = false;
            }
            else
            {
                if (state == State::Pulledin)
                    flexibilityCounter--;
                else
                    state = State::Pulledin;

                if (flexibilityCounter == maxPulledin)
                {
                    state = State::Regular;
                    timeForNextTrigger += memSpec.getRefreshIntervalAB();
                }
            }
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
