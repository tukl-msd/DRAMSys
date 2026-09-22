/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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
 *    Peter Ehses
 *    Eder F. Zulian
 *    Felipe S. Prado
 *    Derek Christ
 *    Marco Mörz
 */

#include "Dram.h"

#include <DRAMSys/common/MemoryManager.h>
#include <DRAMSys/common/dramExtensions.h>

#include <cassert>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

void Dram::b_transport(int id, tlm_generic_payload& trans, [[maybe_unused]] sc_time& delay)
{
    trans.set_response_status(tlm::TLM_OK_RESPONSE);

    auto* ext = trans.get_extension<CommandExtension>();

    if (recording)
    {
        (*dramATRecorders)[id]->record(trans, ext->command.toPhase(), delay);
    }

#ifdef USE_DRAMPOWER
    if (power)
    {
        (*DRAMPowerMappings)[id]->handleTransaction(id, trans, ext->command.toPhase(), delay);
    }
#endif

    if (trans.get_extension<NoDataExtension>() != nullptr)
        return;

    assert(*backingStore != nullptr);

    if (ext->command.isCasCommand())
    {
        if (trans.is_read())
        {
            executeRead(trans);
        }
        else if (trans.is_write())
        {
            executeWrite(trans);
        }
    }

    trans.set_response_status(TLM_OK_RESPONSE);
}

unsigned int Dram::transport_dbg([[maybe_unused]] int id, tlm::tlm_generic_payload& trans)
{
    assert(*backingStore != nullptr);

    if (trans.is_read())
    {
        executeRead(trans);
    }
    else if (trans.is_write())
    {
        executeWrite(trans);
    }

    return trans.get_data_length();
}

void Dram::executeRead(tlm::tlm_generic_payload& trans) const
{
    unsigned char const* phyAddr = *backingStore + trans.get_address();

    if (trans.get_byte_enable_ptr() == nullptr)
    {
        memcpy(trans.get_data_ptr(), phyAddr, trans.get_data_length());
    }
    else
    {
        for (std::size_t i = 0; i < trans.get_data_length(); i++)
        {
            std::size_t byteEnableIndex = i % trans.get_byte_enable_length();
            if (trans.get_byte_enable_ptr()[byteEnableIndex] == TLM_BYTE_ENABLED)
            {
                trans.get_data_ptr()[i] = phyAddr[i];
            }
        }
    }
}

void Dram::executeWrite(const tlm::tlm_generic_payload& trans)
{
    unsigned char* phyAddr = *backingStore + trans.get_address();

    if (trans.get_byte_enable_ptr() == nullptr)
    {
        memcpy(phyAddr, trans.get_data_ptr(), trans.get_data_length());
    }
    else
    {
        for (std::size_t i = 0; i < trans.get_data_length(); i++)
        {
            std::size_t byteEnableIndex = i % trans.get_byte_enable_length();
            if (trans.get_byte_enable_ptr()[byteEnableIndex] == TLM_BYTE_ENABLED)
            {
                phyAddr[i] = trans.get_data_ptr()[i];
            }
        }
    }
}

} // namespace DRAMSys
