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


#include <vector>

#include <systemc>
#include <tlm>
#include "ControllerIF.h"
#include "Command.h"
#include "BankMachine.h"
#include "cmdmux/CmdMuxIF.h"
#include "checker/CheckerIF.h"
#include "refresh/RefreshManagerIF.h"
#include "powerdown/PowerDownManagerIF.h"
#include "respqueue/RespQueueIF.h"

class Controller : public ControllerIF
{
public:
    Controller(const sc_core::sc_module_name& name, const Configuration& config);
    SC_HAS_PROCESS(Controller);

protected:
    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) override;
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) override;
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans) override;

    virtual void sendToFrontend(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay);
    virtual void sendToDram(Command, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);

    virtual void controllerMethod();

    std::unique_ptr<SchedulerIF> scheduler;

    const sc_core::sc_time thinkDelayFw;
    const sc_core::sc_time thinkDelayBw;
    const sc_core::sc_time phyDelayFw;
    const sc_core::sc_time phyDelayBw;

private:
    unsigned totalNumberOfPayloads = 0;
    std::vector<unsigned> ranksNumberOfPayloads;
    ReadyCommands readyCommands;

    std::vector<std::unique_ptr<BankMachine>> bankMachines;
    std::vector<std::vector<BankMachine*>> bankMachinesOnRank;
    std::unique_ptr<CmdMuxIF> cmdMux;
    std::unique_ptr<CheckerIF> checker;
    std::unique_ptr<RespQueueIF> respQueue;
    std::vector<std::unique_ptr<RefreshManagerIF>> refreshManagers;
    std::vector<std::unique_ptr<PowerDownManagerIF>> powerDownManagers;

    struct Transaction
    {
        tlm::tlm_generic_payload *payload = nullptr;
        sc_core::sc_time time = sc_core::sc_max_time();
    } transToAcquire, transToRelease;

    void manageResponses();
    void manageRequests(const sc_core::sc_time &delay);

    sc_core::sc_event beginReqEvent, endRespEvent, controllerEvent, dataResponseEvent;
};

#endif // CONTROLLER_H
