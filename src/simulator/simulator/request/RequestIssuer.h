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

#pragma once

#include "Request.h"
#include "simulator/MemoryManager.h"

#include <systemc>
#include <tlm>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>

#include <optional>

class RequestIssuer : sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<RequestIssuer> iSocket;

    RequestIssuer(sc_core::sc_module_name const& name,
                  MemoryManager& memoryManager,
                  unsigned int clkMhz,
                  std::optional<unsigned int> maxPendingReadRequests,
                  std::optional<unsigned int> maxPendingWriteRequests,
                  std::function<Request()> nextRequest,
                  std::function<void()> transactionFinished,
                  std::function<void()> terminate);
    SC_HAS_PROCESS(RequestIssuer);

private:
    tlm_utils::peq_with_cb_and_phase<RequestIssuer> payloadEventQueue;
    MemoryManager& memoryManager;

    const sc_core::sc_time clkPeriod;

    bool transactionPostponed = false;
    bool finished = false;

    uint64_t transactionsSent = 0;
    uint64_t transactionsReceived = 0;
    sc_core::sc_time lastEndRequest = sc_core::sc_max_time();

    unsigned int pendingReadRequests = 0;
    unsigned int pendingWriteRequests = 0;
    const std::optional<unsigned int> maxPendingReadRequests;
    const std::optional<unsigned int> maxPendingWriteRequests;

    std::function<void()> transactionFinished;
    std::function<void()> terminate;
    std::function<Request()> nextRequest;

    void sendNextRequest();
    bool nextRequestSendable() const;

    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& payload,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& bwDelay)
    {
        payloadEventQueue.notify(payload, phase, bwDelay);
        return tlm::TLM_ACCEPTED;
    }

    void peqCallback(tlm::tlm_generic_payload& payload, const tlm::tlm_phase& phase);
};
