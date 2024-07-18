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
 * Authors:
 *    Lukas Steiner
 *    Derek Christ
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "BankMachine.h"
#include "Command.h"
#include "McConfig.h"
#include "checker/CheckerIF.h"
#include "cmdmux/CmdMuxIF.h"
#include "powerdown/PowerDownManagerIF.h"
#include "refresh/RefreshManagerIF.h"
#include "respqueue/RespQueueIF.h"

#include <DRAMSys/common/DebugManager.h>
#include <DRAMSys/simulation/AddressDecoder.h>

#include <functional>
#include <stack>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <utility>
#include <vector>

namespace DRAMSys
{

class Controller : public sc_core::sc_module
{
public:
    tlm_utils::simple_target_socket<Controller> tSocket{"tSocket"};    // Arbiter side
    tlm_utils::simple_initiator_socket<Controller> iSocket{"iSocket"}; // DRAM side

    Controller(const sc_core::sc_module_name& name,
               const McConfig& config,
               const MemSpec& memSpec,
               const AddressDecoder& addressDecoder);
    SC_HAS_PROCESS(Controller);

    [[nodiscard]] bool idle() const { return totalNumberOfPayloads == 0; }
    void registerIdleCallback(std::function<void()> idleCallback);

protected:
    void end_of_simulation() override;

    virtual tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& delay);
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& delay);
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

    virtual void
    sendToFrontend(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay);

    virtual void controllerMethod();

    const McConfig& config;
    const MemSpec& memSpec;
    const AddressDecoder& addressDecoder;

    std::unique_ptr<SchedulerIF> scheduler;

    sc_core::sc_time scMaxTime = sc_core::sc_max_time();

    uint64_t numberOfBeatsServed = 0;
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

    class IdleTimeCollector
    {
    public:
        void start()
        {
            if (!isIdle)
            {
                PRINTDEBUGMESSAGE("IdleTimeCollector", "IDLE start");
                idleStart = sc_core::sc_time_stamp();
                isIdle = true;
            }
        }

        void end()
        {
            if (isIdle)
            {
                PRINTDEBUGMESSAGE("IdleTimeCollector", "IDLE end");
                idleTime += sc_core::sc_time_stamp() - idleStart;
                isIdle = false;
            }
        }

        sc_core::sc_time getIdleTime() { return idleTime; }

    private:
        bool isIdle = false;
        sc_core::sc_time idleTime = sc_core::SC_ZERO_TIME;
        sc_core::sc_time idleStart;
    } idleTimeCollector;
};

} // namespace DRAMSys

#endif // CONTROLLER_H
