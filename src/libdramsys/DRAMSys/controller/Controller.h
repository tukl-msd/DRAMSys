/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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

#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/Command.h"
#include "DRAMSys/controller/ControllerIF.h"
#include "DRAMSys/controller/checker/CheckerIF.h"
#include "DRAMSys/controller/cmdmux/CmdMuxIF.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerIF.h"
#include "DRAMSys/controller/refresh/RefreshManagerIF.h"
#include "DRAMSys/controller/respqueue/RespQueueIF.h"
#include "DRAMSys/simulation/AddressDecoder.h"

#include <stack>
#include <systemc>
#include <tlm>
#include <utility>
#include <vector>

namespace DRAMSys
{

class Controller : public ControllerIF
{
public:
    Controller(const sc_core::sc_module_name& name,
               const Configuration& config,
               const AddressDecoder& addressDecoder);
    SC_HAS_PROCESS(Controller);

    [[nodiscard]] bool idle() const override;
    void registerIdleCallback(std::function<void()> idleCallback);

protected:
    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) override;
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) override;
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) override;
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans) override;

    virtual void
    sendToFrontend(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay);

    virtual void controllerMethod();

    std::unique_ptr<SchedulerIF> scheduler;

    const sc_core::sc_time scMaxTime = sc_core::sc_max_time();
    const sc_core::sc_time thinkDelayFw;
    const sc_core::sc_time thinkDelayBw;
    const sc_core::sc_time phyDelayFw;
    const sc_core::sc_time phyDelayBw;
    const sc_core::sc_time blockingReadDelay;
    const sc_core::sc_time blockingWriteDelay;

private:
    unsigned totalNumberOfPayloads = 0;
    std::function<void()> idleCallback;
    ControllerVector<Rank, unsigned> ranksNumberOfPayloads;
    ReadyCommands readyCommands;

    ControllerVector<Bank, std::unique_ptr<BankMachine>> bankMachines;
    ControllerVector<Rank, ControllerVector<Bank, BankMachine*>> bankMachinesOnRank;
    std::unique_ptr<CmdMuxIF> cmdMux;
    std::unique_ptr<CheckerIF> checker;
    std::unique_ptr<RespQueueIF> respQueue;
    ControllerVector<Rank, std::unique_ptr<RefreshManagerIF>> refreshManagers;
    ControllerVector<Rank, std::unique_ptr<PowerDownManagerIF>> powerDownManagers;

    const AddressDecoder& addressDecoder;
    uint64_t nextChannelPayloadIDToAppend = 1;

    struct PayloadAndArrival
    {
        tlm::tlm_generic_payload* payload = nullptr;
        sc_core::sc_time arrival = sc_core::sc_max_time();
    } transToAcquire, transToRelease;

    void manageResponses();
    void manageRequests(const sc_core::sc_time& delay);

    sc_core::sc_event beginReqEvent, endRespEvent, controllerEvent, dataResponseEvent;

    const unsigned minBytesPerBurst;
    const unsigned maxBytesPerBurst;

    void createChildTranses(tlm::tlm_generic_payload& parentTrans);

    class MemoryManager : public tlm::tlm_mm_interface
    {
    public:
        MemoryManager() = default;
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager& operator=(MemoryManager&&) = delete;
        ~MemoryManager() override;

        tlm::tlm_generic_payload& allocate();
        void free(tlm::tlm_generic_payload* trans) override;

    private:
        std::stack<tlm::tlm_generic_payload*> freePayloads;
    } memoryManager;
};

} // namespace DRAMSys

#endif // CONTROLLER_H
