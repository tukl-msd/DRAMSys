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
 *    Robert Gernhardt
 *    Matthias Jung
 *    Eder F. Zulian
 *    Lukas Steiner
 *    Derek Christ
 */

#include "Arbiter.h"

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/config/DRAMSysConfiguration.h"
#include "DRAMSys/simulation/AddressDecoder.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

Arbiter::Arbiter(const sc_module_name& name,
                 const SimConfig& simConfig,
                 const McConfig& mcConfig,
                 const MemSpec& memSpec,
                 const AddressDecoder& addressDecoder) :
    sc_module(name),
    addressDecoder(addressDecoder),
    payloadEventQueue(this, &Arbiter::peqCallback),
    tCK(memSpec.tCK),
    arbitrationDelayFw(mcConfig.arbitrationDelayFw),
    arbitrationDelayBw(mcConfig.arbitrationDelayBw),
    bytesPerBeat(memSpec.dataBusWidth / 8),
    addressOffset(simConfig.addressOffset)
{
    iSocket.register_nb_transport_bw(this, &Arbiter::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &Arbiter::nb_transport_fw);
    tSocket.register_b_transport(this, &Arbiter::b_transport);
    tSocket.register_transport_dbg(this, &Arbiter::transport_dbg);
}

ArbiterSimple::ArbiterSimple(const sc_module_name& name,
                             const SimConfig& simConfig,
                             const McConfig& mcConfig,
                             const MemSpec& memSpec,
                             const AddressDecoder& addressDecoder) :
    Arbiter(name, simConfig, mcConfig, memSpec, addressDecoder)
{
}

ArbiterFifo::ArbiterFifo(const sc_module_name& name,
                         const SimConfig& simConfig,
                         const McConfig& mcConfig,
                         const MemSpec& memSpec,
                         const AddressDecoder& addressDecoder) :
    Arbiter(name, simConfig, mcConfig, memSpec, addressDecoder),
    maxActiveTransactionsPerThread(mcConfig.maxActiveTransactions)
{
}

ArbiterReorder::ArbiterReorder(const sc_module_name& name,
                               const SimConfig& simConfig,
                               const McConfig& mcConfig,
                               const MemSpec& memSpec,
                               const AddressDecoder& addressDecoder) :
    Arbiter(name, simConfig, mcConfig, memSpec, addressDecoder),
    maxActiveTransactions(mcConfig.maxActiveTransactions)
{
}

void Arbiter::end_of_elaboration()
{
    // initiator side
    threadIsBusy = ControllerVector<Thread, bool>(tSocket.size(), false);
    nextThreadPayloadIDToAppend = ControllerVector<Thread, std::uint64_t>(tSocket.size(), 1);

    // channel side
    channelIsBusy = ControllerVector<Channel, bool>(iSocket.size(), false);
    pendingRequestsOnChannel = ControllerVector<Channel, std::queue<tlm_generic_payload*>>(
        iSocket.size(), std::queue<tlm_generic_payload*>());
    nextChannelPayloadIDToAppend = ControllerVector<Channel, std::uint64_t>(iSocket.size(), 1);
}

void ArbiterSimple::end_of_elaboration()
{
    Arbiter::end_of_elaboration();

    // initiator side
    pendingResponsesOnThread = ControllerVector<Thread, std::queue<tlm_generic_payload*>>(
        tSocket.size(), std::queue<tlm_generic_payload*>());
}

void ArbiterFifo::end_of_elaboration()
{
    Arbiter::end_of_elaboration();

    // initiator side
    activeTransactionsOnThread = ControllerVector<Thread, unsigned int>(tSocket.size(), 0);
    outstandingEndReqOnThread =
        ControllerVector<Thread, tlm_generic_payload*>(tSocket.size(), nullptr);
    pendingResponsesOnThread = ControllerVector<Thread, std::queue<tlm_generic_payload*>>(
        tSocket.size(), std::queue<tlm_generic_payload*>());

    lastEndReqOnChannel = ControllerVector<Channel, sc_time>(iSocket.size(), sc_max_time());
    lastEndRespOnThread = ControllerVector<Thread, sc_time>(tSocket.size(), sc_max_time());
}

