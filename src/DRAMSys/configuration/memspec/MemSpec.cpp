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
 *    Lukas Steiner
 *    Derek Christ
 *    Marco Mörz 
 */

#include "MemSpec.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

sc_time MemSpec::getCommandLength(Command command) const
{
    return tCK * commandLengthInCycles[command];
}

double MemSpec::getCommandLengthInCycles(Command command) const
{
    return commandLengthInCycles[command];
}

uint64_t MemSpec::getSimMemSizeInBytes() const
{
    return memorySizeBytes;
}

sc_time MemSpec::getRefreshIntervalAB() const
{
    SC_REPORT_FATAL("MemSpec", "All-bank refresh not supported");
    return SC_ZERO_TIME;
}

sc_time MemSpec::getRefreshIntervalPB() const
{
    SC_REPORT_FATAL("MemSpec", "Per-bank refresh not supported");
    return SC_ZERO_TIME;
}

sc_time MemSpec::getRefreshIntervalP2B() const
{
    SC_REPORT_FATAL("MemSpec", "Per-2-bank refresh not supported");
    return SC_ZERO_TIME;
}

sc_time MemSpec::getRefreshIntervalSB() const
{
    SC_REPORT_FATAL("MemSpec", "Same-bank refresh not supported");
    return SC_ZERO_TIME;
}

unsigned MemSpec::getPer2BankOffset() const
{
    return 0;
}

unsigned MemSpec::getRAADEC() const
{
    SC_REPORT_FATAL("MemSpec", "Refresh Management not supported");
    return 0;
}

unsigned MemSpec::getRAAIMT() const
{
    SC_REPORT_FATAL("MemSpec", "Refresh Management not supported");
    return 0;
}

unsigned MemSpec::getRAAMMT() const
{
    SC_REPORT_FATAL("MemSpec", "Refresh Management not supported");
    return 0;
}

bool MemSpec::hasRasAndCasBus() const
{
    return false;
}

bool MemSpec::pseudoChannelMode() const
{
    return false;
}

bool MemSpec::requiresMaskedWrite(const tlm::tlm_generic_payload& payload) const
{
    if (allBytesEnabled(payload))
        return false;

    SC_REPORT_FATAL("MemSpec", "Standard does not support masked writes!");
    throw;
}

} // namespace DRAMSys
