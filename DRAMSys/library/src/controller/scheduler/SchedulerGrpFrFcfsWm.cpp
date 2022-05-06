/*
 * Copyright (c) 2022, Technische Universität Kaiserslautern
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

#include "SchedulerGrpFrFcfsWm.h"
#include "../../configuration/Configuration.h"
#include "BufferCounterBankwise.h"
#include "BufferCounterReadWrite.h"
#include "BufferCounterShared.h"

using namespace tlm;

SchedulerGrpFrFcfsWm::SchedulerGrpFrFcfsWm(const Configuration& config)
    :lowWatermark(config.lowWatermark), highWatermark(config.highWatermark)
{
    readBuffer = std::vector<std::list<tlm_generic_payload *>>(config.memSpec->banksPerChannel);
    writeBuffer = std::vector<std::list<tlm_generic_payload *>>(config.memSpec->banksPerChannel);

    if (config.schedulerBuffer == Configuration::SchedulerBuffer::Bankwise)
        bufferCounter = std::make_unique<BufferCounterBankwise>(config.requestBufferSize, config.memSpec->banksPerChannel);
    else if (config.schedulerBuffer == Configuration::SchedulerBuffer::ReadWrite)
        bufferCounter = std::make_unique<BufferCounterReadWrite>(config.requestBufferSize);
    else if (config.schedulerBuffer == Configuration::SchedulerBuffer::Shared)
        bufferCounter = std::make_unique<BufferCounterShared>(config.requestBufferSize);

    if (lowWatermark == 0 || highWatermark == 0 || lowWatermark == highWatermark)
        SC_REPORT_FATAL("SchedulerGrpFrFcfsWm", "Invalid watermark configuration.");

    SC_REPORT_WARNING("SchedulerGrpFrFcfsWm", "Hazard detection not yet implemented!");
}

bool SchedulerGrpFrFcfsWm::hasBufferSpace() const
{
    return bufferCounter->hasBufferSpace();
}

void SchedulerGrpFrFcfsWm::storeRequest(tlm_generic_payload& trans)
{
    if (trans.is_read())
        readBuffer[DramExtension::getBank(trans).ID()].push_back(&trans);
    else
        writeBuffer[DramExtension::getBank(trans).ID()].push_back(&trans);
    bufferCounter->storeRequest(trans);
    evaluateWriteMode();
}

void SchedulerGrpFrFcfsWm::removeRequest(tlm_generic_payload& trans)
{
    bufferCounter->removeRequest(trans);
    unsigned bankID = DramExtension::getBank(trans).ID();

    if (trans.is_read())
        readBuffer[bankID].remove(&trans);
    else
        writeBuffer[bankID].remove(&trans);

    evaluateWriteMode();
}

tlm_generic_payload *SchedulerGrpFrFcfsWm::getNextRequest(const BankMachine& bankMachine) const
{
    unsigned bankID = bankMachine.getBank().ID();

    if (!writeMode)
    {
        if (!readBuffer[bankID].empty())
        {
            if (bankMachine.isActivated())
            {
                // Search for read row hit
                Row openRow = bankMachine.getOpenRow();
                for (auto it : readBuffer[bankID])
                {
                    if (DramExtension::getRow(it) == openRow)
                        return it;
                }
            }
            // No read row hit found or bank precharged
            return readBuffer[bankID].front();
        }
        else
            return nullptr;
    }
    else
    {
        if (!writeBuffer[bankID].empty())
        {
            if (bankMachine.isActivated())
            {
                // Search for write row hit
                Row openRow = bankMachine.getOpenRow();
                for (auto it : writeBuffer[bankID])
                {
                    if (DramExtension::getRow(it) == openRow)
                        return it;
                }
            }
            // No row hit found or bank precharged
            return writeBuffer[bankID].front();
        }
        else
            return nullptr;
    }
}

bool SchedulerGrpFrFcfsWm::hasFurtherRowHit(Bank bank, Row row, tlm::tlm_command command) const
{
    unsigned rowHitCounter = 0;
    if (!writeMode)
    {
        for (const auto* it : readBuffer[bank.ID()])
        {
            if (DramExtension::getRow(it) == row)
            {
                rowHitCounter++;
                if (rowHitCounter == 2)
                    return true;
            }
        }
        return false;
    }
    else
    {
        for (auto it : writeBuffer[bank.ID()])
        {
            if (DramExtension::getRow(it) == row)
            {
                rowHitCounter++;
                if (rowHitCounter == 2)
                    return true;
            }
        }
        return false;
    }
}

bool SchedulerGrpFrFcfsWm::hasFurtherRequest(Bank bank, tlm::tlm_command command) const
{
    if (!writeMode)
        return (readBuffer[bank.ID()].size() >= 2);
    else
        return (writeBuffer[bank.ID()].size() >= 2);
}

const std::vector<unsigned> &SchedulerGrpFrFcfsWm::getBufferDepth() const
{
    return bufferCounter->getBufferDepth();
}

void SchedulerGrpFrFcfsWm::evaluateWriteMode()
{
    if (writeMode)
    {
        if (bufferCounter->getNumWriteRequests() <= lowWatermark && bufferCounter->getNumReadRequests() != 0)
            writeMode = false;
    }
    else
    {
        if (bufferCounter->getNumWriteRequests() > highWatermark || bufferCounter->getNumReadRequests() == 0)
            writeMode = true;
    }
}