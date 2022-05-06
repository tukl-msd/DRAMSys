/*
 * Copyright (c) 2022, Technische Universität Kaiserslautern
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
 *    Lukas Steiner
 */

#include "LengthConverter.h"

using namespace sc_core;
using namespace tlm;

// TODO: return status, TLM_INCOMPLETE_RESPONSE, acquire + release

LengthConverter::LengthConverter(const sc_module_name &name, unsigned maxOutputLength, bool storageEnabled) :
    sc_module(name), payloadEventQueue(this, &LengthConverter::peqCallback), storageEnabled(storageEnabled),
    memoryManager(storageEnabled, maxOutputLength), maxOutputLength(maxOutputLength)
{
    iSocket.register_nb_transport_bw(this, &LengthConverter::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &LengthConverter::nb_transport_fw);
    tSocket.register_transport_dbg(this, &LengthConverter::transport_dbg);
}

tlm_sync_enum LengthConverter::nb_transport_fw(tlm_generic_payload& trans,
                              tlm_phase& phase, sc_time& fwDelay)
{
    if (phase == BEGIN_REQ)
        trans.acquire();

    payloadEventQueue.notify(trans, phase, fwDelay);
    return TLM_ACCEPTED;
}

tlm_sync_enum LengthConverter::nb_transport_bw(tlm_generic_payload &payload,
                              tlm_phase &phase, sc_time &bwDelay)
{
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

unsigned int LengthConverter::transport_dbg(tlm_generic_payload &trans)
{
    return iSocket->transport_dbg(trans);
}

void LengthConverter::peqCallback(tlm_generic_payload &cbTrans, const tlm_phase &cbPhase)
{
    if (cbPhase == BEGIN_REQ) // from initiator
    {
        if (cbTrans.get_data_length() <= maxOutputLength)
        {
            // pipe transaction through
            tlm_phase fwPhase = BEGIN_REQ;
            sc_time fwDelay = SC_ZERO_TIME;
            tlm_sync_enum returnStatus = iSocket->nb_transport_fw(cbTrans, fwPhase, fwDelay);
            // TODO: END_REQ/BEGIN_RESP shortcut
        }
        else
        {
            // split transaction up into multiple sub-transactions
            createChildTranses(&cbTrans);
            tlm_generic_payload* firstChildTrans = cbTrans.get_extension<ParentExtension>()->getNextChildTrans();
            tlm_phase fwPhase = BEGIN_REQ;
            sc_time fwDelay = SC_ZERO_TIME;
            tlm_sync_enum returnStatus = iSocket->nb_transport_fw(*firstChildTrans, fwPhase, fwDelay);
        }
    }
    else if (cbPhase == END_REQ)
    {
        if (ChildExtension::isChildTrans(&cbTrans))
        {
            tlm_generic_payload* nextChildTrans = cbTrans.get_extension<ChildExtension>()->getNextChildTrans();
            if (nextChildTrans != nullptr)
            {
                tlm_phase fwPhase = BEGIN_REQ;
                //sc_time fwDelay = SC_ZERO_TIME;
                sc_time fwDelay = sc_time(1, SC_NS);
                tlm_sync_enum returnStatus = iSocket->nb_transport_fw(*nextChildTrans, fwPhase, fwDelay);
            }
            else
            {
                tlm_generic_payload* parentTrans = cbTrans.get_extension<ChildExtension>()->getParentTrans();
                tlm_phase bwPhase = END_REQ;
                sc_time bwDelay = SC_ZERO_TIME;
                tlm_sync_enum returnStatus = tSocket->nb_transport_bw(*parentTrans, bwPhase, bwDelay);
            }
        }
        else
        {
            tlm_phase bwPhase = END_REQ;
            sc_time bwDelay = SC_ZERO_TIME;
            tlm_sync_enum returnStatus = tSocket->nb_transport_bw(cbTrans, bwPhase, bwDelay);
        }
    }
    else if (cbPhase == BEGIN_RESP)
    {
        if (ChildExtension::isChildTrans(&cbTrans))
        {
            {
                tlm_phase fwPhase = END_RESP;
                sc_time fwDelay = SC_ZERO_TIME;
                tlm_sync_enum returnStatus = iSocket->nb_transport_fw(cbTrans, fwPhase, fwDelay);
            }

            if (storageEnabled && cbTrans.is_read())
            {
                tlm_generic_payload* parentTrans = cbTrans.get_extension<ChildExtension>()->getParentTrans();
                std::copy(cbTrans.get_data_ptr(), cbTrans.get_data_ptr() + maxOutputLength,
                        parentTrans->get_data_ptr() + (cbTrans.get_address() - parentTrans->get_address()));
            }

            if (cbTrans.get_extension<ChildExtension>()->notifyChildTransCompletion()) // all children finished
            {
                // BEGIN_RESP über tSocket
                tlm_generic_payload* parentTrans = cbTrans.get_extension<ChildExtension>()->getParentTrans();
                tlm_phase bwPhase = BEGIN_RESP;
                sc_time bwDelay = SC_ZERO_TIME;
                tlm_sync_enum returnStatus = tSocket->nb_transport_bw(*parentTrans, bwPhase, bwDelay);
            }
        }
        else
        {
            tlm_phase bwPhase = BEGIN_RESP;
            sc_time bwDelay = SC_ZERO_TIME;
            tlm_sync_enum returnStatus = tSocket->nb_transport_bw(cbTrans, bwPhase, bwDelay);
        }
    }
    else if (cbPhase == END_RESP)
    {
        if (ParentExtension::isParentTrans(&cbTrans))
        {
            cbTrans.get_extension<ParentExtension>()->releaseChildTranses();
        }
        else
        {
            tlm_phase fwPhase = END_RESP;
            sc_time fwDelay = SC_ZERO_TIME;
            tlm_sync_enum returnStatus = iSocket->nb_transport_fw(cbTrans, fwPhase, fwDelay);
        }
        cbTrans.release();
    }
    else
        SC_REPORT_FATAL(0, "Payload event queue in LengthConverter was triggered with unknown phase");
}

void LengthConverter::createChildTranses(tlm_generic_payload* parentTrans)
{
    unsigned numChildTranses = parentTrans->get_data_length() / maxOutputLength;
    std::vector<tlm_generic_payload*> childTranses;

    for (unsigned childId = 0; childId < numChildTranses; childId++)
    {
        tlm_generic_payload* childTrans = memoryManager.allocate();
        childTrans->acquire();
        childTrans->set_command(parentTrans->get_command());
        childTrans->set_address(parentTrans->get_address() + childId * maxOutputLength);
        childTrans->set_data_length(maxOutputLength);
        if (storageEnabled && parentTrans->is_write())
            std::copy(parentTrans->get_data_ptr() + childId * maxOutputLength, parentTrans->get_data_ptr() +
                    (childId + 1) * maxOutputLength, childTrans->get_data_ptr());
        ChildExtension::setExtension(childTrans, parentTrans);
        childTranses.push_back(childTrans);
    }
    ParentExtension::setExtension(parentTrans, std::move(childTranses));
}

LengthConverter::MemoryManager::MemoryManager(bool storageEnabled, unsigned maxDataLength)
        : storageEnabled(storageEnabled), maxDataLength(maxDataLength)
{}

LengthConverter::MemoryManager::~MemoryManager()
{
    while (!freePayloads.empty())
    {
        tlm_generic_payload* payload = freePayloads.top();
        if (storageEnabled)
            delete[] payload->get_data_ptr();
        payload->reset();
        delete payload;
        freePayloads.pop();
    }
}

tlm_generic_payload* LengthConverter::MemoryManager::allocate()
{
    if (freePayloads.empty())
    {
        auto* payload = new tlm_generic_payload(this);

        if (storageEnabled)
        {
            auto* data = new unsigned char[maxDataLength];
            payload->set_data_ptr(data);
        }
        return payload;
    }
    else
    {
        tlm_generic_payload* result = freePayloads.top();
        freePayloads.pop();
        return result;
    }
}

void LengthConverter::MemoryManager::free(tlm_generic_payload* payload)
{
    freePayloads.push(payload);
}