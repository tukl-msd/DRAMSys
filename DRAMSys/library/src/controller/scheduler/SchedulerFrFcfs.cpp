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

#include "SchedulerFrFcfs.h"

#include <systemc.h>

using namespace tlm;

SchedulerFrFcfs::SchedulerFrFcfs()
{
    buffer = std::vector<std::list<tlm_generic_payload *>>
            (Configuration::getInstance().memSpec->numberOfBanks);
    requestBufferSize = Configuration::getInstance().requestBufferSize;
}

bool SchedulerFrFcfs::hasBufferSpace()
{
    if (buffer[lastBankID].size() < requestBufferSize)
        return true;
    else
        return false;
}

void SchedulerFrFcfs::storeRequest(tlm_generic_payload *payload)
{
    lastBankID = DramExtension::getBank(payload).ID();
    buffer[lastBankID].push_back(payload);
}

void SchedulerFrFcfs::removeRequest(tlm_generic_payload *payload)
{
    unsigned bankID = DramExtension::getBank(payload).ID();
    for (auto it = buffer[bankID].begin(); it != buffer[bankID].end(); it++)
    {
        if (*it == payload)
        {
            buffer[bankID].erase(it);
            return;
        }
    }
    SC_REPORT_FATAL("SchedulerFrFcfs", "removeRequest failed!");
}

tlm_generic_payload *SchedulerFrFcfs::getNextRequest(BankMachine *bankMachine)
{
    unsigned bankID = bankMachine->getBank().ID();
    if (!buffer[bankID].empty())
    {
        if (bankMachine->getState() == BmState::Activated)
        {
            // Search for row hit
            Row openRow = bankMachine->getOpenRow();
            for (auto it = buffer[bankID].begin(); it != buffer[bankID].end(); it++)
            {
                if (DramExtension::getRow(*it) == openRow)
                    return *it;
            }
        }
        // No row hit found or bank precharged
        return buffer[bankID].front();
    }
    return nullptr;
}

bool SchedulerFrFcfs::hasFurtherRowHit(Bank bank, Row row)
{
    unsigned rowHitCounter = 0;
    for (auto it = buffer[bank.ID()].begin(); it != buffer[bank.ID()].end(); it++)
    {
        if (DramExtension::getRow(*it) == row)
        {
            rowHitCounter++;
            if (rowHitCounter == 2)
                return true;
        }
    }
    return false;
}

bool SchedulerFrFcfs::hasFurtherRequest(Bank bank)
{
    if (buffer[bank.ID()].size() >= 2)
        return true;
    else
        return false;
}
