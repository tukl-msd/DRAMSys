/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 */

#ifndef REORDERBUFFER_H
#define REORDERBUFFER_H

#include <deque>
#include <set>
#include <systemc>
#include <tlm>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

namespace DRAMSys
{

struct ReorderBuffer : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<ReorderBuffer> iSocket;
    tlm_utils::simple_target_socket<ReorderBuffer> tSocket;

    SC_CTOR(ReorderBuffer) : payloadEventQueue(this, &ReorderBuffer::peqCallback)
    {
        iSocket.register_nb_transport_bw(this, &ReorderBuffer::nb_transport_bw);
        tSocket.register_nb_transport_fw(this, &ReorderBuffer::nb_transport_fw);
    }

private:
    tlm_utils::peq_with_cb_and_phase<ReorderBuffer> payloadEventQueue;
    std::deque<tlm::tlm_generic_payload*> pendingRequestsInOrder;
    std::set<tlm::tlm_generic_payload*> receivedResponses;

    bool responseIsPendingInInitator = false;

    // Initiated by dram side
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& bwDelay)
    {
        payloadEventQueue.notify(trans, phase, bwDelay);
        return tlm::TLM_ACCEPTED;
    }

    // Initiated by initator side (players)
    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& fwDelay)
    {
        if (phase == tlm::BEGIN_REQ)
        {
            trans.acquire();
        }
        else if (phase == tlm::END_RESP)
        {
            trans.release();
        }

        payloadEventQueue.notify(trans, phase, fwDelay);
        return tlm::TLM_ACCEPTED;
    }

    void peqCallback(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase)
    {
        // Phases initiated by initiator side
        if (phase == tlm::BEGIN_REQ)
        {
            pendingRequestsInOrder.push_back(&trans);
            sendToTarget(trans, phase, sc_core::SC_ZERO_TIME);
        }

        else if (phase == tlm::END_RESP)
        {
            responseIsPendingInInitator = false;
            pendingRequestsInOrder.pop_front();
            receivedResponses.erase(&trans);
            sendNextResponse();
        }

        // Phases initiated by dram side
        else if (phase == tlm::END_REQ)
        {
            sendToInitiator(trans, phase, sc_core::SC_ZERO_TIME);
        }
        else if (phase == tlm::BEGIN_RESP)
        {
            sendToTarget(trans, tlm::END_RESP, sc_core::SC_ZERO_TIME);
            receivedResponses.emplace(&trans);
            sendNextResponse();
        }

        else
        {
            SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
        }
    }

    void sendToTarget(tlm::tlm_generic_payload& trans,
                      const tlm::tlm_phase& phase,
                      const sc_core::sc_time& delay)
    {
        tlm::tlm_phase TPhase = phase;
        sc_core::sc_time TDelay = delay;
        iSocket->nb_transport_fw(trans, TPhase, TDelay);
    }

    void sendToInitiator(tlm::tlm_generic_payload& trans,
                         const tlm::tlm_phase& phase,
                         const sc_core::sc_time& delay)
    {

        sc_assert(phase == tlm::END_REQ ||
                  (phase == tlm::BEGIN_RESP && pendingRequestsInOrder.front() == &trans &&
                   receivedResponses.count(&trans)));

        tlm::tlm_phase TPhase = phase;
        sc_core::sc_time TDelay = delay;
        tSocket->nb_transport_bw(trans, TPhase, TDelay);
    }

    void sendNextResponse()
    {
        // only send the next response when there response for the oldest pending request
        // (requestsInOrder.front()) has been received
        if (!responseIsPendingInInitator &&
            (receivedResponses.count(pendingRequestsInOrder.front()) != 0))
        {
            tlm::tlm_generic_payload* payloadToSend = pendingRequestsInOrder.front();
            responseIsPendingInInitator = true;
            sendToInitiator(*payloadToSend, tlm::BEGIN_RESP, sc_core::SC_ZERO_TIME);
        }
        //        else if(!responseIsPendingInInitator && receivedResponses.size()>0 &&
        //        !receivedResponses.count(pendingRequestsInOrder.front())>0)
        //        {
        //            cout << "cant send this response, because we are still waiting for response of
        //            oldest pending request. Elemts in buffer: " <<  receivedResponses.size() <<
        //            endl;
        //        }
    }
};

} // namespace DRAMSys

#endif // REORDERBUFFER_H
