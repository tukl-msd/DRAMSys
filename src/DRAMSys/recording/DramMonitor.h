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

namespace DRAMSys
{
class MemSpec;
}

namespace DRAMSys::Recording
{

class DatabaseRecorder;

/**
 * A pass-through module placed between a Controller and a DRAM that records
 * everything passing over the channel's command and data bus.
 */
class DramMonitor : sc_core::sc_module
{
public:
    DramMonitor(sc_core::sc_module_name const& name,
                unsigned command_bus_id,
                unsigned data_bus_id,
                sc_core::sc_time windowSize,
                MemSpec const* memspec,
                DatabaseRecorder* recorder);

    auto& targetSocket() { return tSocket; }
    auto& initiatorSocket() { return iSocket; }

private:
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);

    void recordBandwidth();

    sc_core::sc_time windowSize;
    uint64_t bytesServedInWindow;

    unsigned command_bus_id;
    unsigned data_bus_id;
    MemSpec const* memspec;
    DatabaseRecorder* recorder;

    tlm_utils::simple_target_socket<DramMonitor> tSocket;
    tlm_utils::simple_initiator_socket<DramMonitor> iSocket;
};

} // namespace DRAMSys::Recording
