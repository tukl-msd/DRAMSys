/*
 * Copyright (c) 2023, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#include "RequestIssuer.h"

RequestIssuer::RequestIssuer(sc_core::sc_module_name const& name,
                             MemoryManager& memoryManager,
                             unsigned int clkMhz,
                             std::optional<unsigned int> maxPendingReadRequests,
                             std::optional<unsigned int> maxPendingWriteRequests,
                             std::function<Request()> nextRequest,
                             std::function<void()> transactionFinished,
                             std::function<void()> terminate) :
    sc_module(name),
    payloadEventQueue(this, &RequestIssuer::peqCallback),
    memoryManager(memoryManager),
    clkPeriod(sc_core::sc_time(1.0 / static_cast<double>(clkMhz), sc_core::SC_US)),
    maxPendingReadRequests(maxPendingReadRequests),
    maxPendingWriteRequests(maxPendingWriteRequests),
    transactionFinished(std::move(transactionFinished)),
    terminate(std::move(terminate)),
    nextRequest(std::move(nextRequest))
{
    SC_THREAD(sendNextRequest);
    iSocket.register_nb_transport_bw(this, &RequestIssuer::nb_transport_bw);
}

void RequestIssuer::sendNextRequest()
{
    Request request = nextRequest();

    if (request.command == Request::Command::Stop)
    {
        finished = true;
        return;
    }

    tlm::tlm_generic_payload& payload = memoryManager.allocate(request.length);
    payload.acquire();
    payload.set_address(request.address);
    payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload.set_dmi_allowed(false);
    payload.set_byte_enable_length(0);
    payload.set_data_length(request.length);
    payload.set_streaming_width(request.length);
    payload.set_command(request.command == Request::Command::Read ? tlm::TLM_READ_COMMAND
                                                                  : tlm::TLM_WRITE_COMMAND);

    std::copy(request.data.cbegin(), request.data.cend(), payload.get_data_ptr());

    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    sc_core::sc_time delay = request.delay;

    sc_core::sc_time sendingTime = sc_core::sc_time_stamp() + delay;

    bool needsOffset = (sendingTime % clkPeriod) != sc_core::SC_ZERO_TIME;
    if (needsOffset)
    {
        sendingTime += clkPeriod;
        sendingTime -= sendingTime % clkPeriod;
    }

    if (sendingTime == lastEndRequest)
    {
        sendingTime += clkPeriod;
    }

    delay = sendingTime - sc_core::sc_time_stamp();
    iSocket->nb_transport_fw(payload, phase, delay);

    if (request.command == Request::Command::Read)
        pendingReadRequests++;
    else if (request.command == Request::Command::Write)
        pendingWriteRequests++;

    transactionsSent++;
}

bool RequestIssuer::nextRequestSendable() const
{
    // If either the maxPendingReadRequests or maxPendingWriteRequests
    // limit is reached, do not send next payload.
    if (maxPendingReadRequests.has_value() && pendingReadRequests >= maxPendingReadRequests.value())
        return false;

    if (maxPendingWriteRequests.has_value() &&
        pendingWriteRequests >= maxPendingWriteRequests.value())
        return false;

    return true;
}

void RequestIssuer::peqCallback(tlm::tlm_generic_payload& payload, const tlm::tlm_phase& phase)
{
    if (phase == tlm::END_REQ)
    {
        lastEndRequest = sc_core::sc_time_stamp();

        if (nextRequestSendable())
            sendNextRequest();
        else
            transactionPostponed = true;
    }
    else if (phase == tlm::BEGIN_RESP)
    {
        tlm::tlm_phase nextPhase = tlm::END_RESP;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        iSocket->nb_transport_fw(payload, nextPhase, delay);

        payload.release();

        transactionFinished();

        transactionsReceived++;

        if (payload.get_command() == tlm::TLM_READ_COMMAND)
            pendingReadRequests--;
        else if (payload.get_command() == tlm::TLM_WRITE_COMMAND)
            pendingWriteRequests--;

        // If the initiator wasn't able to send the next payload in the END_REQ phase, do it
        // now.
        if (transactionPostponed && nextRequestSendable())
        {
            sendNextRequest();
            transactionPostponed = false;
        }

        // If all answers were received:
        if (finished && transactionsSent == transactionsReceived)
            terminate();
    }
    else
    {
        SC_REPORT_FATAL("TrafficInitiator", "PEQ was triggered with unknown phase");
    }
}
