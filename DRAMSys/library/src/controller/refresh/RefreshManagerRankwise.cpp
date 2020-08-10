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

#include "RefreshManagerRankwise.h"
#include "../../common/dramExtensions.h"
#include "../../configuration/Configuration.h"
#include "../../common/utils.h"

using namespace tlm;

RefreshManagerRankwise::RefreshManagerRankwise(std::vector<BankMachine *> &bankMachines,
        PowerDownManagerIF *powerDownManager, Rank rank, CheckerIF *checker)
    : bankMachinesOnRank(bankMachines), powerDownManager(powerDownManager), rank(rank), checker(checker)
{
    Configuration &config = Configuration::getInstance();
    memSpec = config.memSpec;
    timeForNextTrigger = memSpec->getRefreshIntervalAB();
    setUpDummy(refreshPayload, 0, rank);

    maxPostponed = config.refreshMaxPostponed;
    maxPulledin = -config.refreshMaxPulledin;
}

std::tuple<Command, tlm_generic_payload *, sc_time> RefreshManagerRankwise::getNextCommand()
{
    return std::tuple<Command, tlm_generic_payload *, sc_time>(nextCommand, &refreshPayload, timeToSchedule);
}

sc_time RefreshManagerRankwise::start()
{
    timeToSchedule = sc_max_time();
    nextCommand = Command::NOP;

    if (sc_time_stamp() >= timeForNextTrigger)
    {
        powerDownManager->triggerInterruption();
        if (sleeping)
            return timeToSchedule;

        if (sc_time_stamp() >= timeForNextTrigger + memSpec->getRefreshIntervalAB())
        {
            timeForNextTrigger += memSpec->getRefreshIntervalAB();
            state = RmState::Regular;
        }

        if (state == RmState::Regular)
        {
            if (flexibilityCounter == maxPostponed) // forced refresh
            {
                for (auto it : bankMachinesOnRank)
                    it->block();
            }
            else
            {
                bool controllerBusy = false;
                for (auto it : bankMachinesOnRank)
                {
                    if (!it->isIdle())
                    {
                        controllerBusy = true;
                        break;
                    }
                }

                if (controllerBusy)
                {
                    flexibilityCounter++;
                    timeForNextTrigger += memSpec->getRefreshIntervalAB();
                    return timeForNextTrigger;
                }
            }

            if (activatedBanks > 0)
                nextCommand = Command::PREA;
            else
                nextCommand = Command::REFA;
            timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, BankGroup(0), Bank(0));
            return timeToSchedule;
        }
        else // if (state == RmState::Pulledin)
        {
            bool controllerBusy = false;
            for (auto it : bankMachinesOnRank)
            {
                if (!it->isIdle())
                {
                    controllerBusy = true;
                    break;
                }
            }

            if (controllerBusy)
            {
                state = RmState::Regular;
                timeForNextTrigger += memSpec->getRefreshIntervalAB();
                return timeForNextTrigger;
            }
            else
            {
                nextCommand = Command::REFA;
                timeToSchedule = checker->timeToSatisfyConstraints(nextCommand, rank, BankGroup(0), Bank(0));
                return timeToSchedule;
            }
        }
    }
    else
        return timeForNextTrigger;
}

void RefreshManagerRankwise::updateState(Command command)
{
    switch (command)
    {
    case Command::ACT:
        activatedBanks++;
        break;
    case Command::PRE: case Command::RDA: case Command::WRA:
        activatedBanks--;
        break;
    case Command::PREA:
        activatedBanks = 0;
        break;
    case Command::REFA:
        if (sleeping)
        {
            // Refresh command after SREFEX
            state = RmState::Regular; // TODO: check if this assignment is necessary
            timeForNextTrigger = sc_time_stamp() + memSpec->getRefreshIntervalAB();
            sleeping = false;
        }
        else
        {
            if (state == RmState::Pulledin)
                flexibilityCounter--;
            else
                state = RmState::Pulledin;

            if (flexibilityCounter == maxPulledin)
            {
                state = RmState::Regular;
                timeForNextTrigger += memSpec->getRefreshIntervalAB();
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
    }
}
