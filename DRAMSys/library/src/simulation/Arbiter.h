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

#ifndef ARBITER_H
#define ARBITER_H

#include <iostream>
#include <vector>
#include <queue>
#include <set>

#include <tlm>
#include <systemc>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include "AddressDecoder.h"
#include "../common/dramExtensions.h"

DECLARE_EXTENDED_PHASE(REQ_ARBITRATION);
DECLARE_EXTENDED_PHASE(RESP_ARBITRATION);

class Arbiter : public sc_core::sc_module
{
public:
    tlm_utils::multi_passthrough_initiator_socket<Arbiter> iSocket;
    tlm_utils::multi_passthrough_target_socket<Arbiter> tSocket;

protected:
    Arbiter(const sc_core::sc_module_name &name, const Configuration& config,
            const DRAMSysConfiguration::AddressMapping &addressMapping);
    SC_HAS_PROCESS(Arbiter);

    void end_of_elaboration() override;

    AddressDecoder addressDecoder;

    tlm_utils::peq_with_cb_and_phase<Arbiter> payloadEventQueue;
    virtual void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase) = 0;

    std::vector<bool> threadIsBusy;
    std::vector<bool> channelIsBusy;

    std::vector<std::queue<tlm::tlm_generic_payload *>> pendingRequests;

    std::vector<uint64_t> nextThreadPayloadIDToAppend;
    std::vector<uint64_t> nextChannelPayloadIDToAppend;

    tlm::tlm_sync_enum nb_transport_fw(int id, tlm::tlm_generic_payload &payload,
                                  tlm::tlm_phase &phase, sc_core::sc_time &fwDelay);
    tlm::tlm_sync_enum nb_transport_bw(int, tlm::tlm_generic_payload &payload,
                                  tlm::tlm_phase &phase, sc_core::sc_time &bwDelay);
    unsigned int transport_dbg(int /*id*/, tlm::tlm_generic_payload &trans);

    const sc_core::sc_time tCK;
    const sc_core::sc_time arbitrationDelayFw;
    const sc_core::sc_time arbitrationDelayBw;

    const unsigned bytesPerBeat;
    const uint64_t addressOffset;
};

class ArbiterSimple final : public Arbiter
{
public:
    ArbiterSimple(const sc_core::sc_module_name &name, const Configuration& config,
                  const DRAMSysConfiguration::AddressMapping &addressMapping);
    SC_HAS_PROCESS(ArbiterSimple);

private:
    void end_of_elaboration() override;
    void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase) override;

    std::vector<std::queue<tlm::tlm_generic_payload *>> pendingResponses;
};

class ArbiterFifo final : public Arbiter
{
public:
    ArbiterFifo(const sc_core::sc_module_name &name, const Configuration& config,
                const DRAMSysConfiguration::AddressMapping &addressMapping);
    SC_HAS_PROCESS(ArbiterFifo);

private:
    void end_of_elaboration() override;
    void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase) override;

    std::vector<unsigned int> activeTransactions;
    const unsigned maxActiveTransactions;

    std::vector<tlm::tlm_generic_payload *> outstandingEndReq;
    std::vector<std::queue<tlm::tlm_generic_payload *>> pendingResponses;

    std::vector<sc_core::sc_time> lastEndReq;
    std::vector<sc_core::sc_time> lastEndResp;
};

class ArbiterReorder final : public Arbiter
{
public:
    ArbiterReorder(const sc_core::sc_module_name &name, const Configuration& config,
                   const DRAMSysConfiguration::AddressMapping &addressMapping);
    SC_HAS_PROCESS(ArbiterReorder);

private:
    void end_of_elaboration() override;
    void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase) override;

    std::vector<unsigned int> activeTransactions;
    const unsigned maxActiveTransactions;

    struct ThreadPayloadIDCompare
    {
        bool operator() (const tlm::tlm_generic_payload *lhs, const tlm::tlm_generic_payload *rhs) const
        {
            return DramExtension::getThreadPayloadID(lhs) < DramExtension::getThreadPayloadID(rhs);
        }
    };

    std::vector<tlm::tlm_generic_payload *> outstandingEndReq;
    std::vector<std::set<tlm::tlm_generic_payload*, ThreadPayloadIDCompare>> pendingResponses;

    std::vector<sc_core::sc_time> lastEndReq;
    std::vector<sc_core::sc_time> lastEndResp;

    std::vector<uint64_t> nextThreadPayloadIDToReturn;
};

#endif // ARBITER_H
