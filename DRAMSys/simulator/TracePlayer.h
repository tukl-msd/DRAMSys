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
 *    Éder F. Zulian
 *    Felipe S. Prado
 */

#ifndef TRACEPLAYER_H
#define TRACEPLAYER_H

#include <deque>
#include <tlm.h>
#include <systemc.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <iostream>
#include <string>
#include "configuration/Configuration.h"
#include "common/DebugManager.h"
#include "TraceSetup.h"

class TracePlayer : public sc_module
{
public:
    tlm_utils::simple_initiator_socket<TracePlayer> iSocket;
    TracePlayer(sc_module_name name, TraceSetup *setup);
    SC_HAS_PROCESS(TracePlayer);
    virtual void nextPayload() = 0;
    uint64_t getNumberOfLines(std::string pathToTrace);

protected:
    tlm_utils::peq_with_cb_and_phase<TracePlayer> payloadEventQueue;
    void terminate();
    bool storageEnabled = false;
    TraceSetup *setup;
    void sendToTarget(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase,
                      const sc_time &delay);
    uint64_t numberOfTransactions = 0;
    uint64_t transactionsSent = 0;
    bool finished = false;

private:
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload &payload, tlm::tlm_phase &phase,
                                  sc_time &bwDelay);
    void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase);
    uint64_t transactionsReceived = 0;
};

#endif // TRACEPLAYER_H
