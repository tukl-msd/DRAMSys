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
 *    Lukas Steiner
 *    Derek Christ
 */

#include "Arbiter.h"
#include "AddressDecoder.h"
#include "../configuration/Configuration.h"

#include <Configuration.h>

using namespace sc_core;
using namespace tlm;

Arbiter::Arbiter(const sc_module_name &name, const Configuration& config,
                 const DRAMSysConfiguration::AddressMapping& addressMapping) :
    sc_module(name), addressDecoder(config, addressMapping), payloadEventQueue(this, &Arbiter::peqCallback),
    tCK(config.memSpec->tCK),
    arbitrationDelayFw(config.arbitrationDelayFw),
    arbitrationDelayBw(config.arbitrationDelayBw),
    bytesPerBeat(config.memSpec->dataBusWidth / 8),
    addressOffset(config.addressOffset)
{
    iSocket.register_nb_transport_bw(this, &Arbiter::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &Arbiter::nb_transport_fw);
    tSocket.register_transport_dbg(this, &Arbiter::transport_dbg);

    addressDecoder.print();
}

ArbiterSimple::ArbiterSimple(const sc_module_name& name, const Configuration& config,
                             const DRAMSysConfiguration::AddressMapping &addressMapping) :
    Arbiter(name, config, addressMapping) {}

ArbiterFifo::ArbiterFifo(const sc_module_name &name, const Configuration& config,
                         const DRAMSysConfiguration::AddressMapping &addressMapping) :
    Arbiter(name, config, addressMapping),
    maxActiveTransactions(config.maxActiveTransactions) {}

ArbiterReorder::ArbiterReorder(const sc_module_name &name, const Configuration& config,
                               const DRAMSysConfiguration::AddressMapping &addressMapping) :
    Arbiter(name, config, addressMapping),
    maxActiveTransactions(config.maxActiveTransactions) {}

void Arbiter::end_of_elaboration()
{
    // initiator side
    threadIsBusy = std::vector<bool>(tSocket.size(), false);
    nextThreadPayloadIDToAppend = std::vector<uint64_t>(tSocket.size(), 1);

    // channel side
    channelIsBusy = std::vector<bool>(iSocket.size(), false);
    pendingRequests = std::vector<std::queue<tlm_generic_payload *>>(iSocket.size(),
            std::queue<tlm_generic_payload *>());
    nextChannelPayloadIDToAppend = std::vector<uint64_t>(iSocket.size(), 1);
}

void ArbiterSimple::end_of_elaboration()
{
    Arbiter::end_of_elaboration();

    // initiator side
    pendingResponses = std::vector<std::queue<tlm_generic_payload *>>(tSocket.size(),
            std::queue<tlm_generic_payload *>());
}

void ArbiterFifo::end_of_elaboration()
{
    Arbiter::end_of_elaboration();

    // initiator side
    activeTransactions = std::vector<unsigned int>(tSocket.size(), 0);
    outstandingEndReq = std::vector<tlm_generic_payload *>(tSocket.size(), nullptr);
    pendingResponses = std::vector<std::queue<tlm_generic_payload *>>(tSocket.size(),
            std::queue<tlm_generic_payload *>());

    lastEndReq = std::vector<sc_time>(iSocket.size(), sc_max_time());
    lastEndResp = std::vector<sc_time>(tSocket.size(), sc_max_time());
}

void ArbiterReorder::end_of_elaboration()
{
    Arbiter::end_of_elaboration();

    // initiator side
    activeTransactions = std::vector<unsigned int>(tSocket.size(), 0);
    outstandingEndReq = std::vector<tlm_generic_payload *>(tSocket.size(), nullptr);
    pendingResponses = std::vector<std::set<tlm_generic_payload *, ThreadPayloadIDCompare>>
            (tSocket.size(), std::set<tlm_generic_payload *, ThreadPayloadIDCompare>());
    nextThreadPayloadIDToReturn = std::vector<uint64_t>(tSocket.size(), 1);

    lastEndReq = std::vector<sc_time>(iSocket.size(), sc_max_time());
    lastEndResp = std::vector<sc_time>(tSocket.size(), sc_max_time());
}

