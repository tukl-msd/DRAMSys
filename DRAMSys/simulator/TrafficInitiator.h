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
 *    Derek Christ
 */

#ifndef TRAFFICINITIATOR_H
#define TRAFFICINITIATOR_H

#include <deque>
#include <iostream>
#include <string>

#include <tlm>
#include <systemc>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include "configuration/Configuration.h"
#include "common/DebugManager.h"
#include "TraceSetup.h"

class TrafficInitiator : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<TrafficInitiator> iSocket;
    TrafficInitiator(const sc_core::sc_module_name &name, const Configuration& config, TraceSetup& setup,
            unsigned int maxPendingReadRequests, unsigned int maxPendingWriteRequests,
            unsigned int defaultDataLength, bool addLengthConverter);
    SC_HAS_PROCESS(TrafficInitiator);
    virtual void sendNextPayload() = 0;
    const bool addLengthConverter = false;

protected:
    static sc_core::sc_time evaluateGeneratorClk(const DRAMSysConfiguration::TrafficInitiator &conf);

    tlm_utils::peq_with_cb_and_phase<TrafficInitiator> payloadEventQueue;
    void terminate();
    TraceSetup& setup;
    void sendToTarget(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase,
                      const sc_core::sc_time &delay);

    uint64_t transactionsReceived = 0;
    uint64_t transactionsSent = 0;
    unsigned int pendingReadRequests = 0;
    unsigned int pendingWriteRequests = 0;

    const unsigned int maxPendingReadRequests = 0;
    const unsigned int maxPendingWriteRequests = 0;

    bool payloadPostponed = false;
    bool finished = false;
    const unsigned int defaultDataLength;
    const bool storageEnabled;
    const bool simulationProgressBar;

    // 0 disables the max value.
    static constexpr unsigned int defaultMaxPendingWriteRequests = 0;
    static constexpr unsigned int defaultMaxPendingReadRequests = 0;

private:
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload &payload, tlm::tlm_phase &phase,
                                       sc_core::sc_time &bwDelay);
    void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase);
    bool nextPayloadSendable() const;
};

#endif // TRAFFICINITIATOR_H
