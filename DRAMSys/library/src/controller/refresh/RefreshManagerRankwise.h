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

#ifndef REFRESHMANAGERRANKWISE_H
#define REFRESHMANAGERRANKWISE_H

#include "RefreshManagerIF.h"
#include "../../configuration/memspec/MemSpec.h"
#include "../BankMachine.h"
#include "../powerdown/PowerDownManagerIF.h"
#include "../checker/CheckerIF.h"

class RefreshManagerRankwise final : public RefreshManagerIF
{
public:
    RefreshManagerRankwise(std::vector<BankMachine *> &, PowerDownManagerIF *, Rank, CheckerIF *);

    virtual std::tuple<Command, tlm::tlm_generic_payload *, sc_time> getNextCommand() override;
    virtual sc_time start() override;
    virtual void updateState(Command) override;

private:
    enum class RmState {Regular, Pulledin} state = RmState::Regular;
    const MemSpec *memSpec;
    std::vector<BankMachine *> &bankMachinesOnRank;
    PowerDownManagerIF *powerDownManager;
    tlm::tlm_generic_payload refreshPayload;
    sc_time timeForNextTrigger = sc_max_time();
    sc_time timeToSchedule = sc_max_time();
    Rank rank;
    CheckerIF *checker;
    Command nextCommand = Command::NOP;

    unsigned activatedBanks = 0;

    int flexibilityCounter = 0;
    int maxPostponed = 0;
    int maxPulledin = 0;

    bool sleeping = false;
};

#endif // REFRESHMANAGERRANKWISE_H