tlm_sync_enum Arbiter::nb_transport_fw(int id, tlm_generic_payload &payload,
                              tlm_phase &phase, sc_time &fwDelay)
{
    sc_time clockOffset = (sc_time_stamp() + fwDelay) % tCK;
    sc_time notDelay = (clockOffset == SC_ZERO_TIME) ? fwDelay : (fwDelay + tCK - clockOffset);

    if (phase == BEGIN_REQ)
    {
        // TODO: do not adjust address permanently
        // adjust address offset:
        uint64_t adjustedAddress = payload.get_address() - addressOffset;
        payload.set_address(adjustedAddress);

        DecodedAddress decodedAddress = addressDecoder.decodeAddress(adjustedAddress);
        DramExtension::setExtension(payload, Thread(static_cast<unsigned int>(id)), Channel(decodedAddress.channel),
                                    Rank(decodedAddress.rank),
                                    BankGroup(decodedAddress.bankgroup), Bank(decodedAddress.bank),
                                    Row(decodedAddress.row), Column(decodedAddress.column),
                                    payload.get_data_length() / bytesPerBeat, 0, 0);
        payload.acquire();
    }

    PRINTDEBUGMESSAGE(name(), "[fw] " + getPhaseName(phase) + " notification in " +
                      notDelay.to_string());
    payloadEventQueue.notify(payload, phase, notDelay);
    return TLM_ACCEPTED;
}

