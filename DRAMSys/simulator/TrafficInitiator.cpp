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

#include "TrafficInitiator.h"
#include "TraceSetup.h"

using namespace sc_core;
using namespace tlm;

TrafficInitiator::TrafficInitiator(const sc_module_name &name, const Configuration& config, TraceSetup& setup,
        unsigned int maxPendingReadRequests, unsigned int maxPendingWriteRequests,
        unsigned int defaultDataLength, bool addLengthConverter) :
    sc_module(name),
    addLengthConverter(addLengthConverter),
    payloadEventQueue(this, &TrafficInitiator::peqCallback),
    setup(setup),
    maxPendingReadRequests(maxPendingReadRequests),
    maxPendingWriteRequests(maxPendingWriteRequests),
    defaultDataLength(defaultDataLength),
    storageEnabled(config.storeMode != Configuration::StoreMode::NoStorage),
    simulationProgressBar(config.simulationProgressBar)
{
    SC_THREAD(sendNextPayload);
    iSocket.register_nb_transport_bw(this, &TrafficInitiator::nb_transport_bw);
}

void TrafficInitiator::terminate()
{
    std::cout << sc_time_stamp() << " " << this->name() << " terminated " << std::endl;
    setup.trafficInitiatorTerminates();
}

tlm_sync_enum TrafficInitiator::nb_transport_bw(tlm_generic_payload &payload,
                                           tlm_phase &phase, sc_time &bwDelay)
{
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

void TrafficInitiator::peqCallback(tlm_generic_payload &payload,
                              const tlm_phase &phase)
{
    if (phase == END_REQ)
    {
        if (nextPayloadSendable())
            sendNextPayload();
        else
            payloadPostponed = true;
    }
    else if (phase == BEGIN_RESP)
    {
        payload.release();
        sendToTarget(payload, END_RESP, SC_ZERO_TIME);
        if (simulationProgressBar)
            setup.transactionFinished();

        transactionsReceived++;

        if (payload.get_command() == tlm::TLM_READ_COMMAND)
            pendingReadRequests--;
        else if (payload.get_command() == tlm::TLM_WRITE_COMMAND)
            pendingWriteRequests--;

        // If the initiator wasn't able to send the next payload in the END_REQ phase, do it now.
        if (payloadPostponed && nextPayloadSendable())
        {
            sendNextPayload();
            payloadPostponed = false;
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

void TrafficInitiator::sendToTarget(tlm_generic_payload &payload, const tlm_phase &phase, const sc_time &delay)
{
    tlm_phase TPhase = phase;
    sc_time TDelay = delay;
    iSocket->nb_transport_fw(payload, TPhase, TDelay);
}

bool TrafficInitiator::nextPayloadSendable() const
{
    // If either the maxPendingReadRequests or maxPendingWriteRequests
    // limit is reached, do not send next payload.
    if (((pendingReadRequests >= maxPendingReadRequests) && (maxPendingReadRequests != 0))
            || ((pendingWriteRequests >= maxPendingWriteRequests) && (maxPendingWriteRequests != 0)))
        return false;
    else
        return true;
}

sc_core::sc_time TrafficInitiator::evaluateGeneratorClk(const DRAMSysConfiguration::TrafficInitiator &conf)
{
    double frequencyMHz = conf.clkMhz;
    sc_time playerClk = sc_time(1.0 / frequencyMHz, SC_US);
    return playerClk;
}

