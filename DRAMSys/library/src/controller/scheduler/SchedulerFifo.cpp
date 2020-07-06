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

#include "SchedulerFifo.h"

using namespace tlm;

SchedulerFifo::SchedulerFifo()
{
    buffer = std::vector<std::deque<tlm_generic_payload *>>
            (Configuration::getInstance().memSpec->numberOfBanks);
    requestBufferSize = Configuration::getInstance().requestBufferSize;
}

bool SchedulerFifo::hasBufferSpace()
{
    if (buffer[lastBankID].size() < requestBufferSize)
        return true;
    else
        return false;
}

void SchedulerFifo::storeRequest(tlm_generic_payload *payload)
{
    lastBankID = DramExtension::getBank(payload).ID();
    buffer[lastBankID].push_back(payload);
}

void SchedulerFifo::removeRequest(tlm_generic_payload *payload)
{
    buffer[DramExtension::getBank(payload).ID()].pop_front();
}

tlm_generic_payload *SchedulerFifo::getNextRequest(BankMachine *bankMachine)
{
    unsigned bankID = bankMachine->getBank().ID();
    if (!buffer[bankID].empty())
        return buffer[bankID].front();
    else
        return nullptr;
}

bool SchedulerFifo::hasFurtherRowHit(Bank bank, Row row)
{
    if (buffer[bank.ID()].size() >= 2)
    {
        tlm_generic_payload *nextRequest = buffer[bank.ID()][1];
        if (DramExtension::getRow(nextRequest) == row)
            return true;
    }
    return false;
}

bool SchedulerFifo::hasFurtherRequest(Bank bank)
{
    if (buffer[bank.ID()].size() >= 2)
        return true;
    else
        return false;
}
