/*
 * Copyright (c) 2026, Julius-Maximilians-Universität Würzburg
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
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

namespace DRAMSys::Recording
{

struct TlmLTRecord
{
    tlm::tlm_generic_payload* trans{};
    sc_core::sc_time time;
    unsigned signal_id{};
};

struct TlmDBGRecord
{
    tlm::tlm_generic_payload* trans{};
    unsigned signal_id{};
};

template <typename Recorder>
class TlmMonitor : sc_core::sc_module
{
public:
    TlmMonitor(sc_core::sc_module_name const& name, unsigned int signal_id, Recorder* recorder);

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay);
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

    auto& targetSocket() { return tSocket; }
    auto& initiatorSocket() { return iSocket; }

private:
    unsigned int signal_id;

    tlm_utils::simple_target_socket<TlmMonitor> tSocket;
    tlm_utils::simple_initiator_socket<TlmMonitor> iSocket;
    Recorder* recorder;
};

} // namespace DRAMSys::Recording
