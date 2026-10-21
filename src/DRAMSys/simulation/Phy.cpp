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

#include "Phy.h"

#include <DRAMSys/common/MemoryManager.h>
#include <DRAMSys/common/dramExtensions.h>

namespace DRAMSys
{

Phy::Phy(sc_core::sc_module_name const& name,
         std::vector<std::unique_ptr<DramATRecorder>>* dramATRecorders,
         bool databaseRecording,
         bool channelMappingActive,
         std::function<int(int)> idMappingFunc) :
    sc_core::sc_module(name),
    databaseRecording(databaseRecording),
    dramATRecorders(dramATRecorders),
    channelMappingActive(channelMappingActive),
    idMappingFunc(std::move(idMappingFunc))
{
    tSocket.register_b_transport(this, &Phy::b_transport);
    tSocket.register_transport_dbg(this, &Phy::transport_dbg);
};

void Phy::b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    if (databaseRecording)
    {
        auto* ext = trans.get_extension<CommandExtension>();
        dramATRecorders->at(id)->record(trans, ext->command.toPhase(), delay);
    }

    if (channelMappingActive)
    {
        iSocket[idMappingFunc(id)]->b_transport(trans, delay);
    }
    else
    {
        iSocket[id]->b_transport(trans, delay);
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int Phy::transport_dbg(int id, tlm::tlm_generic_payload& trans)
{
    if (channelMappingActive)
    {
        return iSocket[idMappingFunc(id)]->transport_dbg(trans);
    }

    return iSocket[id]->transport_dbg(trans);
}

} // namespace DRAMSys