void ArbiterReorder::end_of_elaboration()
{
    Arbiter::end_of_elaboration();

    // initiator side
    activeTransactionsOnThread = ControllerVector<Thread, unsigned int>(tSocket.size(), 0);
    outstandingEndReqOnThread =
        ControllerVector<Thread, tlm_generic_payload*>(tSocket.size(), nullptr);
    pendingResponsesOnThread =
        ControllerVector<Thread, std::set<tlm_generic_payload*, ThreadPayloadIDCompare>>(
            tSocket.size(), std::set<tlm_generic_payload*, ThreadPayloadIDCompare>());
    nextThreadPayloadIDToReturn = ControllerVector<Thread, std::uint64_t>(tSocket.size(), 1);

    lastEndReqOnChannel = ControllerVector<Channel, sc_time>(iSocket.size(), sc_max_time());
    lastEndRespOnThread = ControllerVector<Thread, sc_time>(tSocket.size(), sc_max_time());
}

tlm_sync_enum
Arbiter::nb_transport_fw(int id, tlm_generic_payload& trans, tlm_phase& phase, sc_time& fwDelay)
{
    sc_time clockOffset = sc_time::from_value((sc_time_stamp() + fwDelay).value() % tCK.value());
    sc_time notDelay = (clockOffset == SC_ZERO_TIME) ? fwDelay : (fwDelay + tCK - clockOffset);

    if (phase == BEGIN_REQ)
    {
        // TODO: do not adjust address permanently
        // adjust address offset:
        uint64_t adjustedAddress = trans.get_address() - addressOffset;
        trans.set_address(adjustedAddress);

        unsigned channel = addressDecoder.decodeChannel(adjustedAddress);
        assert(addressDecoder.decodeChannel(adjustedAddress + trans.get_data_length() - 1) ==
               channel);
        ArbiterExtension::setAutoExtension(trans, Thread(id), Channel(channel));
        trans.acquire();
    }

    PRINTDEBUGMESSAGE(name(),
                      "[fw] " + getPhaseName(phase) + " notification in " + notDelay.to_string());
    payloadEventQueue.notify(trans, phase, notDelay);
    return TLM_ACCEPTED;
}

