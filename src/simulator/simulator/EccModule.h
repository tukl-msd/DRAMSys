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

#ifndef ECCMODULE_H
#define ECCMODULE_H

#include "simulator/MemoryManager.h"

#include <DRAMSys/simulation/AddressDecoder.h>

#include <deque>
#include <map>
#include <systemc>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <unordered_map>

class EccModule : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<EccModule> iSocket;
    tlm_utils::simple_target_socket<EccModule> tSocket;

    EccModule(sc_core::sc_module_name name, DRAMSys::AddressDecoder const& addressDecoder);
    SC_HAS_PROCESS(EccModule);

private:
    using Block = uint64_t;
    using Row = uint64_t;
    using Bank = unsigned int;
    using EccIdentifier = std::pair<Block, Row>;
    using EccQueue = std::deque<EccIdentifier>;

    static DRAMSys::DecodedAddress calculateOffsetAddress(DRAMSys::DecodedAddress decodedAddress);
    static sc_core::sc_time roundLatency(sc_core::sc_time latency);
    bool activeEccBlock(Bank bank, Row row, Block block) const;

    void end_of_simulation() override;

    void peqCallback(tlm::tlm_generic_payload& payload, const tlm::tlm_phase& phase);

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& payload,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& fwDelay);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& payload,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& bwDelay);

    tlm::tlm_generic_payload* generateEccPayload(DRAMSys::DecodedAddress decodedAddress);

    static unsigned int alignToBlock(unsigned int column);

    tlm_utils::peq_with_cb_and_phase<EccModule> payloadEventQueue;

    tlm::tlm_generic_payload* pendingRequest = nullptr;
    tlm::tlm_generic_payload* blockedRequest = nullptr;
    bool targetBusy = false;

    const sc_core::sc_time tCK;
    MemoryManager memoryManager;
    DRAMSys::AddressDecoder const& addressDecoder;

    std::unordered_map<Bank, EccQueue> activeEccBlocks;

    using EccPayload = tlm::tlm_generic_payload*;
    using StartTime = sc_core::sc_time;
    std::unordered_map<tlm::tlm_generic_payload*, StartTime> payloadMap;

    using Latency = sc_core::sc_time;
    std::map<Latency, uint64_t> latencyMap;
};

#endif // ECCMODULE_H
