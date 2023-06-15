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

#ifndef SCHEDULERFRFCFSGRP_H
#define SCHEDULERFRFCFSGRP_H

#include "DRAMSys/controller/scheduler/SchedulerIF.h"
#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/scheduler/BufferCounterIF.h"

#include <vector>
#include <list>
#include <memory>
#include <tlm>

namespace DRAMSys
{

class SchedulerFrFcfsGrp final : public SchedulerIF
{
public:
    explicit SchedulerFrFcfsGrp(const Configuration& config);
    [[nodiscard]] bool hasBufferSpace() const override;
    void storeRequest(tlm::tlm_generic_payload&) override;
    void removeRequest(tlm::tlm_generic_payload&) override;
    [[nodiscard]] tlm::tlm_generic_payload* getNextRequest(const BankMachine&) const override;
    [[nodiscard]] bool hasFurtherRowHit(Bank, Row, tlm::tlm_command) const override;
    [[nodiscard]] bool hasFurtherRequest(Bank, tlm::tlm_command) const override;
    [[nodiscard]] const std::vector<unsigned>& getBufferDepth() const override;

private:
    std::vector<std::list<tlm::tlm_generic_payload *>> buffer;
    tlm::tlm_command lastCommand = tlm::TLM_READ_COMMAND;
    std::unique_ptr<BufferCounterIF> bufferCounter;
};

} // namespace DRAMSys

#endif // SCHEDULERFRFCFSGRP_H