tlm_sync_enum Arbiter::nb_transport_bw([[maybe_unused]] int id,
                                       tlm_generic_payload& payload,
                                       tlm_phase& phase,
                                       sc_time& bwDelay)
{
    PRINTDEBUGMESSAGE(name(),
                      "[bw] " + getPhaseName(phase) + " notification in " + bwDelay.to_string());
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

void Arbiter::b_transport([[maybe_unused]] int id,
                          tlm::tlm_generic_payload& trans,
                          sc_core::sc_time& delay)
{
    trans.set_address(trans.get_address() - addressOffset);

    DecodedAddress decodedAddress = addressDecoder.decodeAddress(trans.get_address());
    iSocket[static_cast<int>(decodedAddress.channel)]->b_transport(trans, delay);
}

unsigned int Arbiter::transport_dbg([[maybe_unused]] int id, tlm::tlm_generic_payload& trans)
{
    trans.set_address(trans.get_address() - addressOffset);

    DecodedAddress decodedAddress = addressDecoder.decodeAddress(trans.get_address());
    return iSocket[static_cast<int>(decodedAddress.channel)]->transport_dbg(trans);
}

void ArbiterSimple::peqCallback(tlm_generic_payload& cbTrans, const tlm_phase& cbPhase)
{
    Thread thread = ArbiterExtension::getThread(cbTrans);
    Channel channel = ArbiterExtension::getChannel(cbTrans);

    if (cbPhase == BEGIN_REQ) // from initiator
    {
        ArbiterExtension::setIDAndTimeOfGeneration(
            cbTrans, nextThreadPayloadIDToAppend[thread]++, sc_time_stamp());

        if (!channelIsBusy[channel])
        {
            channelIsBusy[channel] = true;

            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = arbitrationDelayFw;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(cbTrans, tPhase, tDelay);
        }
        else
            pendingRequestsOnChannel[channel].push(&cbTrans);
    }
    else if (cbPhase == END_REQ) // from target
    {
        {
            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            tSocket[static_cast<int>(thread)]->nb_transport_bw(cbTrans, tPhase, tDelay);
        }

        if (!pendingRequestsOnChannel[channel].empty())
        {
            tlm_generic_payload& tPayload = *pendingRequestsOnChannel[channel].front();
            pendingRequestsOnChannel[channel].pop();
            tlm_phase tPhase = BEGIN_REQ;
            // do not send two requests in the same cycle
            sc_time tDelay = tCK + arbitrationDelayFw;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
        else
            channelIsBusy[channel] = false;
    }
    else if (cbPhase == BEGIN_RESP) // from memory controller
    {
        if (!threadIsBusy[thread])
        {
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = arbitrationDelayBw;

            tlm_sync_enum returnValue =
                tSocket[static_cast<int>(thread)]->nb_transport_bw(cbTrans, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(cbTrans, tPhase, tDelay);
            threadIsBusy[thread] = true;
        }
        else
            pendingResponsesOnThread[thread].push(&cbTrans);
    }
    else if (cbPhase == END_RESP) // from initiator
    {
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(cbTrans, tPhase, tDelay);
        }
        cbTrans.release();

        if (!pendingResponsesOnThread[thread].empty())
        {
            tlm_generic_payload& tPayload = *pendingResponsesOnThread[thread].front();
            pendingResponsesOnThread[thread].pop();
            tlm_phase tPhase = BEGIN_RESP;
            // do not send two responses in the same cycle
            sc_time tDelay = tCK + arbitrationDelayBw;

            tlm_sync_enum returnValue =
                tSocket[static_cast<int>(thread)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
        else
            threadIsBusy[thread] = false;
    }
    else
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
}

void ArbiterFifo::peqCallback(tlm_generic_payload& cbTrans, const tlm_phase& cbPhase)
{
    Thread thread = ArbiterExtension::getThread(cbTrans);
    Channel channel = ArbiterExtension::getChannel(cbTrans);

    if (cbPhase == BEGIN_REQ) // from initiator
    {
        if (activeTransactionsOnThread[thread] < maxActiveTransactionsPerThread)
        {
            activeTransactionsOnThread[thread]++;

            ArbiterExtension::setIDAndTimeOfGeneration(
                cbTrans, nextThreadPayloadIDToAppend[thread]++, sc_time_stamp());

            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            tSocket[static_cast<int>(thread)]->nb_transport_bw(cbTrans, tPhase, tDelay);

            payloadEventQueue.notify(cbTrans, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            outstandingEndReqOnThread[thread] = &cbTrans;
    }
    else if (cbPhase == END_REQ) // from memory controller
    {
        lastEndReqOnChannel[channel] = sc_time_stamp();

        if (!pendingRequestsOnChannel[channel].empty())
        {
            tlm_generic_payload& tPayload = *pendingRequestsOnChannel[channel].front();
            pendingRequestsOnChannel[channel].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = tCK;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
        else
            channelIsBusy[channel] = false;
    }
    else if (cbPhase == BEGIN_RESP) // from memory controller
    {
        // TODO: use early completion
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(cbTrans, tPhase, tDelay);
        }

        payloadEventQueue.notify(cbTrans, RESP_ARBITRATION, arbitrationDelayBw);
    }
    else if (cbPhase == END_RESP) // from initiator
    {
        lastEndRespOnThread[thread] = sc_time_stamp();
        cbTrans.release();

        if (outstandingEndReqOnThread[thread] != nullptr)
        {
            tlm_generic_payload& tPayload = *outstandingEndReqOnThread[thread];
            outstandingEndReqOnThread[thread] = nullptr;
            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            ArbiterExtension::setIDAndTimeOfGeneration(
                tPayload, nextThreadPayloadIDToAppend[thread]++, sc_time_stamp());

            tSocket[static_cast<int>(thread)]->nb_transport_bw(tPayload, tPhase, tDelay);

            payloadEventQueue.notify(tPayload, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            activeTransactionsOnThread[thread]--;

        if (!pendingResponsesOnThread[thread].empty())
        {
            tlm_generic_payload& tPayload = *pendingResponsesOnThread[thread].front();
            pendingResponsesOnThread[thread].pop();
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = tCK;

            tlm_sync_enum returnValue =
                tSocket[static_cast<int>(thread)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
        else
            threadIsBusy[thread] = false;
    }
    else if (cbPhase == REQ_ARBITRATION)
    {
        pendingRequestsOnChannel[channel].push(&cbTrans);

        if (!channelIsBusy[channel])
        {
            channelIsBusy[channel] = true;

            tlm_generic_payload& tPayload = *pendingRequestsOnChannel[channel].front();
            pendingRequestsOnChannel[channel].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = lastEndReqOnChannel[channel] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
    }
    else if (cbPhase == RESP_ARBITRATION)
    {
        pendingResponsesOnThread[thread].push(&cbTrans);

        if (!threadIsBusy[thread])
        {
            threadIsBusy[thread] = true;

            tlm_generic_payload& tPayload = *pendingResponsesOnThread[thread].front();
            pendingResponsesOnThread[thread].pop();
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = lastEndRespOnThread[thread] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

            tlm_sync_enum returnValue =
                tSocket[static_cast<int>(thread)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
    }
    else
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
}

void ArbiterReorder::peqCallback(tlm_generic_payload& cbTrans, const tlm_phase& cbPhase)
{
    Thread thread = ArbiterExtension::getThread(cbTrans);
    Channel channel = ArbiterExtension::getChannel(cbTrans);

    if (cbPhase == BEGIN_REQ) // from initiator
    {
        if (activeTransactionsOnThread[thread] < maxActiveTransactions)
        {
            activeTransactionsOnThread[thread]++;

            ArbiterExtension::setIDAndTimeOfGeneration(
                cbTrans, nextThreadPayloadIDToAppend[thread]++, sc_time_stamp());

            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            tSocket[static_cast<int>(thread)]->nb_transport_bw(cbTrans, tPhase, tDelay);

            payloadEventQueue.notify(cbTrans, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            outstandingEndReqOnThread[thread] = &cbTrans;
    }
    else if (cbPhase == END_REQ) // from memory controller
    {
        lastEndReqOnChannel[channel] = sc_time_stamp();

        if (!pendingRequestsOnChannel[channel].empty())
        {
            tlm_generic_payload& tPayload = *pendingRequestsOnChannel[channel].front();
            pendingRequestsOnChannel[channel].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = tCK;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
        else
            channelIsBusy[channel] = false;
    }
    else if (cbPhase == BEGIN_RESP) // from memory controller
    {
        // TODO: use early completion
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;
            iSocket[static_cast<int>(channel)]->nb_transport_fw(cbTrans, tPhase, tDelay);
        }

        payloadEventQueue.notify(cbTrans, RESP_ARBITRATION, arbitrationDelayBw);
    }
    else if (cbPhase == END_RESP) // from initiator
    {
        lastEndRespOnThread[thread] = sc_time_stamp();
        cbTrans.release();

        if (outstandingEndReqOnThread[thread] != nullptr)
        {
            tlm_generic_payload& tPayload = *outstandingEndReqOnThread[thread];
            outstandingEndReqOnThread[thread] = nullptr;
            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            ArbiterExtension::setIDAndTimeOfGeneration(
                tPayload, nextThreadPayloadIDToAppend[thread]++, sc_time_stamp());

            tSocket[static_cast<int>(thread)]->nb_transport_bw(tPayload, tPhase, tDelay);

            payloadEventQueue.notify(tPayload, REQ_ARBITRATION, arbitrationDelayFw);
        }
        else
            activeTransactionsOnThread[thread]--;

        tlm_generic_payload& tPayload = **pendingResponsesOnThread[thread].begin();

        if (!pendingResponsesOnThread[thread].empty() &&
            ArbiterExtension::getThreadPayloadID(tPayload) == nextThreadPayloadIDToReturn[thread])
        {
            nextThreadPayloadIDToReturn[thread]++;
            pendingResponsesOnThread[thread].erase(pendingResponsesOnThread[thread].begin());

            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = tCK;

            tlm_sync_enum returnValue =
                tSocket[static_cast<int>(thread)]->nb_transport_bw(tPayload, tPhase, tDelay);
            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
                payloadEventQueue.notify(tPayload, tPhase, tDelay);
        }
        else
            threadIsBusy[thread] = false;
    }
    else if (cbPhase == REQ_ARBITRATION)
    {
        pendingRequestsOnChannel[channel].push(&cbTrans);

        if (!channelIsBusy[channel])
        {
            channelIsBusy[channel] = true;

            tlm_generic_payload& tPayload = *pendingRequestsOnChannel[channel].front();
            pendingRequestsOnChannel[channel].pop();
            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = lastEndReqOnChannel[channel] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

            iSocket[static_cast<int>(channel)]->nb_transport_fw(tPayload, tPhase, tDelay);
        }
    }
    else if (cbPhase == RESP_ARBITRATION)
    {
        pendingResponsesOnThread[thread].insert(&cbTrans);

        if (!threadIsBusy[thread])
        {
            tlm_generic_payload& tPayload = **pendingResponsesOnThread[thread].begin();

            if (ArbiterExtension::getThreadPayloadID(tPayload) ==
                nextThreadPayloadIDToReturn[thread])
            {
                threadIsBusy[thread] = true;

                nextThreadPayloadIDToReturn[thread]++;
                pendingResponsesOnThread[thread].erase(pendingResponsesOnThread[thread].begin());
                tlm_phase tPhase = BEGIN_RESP;
                sc_time tDelay =
                    lastEndRespOnThread[thread] == sc_time_stamp() ? tCK : SC_ZERO_TIME;

                tlm_sync_enum returnValue =
                    tSocket[static_cast<int>(thread)]->nb_transport_bw(tPayload, tPhase, tDelay);
                // Early completion from initiator
                if (returnValue == TLM_UPDATED)
                    payloadEventQueue.notify(tPayload, tPhase, tDelay);
            }
        }
    }
    else
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
}

} // namespace DRAMSys
