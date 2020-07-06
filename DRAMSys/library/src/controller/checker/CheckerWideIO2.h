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

#ifndef CHECKERWIDEIO2_H
#define CHECKERWIDEIO2_H

#include "CheckerIF.h"
#include <queue>
#include <vector>
#include "../../configuration/memspec/MemSpecWideIO2.h"
#include "../../configuration/Configuration.h"

class CheckerWideIO2 final : public CheckerIF
{
public:
    CheckerWideIO2();
    virtual sc_time timeToSatisfyConstraints(Command, Rank, BankGroup, Bank) const override;
    virtual void insert(Command, Rank, BankGroup, Bank) override;

private:
    const MemSpecWideIO2 *memSpec;

    std::vector<std::vector<sc_time>> lastScheduledByCommandAndBank;
    std::vector<std::vector<sc_time>> lastScheduledByCommandAndRank;
    std::vector<sc_time> lastScheduledByCommand;
    sc_time lastCommandOnBus;

    // Four activate window
    std::vector<std::queue<sc_time>> last4Activates;

    sc_time tBURST;
    sc_time tRDPRE;
    sc_time tRDPDEN;
    sc_time tRDWR;
    sc_time tRDWR_R;
    sc_time tWRPRE;
    sc_time tWRPDEN;
    sc_time tWRAPDEN;
    sc_time tWRRD;
    sc_time tWRRD_R;
};

#endif // CHECKERWIDEIO2_H
