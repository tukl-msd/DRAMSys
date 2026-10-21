/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 *    Thomas Zimmermann
 */

#ifndef PHY_H
#define PHY_H

#include <DRAMSys/common/DramATRecorder.h>
#include <DRAMSys/configuration/memspec/MemSpec.h>
#include <DRAMSys/controller/Command.h>

#include <systemc>
#include <tlm>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

namespace DRAMSys
{

class Phy : sc_core::sc_module
{
public:
    tlm_utils::multi_passthrough_target_socket<Phy> tSocket{"tSocket"};
    tlm_utils::multi_passthrough_initiator_socket<Phy> iSocket{"iSocket"};

    Phy(
        sc_core::sc_module_name const& name,
        std::vector<std::unique_ptr<DramATRecorder>>* dramATRecorders,
        bool databaseRecording,
        bool channelMappingActive = false,
        std::function<int(int)> idMappingFunc = [](int id) { return id % 2; });

    void setChannelMappingActive(bool active) { channelMappingActive = active; }

private:
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans);

    bool databaseRecording;
    std::vector<std::unique_ptr<DramATRecorder>>* dramATRecorders;

    bool channelMappingActive;
    std::function<int(int)> idMappingFunc;
};

} // namespace DRAMSys

#endif // PHY_H
