/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
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
 *    Robert Gernhardt
 *    Matthias Jung
 *    Eder F. Zulian
 */

#ifndef ARBITER_H
#define ARBITER_H

#include <tlm.h>
#include <systemc.h>
#include <iostream>
#include <vector>
#include <queue>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include "../common/AddressDecoder.h"
#include "../common/dramExtensions.h"

class Arbiter : public sc_module
{
public:
    tlm_utils::multi_passthrough_initiator_socket<Arbiter> iSocket;
    tlm_utils::multi_passthrough_target_socket<Arbiter> tSocket;

    Arbiter(sc_module_name, std::string);
    SC_HAS_PROCESS(Arbiter);

private:
    AddressDecoder *addressDecoder;

    tlm_utils::peq_with_cb_and_phase<Arbiter> payloadEventQueue;

    std::vector<bool> channelIsFree;

    // used to account for the request_accept_delay in the dram controllers
    // This is a queue of new transactions. The phase of a new request is BEGIN_REQ.
    std::vector<std::queue<tlm::tlm_generic_payload *>> pendingRequests;
    // used to account for the response_accept_delay in the initiators (traceplayer, core etc.)
    // This is a queue of responses comming from the memory side. The phase of these transactions is BEGIN_RESP.
    std::map<unsigned int, std::queue<tlm::tlm_generic_payload *>> pendingResponses;

    // Initiated by initiator side
    // This function is called when an arbiter's target socket receives a transaction from a device
    tlm::tlm_sync_enum nb_transport_fw(int id, tlm::tlm_generic_payload &payload,
                                  tlm::tlm_phase &phase, sc_time &fwDelay);

    // Initiated by dram side
    // This function is called when an arbiter's initiator socket receives a transaction from a memory controller
    tlm::tlm_sync_enum nb_transport_bw(int channelId, tlm::tlm_generic_payload &payload,
                                  tlm::tlm_phase &phase, sc_time &bwDelay);

    unsigned int transport_dbg(int /*id*/, tlm::tlm_generic_payload &trans);

    void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase);

    void appendDramExtension(int socketId, tlm::tlm_generic_payload &payload);
    std::vector<uint64_t> nextPayloadID;
};

#endif // ARBITER_H
