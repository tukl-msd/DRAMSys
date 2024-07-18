/*
 * Copyright (c) 2022, RPTU Kaiserslautern-Landau
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

#ifndef SCHEDULERGRPFRFCFS_H
#define SCHEDULERGRPFRFCFS_H

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/scheduler/BufferCounterIF.h"
#include "DRAMSys/controller/scheduler/SchedulerIF.h"

#include <list>
#include <memory>
#include <tlm>
#include <vector>

namespace DRAMSys
{

class SchedulerGrpFrFcfs final : public SchedulerIF
{
public:
    explicit SchedulerGrpFrFcfs(const McConfig& config, const MemSpec& memSpec);
    [[nodiscard]] bool hasBufferSpace() const override;
    void storeRequest(tlm::tlm_generic_payload& payload) override;
    void removeRequest(tlm::tlm_generic_payload& payload) override;
    [[nodiscard]] tlm::tlm_generic_payload*
    getNextRequest(const BankMachine& bankMachine) const override;
    [[nodiscard]] bool
    hasFurtherRowHit(Bank bank, Row row, tlm::tlm_command command) const override;
    [[nodiscard]] bool hasFurtherRequest(Bank bank, tlm::tlm_command command) const override;
    [[nodiscard]] const std::vector<unsigned>& getBufferDepth() const override;

private:
    ControllerVector<Bank, std::list<tlm::tlm_generic_payload*>> readBuffer;
    ControllerVector<Bank, std::list<tlm::tlm_generic_payload*>> writeBuffer;
    tlm::tlm_command lastCommand = tlm::TLM_READ_COMMAND;
    std::unique_ptr<BufferCounterIF> bufferCounter;
};

} // namespace DRAMSys

#endif // SCHEDULERGRPFRFCFS_H
