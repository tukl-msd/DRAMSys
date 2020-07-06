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

#include "Arbiter.h"
#include "../common/AddressDecoder.h"
#include "../configuration/Configuration.h"

using namespace tlm;

Arbiter::Arbiter(sc_module_name name, std::string pathToAddressMapping) :
    sc_module(name), payloadEventQueue(this, &Arbiter::peqCallback)
{
    // The arbiter communicates with one or more memory unity through one or more sockets (one or more memory channels).
    // Each of the arbiter's initiator sockets is bound to a memory controller's target socket.
    // Anytime an transaction comes from a memory unity to the arbiter the "bw" callback is called.
    iSocket.register_nb_transport_bw(this, &Arbiter::nb_transport_bw);

    for (size_t i = 0; i < Configuration::getInstance().memSpec->numberOfChannels; ++i)
    {
        channelIsFree.push_back(true);
        pendingRequests.push_back(std::queue<tlm_generic_payload *>());
        nextPayloadID.push_back(0);
    }

    // One or more devices can accesss all the memory units through the arbiter.
    // Devices' initiator sockets are bound to arbiter's target sockets.
    // As soon the arbiter receives a request in any of its target sockets it should treat and forward it to the proper memory channel.
    tSocket.register_nb_transport_fw(this, &Arbiter::nb_transport_fw);

    tSocket.register_transport_dbg(this, &Arbiter::transport_dbg);

    addressDecoder = new AddressDecoder(pathToAddressMapping);
    addressDecoder->print();
}

// Initiated by initiator side
// This function is called when an arbiter's target socket receives a transaction from a device
tlm_sync_enum Arbiter::nb_transport_fw(int id, tlm_generic_payload &payload,
                              tlm_phase &phase, sc_time &fwDelay)
{
    sc_time notDelay = std::ceil((sc_time_stamp() + fwDelay) / Configuration::getInstance().memSpec->tCK)
            * Configuration::getInstance().memSpec->tCK - sc_time_stamp();

    if (phase == BEGIN_REQ)
    {
        // TODO: do not adjust address permanently
        // adjust address offset:
        payload.set_address(payload.get_address() -
                            Configuration::getInstance().addressOffset);

        // In the begin request phase the socket ID is appended to the payload.
        // It will extracted from the payload and used later.
        appendDramExtension(id, payload);
        payload.acquire();
    }
    else if (phase == END_RESP)
    {
        // TODO: why one additional cycle???
        notDelay += Configuration::getInstance().memSpec->tCK;
    }

    PRINTDEBUGMESSAGE(name(), "[fw] " + getPhaseName(phase) + " notification in " +
                      notDelay.to_string());
    payloadEventQueue.notify(payload, phase, notDelay);
    return TLM_ACCEPTED;
}

