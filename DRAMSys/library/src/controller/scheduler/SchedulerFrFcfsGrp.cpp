/*
 * Copyright (c) 2020, Technische Universität Kaiserslautern
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

#include "SchedulerFrFcfsGrp.h"
#include "../../configuration/Configuration.h"
#include "BufferCounterBankwise.h"
#include "BufferCounterReadWrite.h"
#include "BufferCounterShared.h"

using namespace tlm;

SchedulerFrFcfsGrp::SchedulerFrFcfsGrp()
{
    Configuration &config = Configuration::getInstance();
    buffer = std::vector<std::list<tlm_generic_payload *>>(config.memSpec->numberOfBanks);

    if (config.schedulerBuffer == Configuration::SchedulerBuffer::Bankwise)
        bufferCounter = new BufferCounterBankwise(config.requestBufferSize, config.memSpec->numberOfBanks);
    else if (config.schedulerBuffer == Configuration::SchedulerBuffer::ReadWrite)
        bufferCounter = new BufferCounterReadWrite(config.requestBufferSize);
    else if (config.schedulerBuffer == Configuration::SchedulerBuffer::Shared)
        bufferCounter = new BufferCounterShared(config.requestBufferSize);
}

SchedulerFrFcfsGrp::~SchedulerFrFcfsGrp()
{
    delete bufferCounter;
}

bool SchedulerFrFcfsGrp::hasBufferSpace() const
{
    return bufferCounter->hasBufferSpace();
}

void SchedulerFrFcfsGrp::storeRequest(tlm_generic_payload *payload)
{
    buffer[DramExtension::getBank(payload).ID()].push_back(payload);
    bufferCounter->storeRequest(payload);
}

void SchedulerFrFcfsGrp::removeRequest(tlm_generic_payload *payload)
{
    bufferCounter->removeRequest(payload);
    lastCommand = payload->get_command();
    unsigned bankID = DramExtension::getBank(payload).ID();
    for (auto it = buffer[bankID].begin(); it != buffer[bankID].end(); it++)
    {
        if (*it == payload)
        {
            buffer[bankID].erase(it);
            break;
        }
    }
}

tlm_generic_payload *SchedulerFrFcfsGrp::getNextRequest(BankMachine *bankMachine) const
{
    unsigned bankID = bankMachine->getBank().ID();
    if (!buffer[bankID].empty())
    {
        if (bankMachine->getState() == BankMachine::State::Activated)
        {
            // Filter all row hits
            Row openRow = bankMachine->getOpenRow();
            std::list<tlm_generic_payload *> rowHits;
            for (auto it = buffer[bankID].begin(); it != buffer[bankID].end(); it++)
            {
                if (DramExtension::getRow(*it) == openRow)
                    rowHits.push_back(*it);
            }

            if (!rowHits.empty())
            {
                for (auto outerIt = rowHits.begin(); outerIt != rowHits.end(); outerIt++)
                {
                    if ((*outerIt)->get_command() == lastCommand)
                    {
                        bool hazardDetected = false;
                        for (auto innerIt = rowHits.begin(); *innerIt != *outerIt; innerIt++)
                        {
                            if ((*outerIt)->get_address() == (*innerIt)->get_address())
                            {
                                hazardDetected = true;
                                break;
                            }
                        }
                        if (!hazardDetected)
                            return *outerIt;
                    }
                }
                // no rd/wr hit found -> take first row hit
                return *rowHits.begin();
            }
        }
        // No row hit found or bank precharged
        return buffer[bankID].front();
    }
    return nullptr;
}

bool SchedulerFrFcfsGrp::hasFurtherRowHit(Bank bank, Row row) const
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

bool SchedulerFrFcfsGrp::hasFurtherRequest(Bank bank) const
{
    if (buffer[bank.ID()].size() >= 2)
        return true;
    else
        return false;
}

const std::vector<unsigned> &SchedulerFrFcfsGrp::getBufferDepth() const
{
    return bufferCounter->getBufferDepth();
}