tlm_sync_enum Arbiter::nb_transport_bw(int, tlm_generic_payload &payload,
                              tlm_phase &phase, sc_time &bwDelay)
{
    PRINTDEBUGMESSAGE(name(), "[bw] " + getPhaseName(phase) + " notification in " +
                      bwDelay.to_string());
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

unsigned int Arbiter::transport_dbg(int /*id*/, tlm::tlm_generic_payload &trans)
{
    trans.set_address(trans.get_address() - addressOffset);

    DecodedAddress decodedAddress = addressDecoder.decodeAddress(trans.get_address());
    return iSocket[static_cast<int>(decodedAddress.channel)]->transport_dbg(trans);
}

void ArbiterSimple::peqCallback(tlm_generic_payload &cbPayload, const tlm_phase &cbPhase)
{
    unsigned int threadId = DramExtension::getExtension(cbPayload).getThread().ID();
    unsigned int channelId = DramExtension::getExtension(cbPayload).getChannel().ID();

    if (cbPhase == BEGIN_REQ) // from initiator
    {
        GenerationExtension::setExtension(cbPayload, sc_time_stamp());
        DramExtension::setPayloadIDs(cbPayload,
                nextThreadPayloadIDToAppend[threadId]++, nextChannelPayloadIDToAppend[channelId]++);

        if (!channelIsBusy[channelId])
        {
            channelIsBusy[channelId] = true;

            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = arbitrationDelayFw;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(cbPayload, tPhase, tDelay);
        }
        else
            pendingRequests[channelId].push(&cbPayload);
    }
    else if (cbPhase == END_REQ) // from target
    {
        {
            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            tSocket[static_cast<int>(threadId)]->nb_transport_bw(cbPayload, tPhase, tDelay);
        }

        if (!pendingRequests[channelId].empty())
        {
            tlm_generic_payload &tPayload = *pendingRequests[channelId].front();
            pendingRequests[channelId].pop();
            tlm_phase tPhase = BEGIN_REQ;
            // do not send two requests in the same cycle
            sc_time tDelay = tCK + arbitrationDelayFw;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
        else
            channelIsBusy[channelId] = false;
    }
    else if (cbPhase == BEGIN_RESP) // from memory controller
    {
        if (!threadIsBusy[threadId])
        {
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = arbitrationDelayBw;

            tlm_sync_enum returnValue = tSocket[static_cast<int>(threadId)]->nb_transport_bw(cbPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(cbPayload, tPhase, tDelay);
            threadIsBusy[threadId] = true;
        }
        else
            pendingResponses[threadId].push(&cbPayload);
    }
    else if (cbPhase == END_RESP) // from initiator
    {
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(cbPayload, tPhase, tDelay);
        }
        cbPayload.release();

        if (!pendingResponses[threadId].empty())
        {
            tlm_generic_payload &tPayload = *pendingResponses[threadId].front();
            pendingResponses[threadId].pop();
            tlm_phase tPhase = BEGIN_RESP;
            // do not send two responses in the same cycle
            sc_time tDelay = tCK + arbitrationDelayBw;

            tlm_sync_enum returnValue = tSocket[static_cast<int>(threadId)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
        else
            threadIsBusy[threadId] = false;
    }
    else
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
}

void ArbiterFifo::peqCallback(tlm_generic_payload &cbPayload, const tlm_phase &cbPhase)
{
    unsigned int threadId = DramExtension::getExtension(cbPayload).getThread().ID();
    unsigned int channelId = DramExtension::getExtension(cbPayload).getChannel().ID();

    if (cbPhase == BEGIN_REQ) // from initiator
    {
        if (activeTransactions[threadId] < maxActiveTransactions)
        {
            activeTransactions[threadId]++;

            GenerationExtension::setExtension(cbPayload, sc_time_stamp());
            DramExtension::setPayloadIDs(cbPayload,
                    nextThreadPayloadIDToAppend[threadId]++, nextChannelPayloadIDToAppend[channelId]++);

            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            tSocket[static_cast<int>(threadId)]->nb_transport_bw(cbPayload, tPhase, tDelay);

            payloadEventQueue.notify(cbPayload, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            outstandingEndReq[threadId] = &cbPayload;
    }
    else if (cbPhase == END_REQ) // from memory controller
    {
        lastEndReq[channelId] = sc_time_stamp();

        if (!pendingRequests[channelId].empty())
        {
            tlm_generic_payload &tPayload = *pendingRequests[channelId].front();
            pendingRequests[channelId].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = tCK;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
        else
            channelIsBusy[channelId] = false;
    }   
    else if (cbPhase == BEGIN_RESP) // from memory controller
    {
        // TODO: use early completion
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(cbPayload, tPhase, tDelay);
        }

        payloadEventQueue.notify(cbPayload, RESP_ARBITRATION, arbitrationDelayBw);
    }
    else if (cbPhase == END_RESP) // from initiator
    {
        lastEndResp[threadId] = sc_time_stamp();
        cbPayload.release();

        if (outstandingEndReq[threadId] != nullptr)
        {
            tlm_generic_payload &tPayload = *outstandingEndReq[threadId];
            outstandingEndReq[threadId] = nullptr;
            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;
            unsigned int tChannelId = DramExtension::getExtension(tPayload).getChannel().ID();

            GenerationExtension::setExtension(tPayload, sc_time_stamp());
            DramExtension::setPayloadIDs(tPayload,
                    nextThreadPayloadIDToAppend[threadId]++, nextChannelPayloadIDToAppend[tChannelId]++);

            tSocket[static_cast<int>(threadId)]->nb_transport_bw(tPayload, tPhase, tDelay);

            payloadEventQueue.notify(tPayload, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            activeTransactions[threadId]--;

        if (!pendingResponses[threadId].empty())
        {
            tlm_generic_payload &tPayload = *pendingResponses[threadId].front();
            pendingResponses[threadId].pop();
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = tCK;

            tlm_sync_enum returnValue = tSocket[static_cast<int>(threadId)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
        else
            threadIsBusy[threadId] = false;
    }
    else if (cbPhase == REQ_ARBITRATION)
    {
        pendingRequests[channelId].push(&cbPayload);

        if (!channelIsBusy[channelId])
        {
            channelIsBusy[channelId] = true;

            tlm_generic_payload &tPayload = *pendingRequests[channelId].front();
            pendingRequests[channelId].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = lastEndReq[channelId] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
    }
    else if (cbPhase == RESP_ARBITRATION)
    {
        pendingResponses[threadId].push(&cbPayload);

        if (!threadIsBusy[threadId])
        {
            threadIsBusy[threadId] = true;

            tlm_generic_payload &tPayload = *pendingResponses[threadId].front();
            pendingResponses[threadId].pop();
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = lastEndResp[threadId] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

            tlm_sync_enum returnValue = tSocket[static_cast<int>(threadId)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
    }
    else
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
}

void ArbiterReorder::peqCallback(tlm_generic_payload &cbPayload, const tlm_phase &cbPhase)
{
    unsigned int threadId = DramExtension::getExtension(cbPayload).getThread().ID();
    unsigned int channelId = DramExtension::getExtension(cbPayload).getChannel().ID();

    if (cbPhase == BEGIN_REQ) // from initiator
    {
        if (activeTransactions[threadId] < maxActiveTransactions)
        {
            activeTransactions[threadId]++;

            GenerationExtension::setExtension(cbPayload, sc_time_stamp());
            DramExtension::setPayloadIDs(cbPayload,
                    nextThreadPayloadIDToAppend[threadId]++, nextChannelPayloadIDToAppend[channelId]++);

            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            tSocket[static_cast<int>(threadId)]->nb_transport_bw(cbPayload, tPhase, tDelay);

            payloadEventQueue.notify(cbPayload, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            outstandingEndReq[threadId] = &cbPayload;
    }
    else if (cbPhase == END_REQ) // from memory controller
    {
        lastEndReq[channelId] = sc_time_stamp();

        if (!pendingRequests[channelId].empty())
        {
            tlm_generic_payload &tPayload = *pendingRequests[channelId].front();
            pendingRequests[channelId].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = tCK;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
        else
            channelIsBusy[channelId] = false;
    }
    else if (cbPhase == BEGIN_RESP) // from memory controller
    {
        // TODO: use early completion
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;
            iSocket[static_cast<int>(channelId)]->nb_transport_fw(cbPayload, tPhase, tDelay);
        }

        payloadEventQueue.notify(cbPayload, RESP_ARBITRATION, arbitrationDelayBw);
    }
    else if (cbPhase == END_RESP) // from initiator
    {
        lastEndResp[threadId] = sc_time_stamp();
        cbPayload.release();

        if (outstandingEndReq[threadId] != nullptr)
        {
            tlm_generic_payload &tPayload = *outstandingEndReq[threadId];
            outstandingEndReq[threadId] = nullptr;
            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;
            unsigned int tChannelId = DramExtension::getExtension(tPayload).getChannel().ID();

            GenerationExtension::setExtension(tPayload, sc_time_stamp());
            DramExtension::setPayloadIDs(tPayload,
                    nextThreadPayloadIDToAppend[threadId]++, nextChannelPayloadIDToAppend[tChannelId]++);

            tSocket[static_cast<int>(threadId)]->nb_transport_bw(tPayload, tPhase, tDelay);

            payloadEventQueue.notify(tPayload, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            activeTransactions[threadId]--;

        tlm_generic_payload &tPayload = **pendingResponses[threadId].begin();

        if (!pendingResponses[threadId].empty() &&
                DramExtension::getThreadPayloadID(tPayload) == nextThreadPayloadIDToReturn[threadId])
        {
            nextThreadPayloadIDToReturn[threadId]++;
            pendingResponses[threadId].erase(pendingResponses[threadId].begin());

            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = tCK;

            tlm_sync_enum returnValue = tSocket[static_cast<int>(threadId)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
        else
            threadIsBusy[threadId] = false;
    }
    else if (cbPhase == REQ_ARBITRATION)
    {
        pendingRequests[channelId].push(&cbPayload);

        if (!channelIsBusy[channelId])
        {
            channelIsBusy[channelId] = true;

            tlm_generic_payload &tPayload = *pendingRequests[channelId].front();
            pendingRequests[channelId].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = lastEndReq[channelId] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

            iSocket[static_cast<int>(channelId)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
    }
    else if (cbPhase == RESP_ARBITRATION)
    {
        pendingResponses[threadId].insert(&cbPayload);

        if (!threadIsBusy[threadId])
        {
            tlm_generic_payload &tPayload = **pendingResponses[threadId].begin();

            if (DramExtension::getThreadPayloadID(tPayload) == nextThreadPayloadIDToReturn[threadId])
            {
                threadIsBusy[threadId] = true;

                nextThreadPayloadIDToReturn[threadId]++;
                pendingResponses[threadId].erase(pendingResponses[threadId].begin());
                tlm_phase tPhase = BEGIN_RESP;
                sc_time tDelay = lastEndResp[threadId] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

                tlm_sync_enum returnValue = tSocket[static_cast<int>(threadId)]->nb_transport_bw(tPayload, tPhase, tDelay);
                // Early completion from initiator
                if (returnValue == TLM_UPDATED)
                    payloadEventQueue.notify(tPayload, tPhase, tDelay);
            }
        }
    }
    else
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
}
