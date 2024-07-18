/*
 * Copyright (c) 2020, RPTU Kaiserslautern-Landau
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

#include "DRAMSys/controller/scheduler/BufferCounterBankwise.h"
#include "DRAMSys/controller/scheduler/BufferCounterReadWrite.h"
#include "DRAMSys/controller/scheduler/BufferCounterShared.h"

using namespace tlm;

namespace DRAMSys
{

SchedulerFrFcfsGrp::SchedulerFrFcfsGrp(const McConfig& config, const MemSpec& memSpec)
{
    buffer = ControllerVector<Bank, std::list<tlm_generic_payload*>>(memSpec.banksPerChannel);

    if (config.schedulerBuffer == Config::SchedulerBufferType::Bankwise)
        bufferCounter = std::make_unique<BufferCounterBankwise>(config.requestBufferSize,
                                                                memSpec.banksPerChannel);
    else if (config.schedulerBuffer == Config::SchedulerBufferType::ReadWrite)
        bufferCounter = std::make_unique<BufferCounterReadWrite>(config.requestBufferSizeRead,
                                                                 config.requestBufferSizeWrite);
    else if (config.schedulerBuffer == Config::SchedulerBufferType::Shared)
        bufferCounter = std::make_unique<BufferCounterShared>(config.requestBufferSize);
}

bool SchedulerFrFcfsGrp::hasBufferSpace() const
{
    return bufferCounter->hasBufferSpace();
}

void SchedulerFrFcfsGrp::storeRequest(tlm_generic_payload& trans)
{
    buffer[ControllerExtension::getBank(trans)].push_back(&trans);
    bufferCounter->storeRequest(trans);
}

void SchedulerFrFcfsGrp::removeRequest(tlm_generic_payload& trans)
{
    bufferCounter->removeRequest(trans);
    lastCommand = trans.get_command();
    Bank bank = ControllerExtension::getBank(trans);
    for (auto it = buffer[bank].begin(); it != buffer[bank].end(); it++)
    {
        if (*it == &trans)
        {
            buffer[bank].erase(it);
            break;
        }
    }
}

tlm_generic_payload* SchedulerFrFcfsGrp::getNextRequest(const BankMachine& bankMachine) const
{
    Bank bank = bankMachine.getBank();
    if (!buffer[bank].empty())
    {
        if (bankMachine.isActivated())
        {
            // Filter all row hits
            Row openRow = bankMachine.getOpenRow();
            std::list<tlm_generic_payload*> rowHits;
            for (auto* it : buffer[bank])
            {
                if (ControllerExtension::getRow(*it) == openRow)
                    rowHits.push_back(it);
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
        return buffer[bank].front();
    }
    return nullptr;
}

bool SchedulerFrFcfsGrp::hasFurtherRowHit(Bank bank,
                                          Row row,
                                          [[maybe_unused]] tlm_command command) const
{
    unsigned rowHitCounter = 0;
    for (auto* it : buffer[bank])
    {
        if (ControllerExtension::getRow(*it) == row)
        {
            rowHitCounter++;
            if (rowHitCounter == 2)
                return true;
        }
    }
    return false;
}

bool SchedulerFrFcfsGrp::hasFurtherRequest(Bank bank, [[maybe_unused]] tlm_command command) const
{
    return buffer[bank].size() >= 2;
}

const std::vector<unsigned>& SchedulerFrFcfsGrp::getBufferDepth() const
{
    return bufferCounter->getBufferDepth();
}

} // namespace DRAMSys
