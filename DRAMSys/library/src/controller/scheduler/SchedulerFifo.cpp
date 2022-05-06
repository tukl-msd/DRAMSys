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
#include "../../configuration/Configuration.h"
#include "BufferCounterBankwise.h"
#include "BufferCounterReadWrite.h"
#include "BufferCounterShared.h"

using namespace tlm;

SchedulerFifo::SchedulerFifo(const Configuration& config)
{
    buffer = std::vector<std::deque<tlm_generic_payload *>>(config.memSpec->banksPerChannel);

    if (config.schedulerBuffer == Configuration::SchedulerBuffer::Bankwise)
        bufferCounter = std::make_unique<BufferCounterBankwise>(config.requestBufferSize, config.memSpec->banksPerChannel);
    else if (config.schedulerBuffer == Configuration::SchedulerBuffer::ReadWrite)
        bufferCounter = std::make_unique<BufferCounterReadWrite>(config.requestBufferSize);
    else if (config.schedulerBuffer == Configuration::SchedulerBuffer::Shared)
        bufferCounter = std::make_unique<BufferCounterShared>(config.requestBufferSize);
}

bool SchedulerFifo::hasBufferSpace() const
{
    return bufferCounter->hasBufferSpace();
}

void SchedulerFifo::storeRequest(tlm_generic_payload& payload)
{
    buffer[DramExtension::getBank(payload).ID()].push_back(&payload);
    bufferCounter->storeRequest(payload);
}

void SchedulerFifo::removeRequest(tlm_generic_payload& payload)
{
    buffer[DramExtension::getBank(payload).ID()].pop_front();
    bufferCounter->removeRequest(payload);
}

tlm_generic_payload *SchedulerFifo::getNextRequest(const BankMachine& bankMachine) const
{
    unsigned bankID = bankMachine.getBank().ID();
    if (!buffer[bankID].empty())
        return buffer[bankID].front();
    else
        return nullptr;
}

bool SchedulerFifo::hasFurtherRowHit(Bank bank, Row row, tlm_command command) const
{
    if (buffer[bank.ID()].size() >= 2)
    {
        tlm_generic_payload *nextRequest = buffer[bank.ID()][1];
        if (DramExtension::getRow(nextRequest) == row)
            return true;
    }
    return false;
}

bool SchedulerFifo::hasFurtherRequest(Bank bank, tlm_command command) const
{
    if (buffer[bank.ID()].size() >= 2)
        return true;
    else
        return false;
}

const std::vector<unsigned> &SchedulerFifo::getBufferDepth() const
{
    return bufferCounter->getBufferDepth();
}
