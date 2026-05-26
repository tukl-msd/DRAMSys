/*
 * Copyright (c) 2023, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_target_socket.h>

#include <queue>

class TargetMemory : public sc_core::sc_module
{
public:
    tlm_utils::simple_target_socket<TargetMemory> tSocket;
    TargetMemory(const sc_core::sc_module_name& name,
                 sc_core::sc_time acceptDelay,
                 sc_core::sc_time memoryLatency,
                 std::size_t bufferSize = DEFAULT_BUFFER_SIZE);

private:
    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay);

    void peqCallback(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase);

    void sendEndRequest(tlm::tlm_generic_payload& trans);
    void sendResponse(tlm::tlm_generic_payload& trans);
    void executeTransaction(tlm::tlm_generic_payload& trans);

    void printBuffer(int max, int n);

    static constexpr std::size_t SIZE = static_cast<std::size_t>(64 * 1024);
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 8;
    const std::size_t bufferSize;

    const sc_core::sc_time acceptDelay;
    const sc_core::sc_time memoryLatency;

    std::vector<uint8_t> memory;

    unsigned int currentTransactions = 0;
    bool responseInProgress = false;
    tlm::tlm_generic_payload* endRequestPending = nullptr;

    tlm_utils::peq_with_cb_and_phase<TargetMemory> peq;
    std::queue<tlm::tlm_generic_payload*> responseQueue;
};