// Initiated by dram side
// This function is called when an arbiter's initiator socket receives a transaction from a memory controller
tlm_sync_enum Arbiter::nb_transport_bw(int channelId, tlm_generic_payload &payload,
                              tlm_phase &phase, sc_time &bwDelay)
{
    // Check channel ID
    assert((unsigned int)channelId == DramExtension::getExtension(payload).getChannel().ID());

    PRINTDEBUGMESSAGE(name(), "[bw] " + getPhaseName(phase) + " notification in " +
                      bwDelay.to_string());
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

unsigned int Arbiter::transport_dbg(int /*id*/, tlm::tlm_generic_payload &trans)
{
    // adjust address offset:
    trans.set_address(trans.get_address() -
                      Configuration::getInstance().addressOffset);

    DecodedAddress decodedAddress = addressDecoder->decodeAddress(trans.get_address());
    return iSocket[decodedAddress.channel]->transport_dbg(trans);
}

void Arbiter::peqCallback(tlm_generic_payload &payload, const tlm_phase &phase)
{
    unsigned int threadId = DramExtension::getExtension(payload).getThread().ID();
    unsigned int channelId = DramExtension::getExtension(payload).getChannel().ID();

    // Check the valid range of thread ID and channel Id
    // TODO: thread ID not checked
    assert(channelId < Configuration::getInstance().memSpec->numberOfChannels);

    // Phases initiated by the intiator side from arbiter's point of view (devices performing memory requests to the arbiter)
    if (phase == BEGIN_REQ)
    {
        if (channelIsFree[channelId])
        {
            // This channel was available. Forward the new transaction to the memory controller.
            channelIsFree[channelId] = false;
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = SC_ZERO_TIME;
            iSocket[channelId]->nb_transport_fw(payload, tPhase, tDelay);
            // TODO: early completion of channel controller!!!
        }
        else
        {
            // This channel is busy. Enqueue the new transaction which phase is BEGIN_REQ.
            pendingRequests[channelId].push(&payload);
        }
    }
    // Phases initiated by the target side from arbiter's point of view (memory side)
    else if (phase == END_REQ)
    {
        channelIsFree[channelId] = true;

        // The arbiter receives a transaction which phase is END_REQ from memory controller and forwards it to the requester device.
        tlm_phase tPhase = END_REQ;
        sc_time tDelay = SC_ZERO_TIME;
        tSocket[threadId]->nb_transport_bw(payload, tPhase, tDelay);

        // This channel is now free! Dispatch a new transaction (phase is BEGIN_REQ) from the queue, if any. Send it to the memory controller.
        if (!pendingRequests[channelId].empty())
        {
            // Send ONE of the enqueued new transactions (phase is BEGIN_REQ) through this channel.
            tlm_generic_payload &payloadToSend = *pendingRequests[channelId].front();
            pendingRequests[channelId].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = SC_ZERO_TIME;
            iSocket[channelId]->nb_transport_fw(payloadToSend, tPhase, tDelay);
            // TODO: early completion of channel controller
            // Mark the channel as busy again.
            channelIsFree[channelId] = false;
        }
    }
    else if (phase == BEGIN_RESP)
    {
        // The arbiter receives a transaction in BEGIN_RESP phase
        // (that came from the memory side) and forwards it to the requester
        // device
        if (pendingResponses[threadId].empty())
        {
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = SC_ZERO_TIME;
            tlm_sync_enum returnValue = tSocket[threadId]->nb_transport_bw(payload, tPhase, tDelay);
            if (returnValue != TLM_ACCEPTED)
            {
                tPhase = END_RESP;
                payloadEventQueue.notify(payload, tPhase, tDelay);
            }
        }

        // Enqueue the transaction in BEGIN_RESP phase until the initiator
        // device acknowledges it (phase changes to END_RESP).
        pendingResponses[threadId].push(&payload);
    }
    else if (phase == END_RESP)
    {
        // Send the END_RESP message to the memory
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;
            iSocket[channelId]->nb_transport_fw(payload, tPhase, tDelay);
        }
        // Drop one element of the queue of BEGIN_RESP from memory to this device
        pendingResponses[threadId].pop();
        payload.release();

        // Check if there are queued transactoins with phase BEGIN_RESP from memory to this device
        if (!pendingResponses[threadId].empty())
        {
            // The queue is not empty.
            tlm_generic_payload &payloadToSend = *pendingResponses[threadId].front();
            // Send ONE extra BEGIN_RESP to the device
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = SC_ZERO_TIME;
            tlm_sync_enum returnValue = tSocket[threadId]->nb_transport_bw(payloadToSend, tPhase, tDelay);
            if (returnValue != TLM_ACCEPTED)
            {
                tPhase = END_RESP;
                payloadEventQueue.notify(payloadToSend, tPhase, tDelay);
            }
        }
    }
    else
        SC_REPORT_FATAL(0,
            "Payload event queue in arbiter was triggered with unknown phase");
}

void Arbiter::appendDramExtension(int socketId, tlm_generic_payload &payload)
{
    // Append Generation Extension
    GenerationExtension *genExtension = new GenerationExtension(sc_time_stamp());
    payload.set_auto_extension(genExtension);

    unsigned int burstlength = payload.get_streaming_width();
    DecodedAddress decodedAddress = addressDecoder->decodeAddress(payload.get_address());
    DramExtension *extension = new DramExtension(Thread(socketId),
                                                 Channel(decodedAddress.channel), Rank(decodedAddress.rank),
                                                 BankGroup(decodedAddress.bankgroup), Bank(decodedAddress.bank),
                                                 Row(decodedAddress.row), Column(decodedAddress.column),
                                                 burstlength, nextPayloadID[decodedAddress.channel]++);
    payload.set_auto_extension(extension);
}
