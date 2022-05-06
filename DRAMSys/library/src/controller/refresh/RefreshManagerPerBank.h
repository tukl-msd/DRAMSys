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

#ifndef REFRESHMANAGERPERBANK_H
#define REFRESHMANAGERPERBANK_H

#include <vector>
#include <list>
#include <unordered_map>

#include <systemc>
#include <tlm>
#include "RefreshManagerIF.h"
#include "../checker/CheckerIF.h"
#include "../../configuration/memspec/MemSpec.h"
#include "../../configuration/Configuration.h"

class BankMachine;
class PowerDownManagerIF;

class RefreshManagerPerBank final : public RefreshManagerIF
{
public:
    RefreshManagerPerBank(const Configuration& config, std::vector<BankMachine *>& bankMachinesOnRank,
                          PowerDownManagerIF& powerDownManager, Rank rank, const CheckerIF& checker);

    CommandTuple::Type getNextCommand() override;
    sc_core::sc_time start() override;
    void updateState(Command) override;

private:
    enum class State {Regular, Pulledin} state = State::Regular;
    const MemSpec& memSpec;
    PowerDownManagerIF& powerDownManager;
    std::unordered_map<BankMachine*, tlm::tlm_generic_payload> refreshPayloads;
    sc_core::sc_time timeForNextTrigger = sc_core::sc_max_time();
    sc_core::sc_time timeToSchedule = sc_core::sc_max_time();
    const CheckerIF& checker;
    Command nextCommand = Command::NOP;

    std::list<BankMachine*> remainingBankMachines;
    std::list<BankMachine*> allBankMachines;
    std::list<BankMachine*>::iterator currentIterator;

    int flexibilityCounter = 0;
    const int maxPostponed;
    const int maxPulledin;

    bool sleeping = false;
    bool skipSelection = false;
};

#endif // REFRESHMANAGERPERBANK_H
