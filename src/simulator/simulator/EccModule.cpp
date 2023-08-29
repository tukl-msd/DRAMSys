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

#include "EccModule.h"

#include "DRAMSys/common/dramExtensions.h"

#include <fstream>

using namespace sc_core;
using namespace tlm;

EccModule::EccModule(sc_module_name name, DRAMSys::AddressDecoder const& addressDecoder) :
    sc_core::sc_module(name),
    payloadEventQueue(this, &EccModule::peqCallback),
    memoryManager(false),
    addressDecoder(addressDecoder)
{
    iSocket.register_nb_transport_bw(this, &EccModule::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &EccModule::nb_transport_fw);
}

tlm::tlm_sync_enum EccModule::nb_transport_fw(tlm::tlm_generic_payload& payload,
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

tlm::tlm_sync_enum EccModule::nb_transport_bw(tlm::tlm_generic_payload& payload,
                                              tlm::tlm_phase& phase,
                                              sc_core::sc_time& bwDelay)
{
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

void EccModule::peqCallback(tlm::tlm_generic_payload& cbPayload, const tlm::tlm_phase& cbPhase)
{
    if (cbPhase == BEGIN_REQ) // from initiator
    {
        // Put transaction into latency map
        payloadMap.emplace(&cbPayload, sc_time_stamp());

        if (!targetBusy)
        {
            targetBusy = true;

            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            DRAMSys::DecodedAddress decodedAddress =
                addressDecoder.decodeAddress(cbPayload.get_address());
            decodedAddress = calculateOffsetAddress(decodedAddress);

            // Update the original address to account for the offsets
            cbPayload.set_address(addressDecoder.encodeAddress(decodedAddress));

            // In case there is no entry yet.
            activeEccBlocks.try_emplace(decodedAddress.bank);

#ifdef ECC_ENABLE
            auto currentBlock = alignToBlock(decodedAddress.column);
            if (!activeEccBlock(decodedAddress.bank, decodedAddress.row, currentBlock))
            {
                blockedRequest = &cbPayload;

                auto& eccFifo = activeEccBlocks[decodedAddress.bank];
                eccFifo.push_back({currentBlock, decodedAddress.row});

                // Only hold 4 elements at max.
                if (eccFifo.size() >= 4)
                    eccFifo.pop_front();

                tlm::tlm_generic_payload* eccPayload = generateEccPayload(decodedAddress);

                iSocket->nb_transport_fw(*eccPayload, tPhase, tDelay);
            }
            else
#endif
            {
                iSocket->nb_transport_fw(cbPayload, tPhase, tDelay);
            }
        }
        else
        {
            pendingRequest = &cbPayload;
        }
    }
    else if (cbPhase == END_REQ) // from target
    {
        // Send payload to inititator in case it is not an ECC transaction
        if (cbPayload.get_extension<DRAMSys::EccExtension>() == nullptr)
        {
            tlm_phase tPhase = END_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            tSocket->nb_transport_bw(cbPayload, tPhase, tDelay);
        }

        if (blockedRequest != nullptr)
        {
            tlm_generic_payload& tPayload = *blockedRequest;
            blockedRequest = nullptr;

            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            iSocket->nb_transport_fw(tPayload, tPhase, tDelay);

            // Do not attempt to send another pending request and hold targetBusy high
            return;
        }

        if (pendingRequest != nullptr)
        {
            tlm_generic_payload& tPayload = *pendingRequest;

            tlm_phase tPhase = BEGIN_REQ;
            sc_time tDelay = SC_ZERO_TIME;

            DRAMSys::DecodedAddress decodedAddress =
                addressDecoder.decodeAddress(tPayload.get_address());
            decodedAddress = calculateOffsetAddress(decodedAddress);

#ifdef ECC_ENABLE
            auto currentBlock = alignToBlock(decodedAddress.column);

            if (!activeEccBlock(decodedAddress.bank, decodedAddress.row, currentBlock))
            {
                blockedRequest = pendingRequest;
                pendingRequest = nullptr;

                auto& eccFifo = activeEccBlocks[decodedAddress.bank];
                eccFifo.push_back({currentBlock, decodedAddress.row});

                // Only hold 4 elements at max.
                if (eccFifo.size() >= 4)
                    eccFifo.pop_front();

                tlm::tlm_generic_payload* eccPayload = generateEccPayload(decodedAddress);

                iSocket->nb_transport_fw(*eccPayload, tPhase, tDelay);
            }
            else
#endif
            {
                iSocket->nb_transport_fw(tPayload, tPhase, tDelay);
                pendingRequest = nullptr;
            }
        }
        else
        {
            assert(!pendingRequest);
            assert(!blockedRequest);
            targetBusy = false;
        }
    }
    else if (cbPhase == BEGIN_RESP) // from memory controller
    {
        // Send payload to inititator in case it is not an ECC transaction
        if (cbPayload.get_extension<DRAMSys::EccExtension>() == nullptr)
        {
            tlm_phase tPhase = BEGIN_RESP;
            sc_time tDelay = SC_ZERO_TIME;

            tlm_sync_enum returnValue = tSocket->nb_transport_bw(cbPayload, tPhase, tDelay);

            // Early completion from initiator
            if (returnValue == TLM_UPDATED)
            {
                payloadEventQueue.notify(cbPayload, tPhase, tDelay);
            }

            Latency latency = sc_time_stamp() - payloadMap.at(&cbPayload);
            payloadMap.erase(&cbPayload);

            latency = roundLatency(latency);
            latencyMap.try_emplace(latency, 0);
            latencyMap.at(latency)++;
        }
        else
        {
            // Send END_RESP by ourselfes
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;

            iSocket->nb_transport_fw(cbPayload, tPhase, tDelay);
        }
    }
    else if (cbPhase == END_RESP) // from initiator
    {
        {
            tlm_phase tPhase = END_RESP;
            sc_time tDelay = SC_ZERO_TIME;

            iSocket->nb_transport_fw(cbPayload, tPhase, tDelay);
        }

        cbPayload.release();
    }
    else
    {
        SC_REPORT_FATAL(0, "Payload event queue in arbiter was triggered with unknown phase");
    }
}

tlm::tlm_generic_payload* EccModule::generateEccPayload(DRAMSys::DecodedAddress decodedAddress)
{
    unsigned int eccAtom = decodedAddress.column / 512;
    uint64_t eccColumn = 1792 + eccAtom * 32;

    decodedAddress.column = eccColumn;
    uint64_t eccAddress = addressDecoder.encodeAddress(decodedAddress);

    tlm_generic_payload& payload = memoryManager.allocate(32);
    payload.acquire();
    payload.set_address(eccAddress);
    payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload.set_dmi_allowed(false);
    payload.set_byte_enable_length(0);
    payload.set_data_length(32);
    payload.set_streaming_width(32);
    payload.set_command(tlm::TLM_READ_COMMAND);
    payload.set_extension<DRAMSys::EccExtension>(new DRAMSys::EccExtension);

    return &payload;
}

unsigned int EccModule::alignToBlock(unsigned column)
{
    return column & ~(512 - 1);
}

DRAMSys::DecodedAddress EccModule::calculateOffsetAddress(DRAMSys::DecodedAddress decodedAddress)
{
    unsigned int newRow =
        std::floor((decodedAddress.row * 256 + decodedAddress.column) / 1792) + decodedAddress.row;
    unsigned int newColumn = (decodedAddress.row * 256 + decodedAddress.column) % 1792;

    DRAMSys::DecodedAddress offsetAddress(decodedAddress);
    offsetAddress.row = newRow;
    offsetAddress.column = newColumn;
    return offsetAddress;
}

void EccModule::end_of_simulation()
{
    uint64_t latencies = 0;
    uint64_t numberOfLatencies = 0;

    for (auto const& [latency, occurences] : latencyMap)
    {
        latencies += (latency.to_double() / 1000.0) * occurences;
        numberOfLatencies += occurences;
    }

    std::cout << "Average latency: " << static_cast<double>(latencies) / numberOfLatencies
              << std::endl;
}

sc_time EccModule::roundLatency(sc_time latency)
{
    static const sc_time BUCKET_SIZE = sc_time(1, SC_NS);
    latency += BUCKET_SIZE / 2;
    latency = latency - (latency % BUCKET_SIZE);
    return latency;
}

bool EccModule::activeEccBlock(Bank bank, Row row, Block block) const
{
    auto eccIt = std::find_if(activeEccBlocks.at(bank).cbegin(),
                              activeEccBlocks.at(bank).cend(),
                              [block, row](EccIdentifier identifier) {
                                  return (identifier.first == block) && (identifier.second == row);
                              });

    return eccIt != activeEccBlocks.at(bank).cend();
}
