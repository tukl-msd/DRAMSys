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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <queue>
#include <vector>
#include <utility>
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include "ControllerIF.h"
#include "../common/dramExtensions.h"
#include "BankMachine.h"
#include "cmdmux/CmdMuxIF.h"
#include "scheduler/SchedulerIF.h"
#include "../common/DebugManager.h"
#include "checker/CheckerIF.h"
#include "refresh/RefreshManagerIF.h"
#include "powerdown/PowerDownManagerIF.h"
#include "respqueue/RespQueueIF.h"

class BankMachine;
class SchedulerIF;
class PowerDownManagerStaggered;

class Controller : public ControllerIF
{
public:
    Controller(sc_module_name);
    SC_HAS_PROCESS(Controller);
    virtual ~Controller();

protected:
    virtual tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload &, tlm::tlm_phase &, sc_time &) override;
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload &, tlm::tlm_phase &, sc_time &) override;
    virtual unsigned int transport_dbg(tlm::tlm_generic_payload &) override;

    virtual void sendToFrontend(tlm::tlm_generic_payload *, tlm::tlm_phase);
    virtual void sendToDram(Command, tlm::tlm_generic_payload *);

private:
    unsigned totalNumberOfPayloads = 0;
    std::vector<unsigned> ranksNumberOfPayloads;

    MemSpec *memSpec;

    std::vector<BankMachine *> bankMachines;
    std::vector<std::vector<BankMachine *>> bankMachinesOnRank;
    CmdMuxIF *cmdMux;
    SchedulerIF *scheduler;
    CheckerIF *checker;
    RespQueueIF *respQueue;
    std::vector<RefreshManagerIF *> refreshManagers;
    std::vector<PowerDownManagerIF *> powerDownManagers;

    tlm::tlm_generic_payload *payloadToAcquire = nullptr;
    sc_time timeToAcquire = sc_max_time();
    tlm::tlm_generic_payload *payloadToRelease = nullptr;
    sc_time timeToRelease = sc_max_time();

    void finishBeginReq();
    void startEndReq();
    void startBeginResp();
    void finishEndResp();

    void controllerMethod();
    sc_event beginReqEvent, endRespEvent, controllerEvent, dataResponseEvent;
};

#endif // CONTROLLER_H
