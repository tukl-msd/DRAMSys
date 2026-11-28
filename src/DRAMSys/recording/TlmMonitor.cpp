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

#include "TlmMonitor.h"
#include "database/DatabaseRecorder.h"

#include "DRAMSys/common/dramExtensions.h"

namespace DRAMSys::Recording
{

template <typename Recorder>
TlmMonitor<Recorder>::TlmMonitor(sc_core::sc_module_name const& name,
                                 unsigned int signal_id,
                                 Recorder* recorder) :
    sc_core::sc_module(name),
    signal_id(signal_id),
    recorder(recorder)
{
    iSocket.register_nb_transport_bw(this, &TlmMonitor::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &TlmMonitor::nb_transport_fw);
    tSocket.register_b_transport(this, &TlmMonitor::b_transport);
    tSocket.register_transport_dbg(this, &TlmMonitor::transport_dbg);
}

template <typename Recorder>
unsigned int TlmMonitor<Recorder>::transport_dbg(tlm::tlm_generic_payload& trans)
{
    TlmDBGRecord record{&trans, signal_id};
    recorder->record(record);
    return iSocket->transport_dbg(trans);
}

template <typename Recorder>
void TlmMonitor<Recorder>::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    TlmLTRecord record{&trans, sc_core::sc_time_stamp() + delay, signal_id};
    recorder->record(record);
    iSocket->b_transport(trans, delay);
}

template <typename Recorder>
tlm::tlm_sync_enum TlmMonitor<Recorder>::nb_transport_fw(tlm::tlm_generic_payload& trans,
                                                         tlm::tlm_phase& phase,
                                                         sc_core::sc_time& delay)
{
    uint64_t transaction_id = DRAMSys::ArbiterExtension::getTransactionId(trans);
    Recording::TlmATRecord record{
        phase, sc_core::sc_time_stamp() + delay, transaction_id, signal_id};
    recorder->record(record);
    return iSocket->nb_transport_fw(trans, phase, delay);
}

template <typename Recorder>
tlm::tlm_sync_enum TlmMonitor<Recorder>::nb_transport_bw(tlm::tlm_generic_payload& trans,
                                                         tlm::tlm_phase& phase,
                                                         sc_core::sc_time& delay)
{
    uint64_t transaction_id = DRAMSys::ArbiterExtension::getTransactionId(trans);
    Recording::TlmATRecord record{
        phase, sc_core::sc_time_stamp() + delay, transaction_id, signal_id};
    recorder->record(record);
    return tSocket->nb_transport_bw(trans, phase, delay);
}

template class TlmMonitor<DatabaseRecorder>;

} // namespace DRAMSys::Recording
