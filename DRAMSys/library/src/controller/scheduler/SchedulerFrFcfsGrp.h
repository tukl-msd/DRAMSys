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

#ifndef SCHEDULERFRFCFSGRP_H
#define SCHEDULERFRFCFSGRP_H

#include <tlm.h>
#include <vector>
#include <list>

#include "SchedulerIF.h"
#include "../../common/dramExtensions.h"
#include "../BankMachine.h"

class SchedulerFrFcfsGrp : public SchedulerIF
{
public:
    SchedulerFrFcfsGrp();
    virtual bool hasBufferSpace() override;
    virtual void storeRequest(tlm::tlm_generic_payload *) override;
    virtual void removeRequest(tlm::tlm_generic_payload *) override;
    virtual tlm::tlm_generic_payload *getNextRequest(BankMachine *) override;
    virtual bool hasFurtherRowHit(Bank, Row) override;
    virtual bool hasFurtherRequest(Bank) override;
private:
    std::vector<std::list<tlm::tlm_generic_payload *>> buffer;
    unsigned requestBufferSize;
    tlm::tlm_command lastCommand = tlm::TLM_READ_COMMAND;
    unsigned lastBankID;
};

#endif // SCHEDULERFRFCFSGRP_H
