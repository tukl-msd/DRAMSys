/*
 * Copyright (c) 2022, RPTU Kaiserslautern-Landau
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
 *    Christian Malek
 *    Derek Christ
 */

#include "InlineEcc.h"

#include "OrinScheme.h"
#include "TwoLevelScheme.h"

using namespace sc_core;
using namespace tlm;

DECLARE_EXTENDED_PHASE(INTERNAL);

namespace DRAMSys
{

InlineEcc::InlineEcc(sc_module_name const& name,
                     Config::InlineEccType inlineEccType,
                     std::shared_ptr<const AddressDecoder> addressDecoder,
                     sc_core::sc_time tCK,
                     unsigned int atomSize) :
    sc_core::sc_module(name),
    payloadEventQueue(this, &InlineEcc::peqCallback),
    tCK(tCK),
    addressDecoder(addressDecoder),
    eccScheme(createEccScheme(inlineEccType, addressDecoder, atomSize))
{
    iSocket.register_nb_transport_bw(this, &InlineEcc::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &InlineEcc::nb_transport_fw);
}

std::unique_ptr<EccScheme>
InlineEcc::createEccScheme(Config::InlineEccType inlineEccType,
                           std::shared_ptr<const AddressDecoder> addressDecoder,
                           unsigned int atomSize)
{
    if (inlineEccType == Config::InlineEccType::Orin)
        return std::make_unique<OrinScheme>(addressDecoder, atomSize);

    if (inlineEccType == Config::InlineEccType::TwoLevel)
        return std::make_unique<TwoLevelScheme>(addressDecoder, atomSize);

    return {};
}

tlm::tlm_sync_enum InlineEcc::nb_transport_fw(tlm::tlm_generic_payload& payload,
                                              tlm::tlm_phase& phase,
                                              sc_core::sc_time& fwDelay)
{
    if (phase == BEGIN_REQ)
    {
        payload.acquire();
    }

    payloadEventQueue.notify(payload, phase, fwDelay);
    return TLM_ACCEPTED;
}

tlm::tlm_sync_enum InlineEcc::nb_transport_bw(tlm::tlm_generic_payload& payload,
                                              tlm::tlm_phase& phase,
                                              sc_core::sc_time& bwDelay)
{
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

void InlineEcc::peqCallback(tlm::tlm_generic_payload& cbPayload, const tlm::tlm_phase& cbPhase)
{
    if (cbPhase == BEGIN_REQ)
    {
        assert(!backPressureRequest);
        assert(!pendingRequest);

        // Do not accept requests as long as target is busy
        if (targetBusy)
        {
            backPressureRequest = &cbPayload;
            return;
        }

        bool registered = eccScheme->registerRequest(cbPayload);

        if (!registered)
        {
            pendingRequest = &cbPayload;
            return;
        }

        auto* request = eccScheme->getNextRequest();

        // Should never be nullptr
        assert(request);

        tlm_phase fwPhase = BEGIN_REQ;
        sc_time fwDelay = SC_ZERO_TIME;
        iSocket->nb_transport_fw(*request, fwPhase, fwDelay);
        targetBusy = true;

        tlm_phase bwPhase = END_REQ;
        sc_time bwDelay = SC_ZERO_TIME;

        tSocket->nb_transport_bw(cbPayload, bwPhase, bwDelay);
    }
    else if (cbPhase == END_REQ)
    {
        targetBusy = false;

        auto* request = eccScheme->getNextRequest();

        if (request != nullptr)
        {
            tlm_phase fwPhase = BEGIN_REQ;
            sc_time fwDelay = tCK;

            iSocket->nb_transport_fw(*request, fwPhase, fwDelay);
            targetBusy = true;
            return;
        }

        if (backPressureRequest != nullptr)
        {
            payloadEventQueue.notify(*backPressureRequest, BEGIN_REQ, tCK);
            backPressureRequest = nullptr;
            return;
        }
    }
    else if (cbPhase == BEGIN_RESP)
    {
        // Send END_RESP to target
        tlm_phase fwPhase = END_RESP;
        sc_time fwDelay = SC_ZERO_TIME;
        iSocket->nb_transport_fw(cbPayload, fwPhase, fwDelay);

        bool forward = eccScheme->registerResponse(&cbPayload);
        if (forward)
        {
            if (initiatorBusy)
            {
                pendingResponse = &cbPayload;
                return;
            }

            assert(!initiatorBusy);
            initiatorBusy = true;

            tlm_phase bwPhase = BEGIN_RESP;
            sc_time bwDelay = SC_ZERO_TIME;

            tlm_sync_enum returnValue = tSocket->nb_transport_bw(cbPayload, bwPhase, bwDelay);

            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
            {
                payloadEventQueue.notify(cbPayload, bwPhase, bwDelay);
            }
        }
        else
        {
            auto* response = eccScheme->getNextResponse();
            if (response != nullptr)
            {
                if (initiatorBusy)
                {
                    assert(pendingResponse == nullptr);
                    pendingResponse = &cbPayload;
                    return;
                }
                assert(!initiatorBusy);
                sendResponse(response);
            }
        }

        // Could be that response cleared up a write back, try to trigger again...
        if (!targetBusy)
        {
            auto* request = eccScheme->getNextRequest();

            if (request != nullptr)
            {
                tlm_phase fwPhase = BEGIN_REQ;
                sc_time fwDelay = tCK;

                iSocket->nb_transport_fw(*request, fwPhase, fwDelay);
                targetBusy = true;
                return;
            }
        }

        if (pendingRequest != nullptr)
        {
            payloadEventQueue.notify(*pendingRequest, BEGIN_REQ, SC_ZERO_TIME);
            pendingRequest = nullptr;
        }
    }
    else if (cbPhase == END_RESP)
    {
        assert(initiatorBusy);
        initiatorBusy = false;
        // cbPayload.release();

        if (pendingResponse != nullptr)
        {
            sendResponse(pendingResponse);

            pendingResponse = nullptr;
            return;
        }

        // Check if there are pending responses
        auto* response = eccScheme->getNextResponse();

        if (response != nullptr)
        {
            sendResponse(response);
            return;
        }

        if (pendingRequest != nullptr)
        {
            payloadEventQueue.notify(*pendingRequest, BEGIN_REQ, SC_ZERO_TIME);
            pendingRequest = nullptr;
        }
    }
    else
    {
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
    }
}

void InlineEcc::sendResponse(tlm::tlm_generic_payload* payload)
{
    tlm_phase bwPhase = BEGIN_RESP;
    sc_time bwDelay = SC_ZERO_TIME;
    tlm_sync_enum returnValue = tSocket->nb_transport_bw(*payload, bwPhase, bwDelay);
    initiatorBusy = true;

    // Early completion from initiator
    if (returnValue == TLM_UPDATED)
    {
        payloadEventQueue.notify(*payload, bwPhase, bwDelay);
        // initiatorBusy = false; // TODO is this correct?
    }
}

} // namespace DRAMSys
