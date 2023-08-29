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
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 *    Luiza Correa
 *    Derek Christ
 */

#include "utils.h"

#include <sstream>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

bool TimeInterval::timeIsInInterval(const sc_time& time) const
{
    return (start < time && time < end);
}

bool TimeInterval::intersects(const TimeInterval& other) const
{
    return other.timeIsInInterval(this->start) || this->timeIsInInterval(other.start);
}

sc_time TimeInterval::getLength() const
{
    if (start > end)
        return start - end;

    return end - start;
}

std::string getPhaseName(const tlm_phase& phase)
{
    std::ostringstream oss;
    oss << phase;
    return oss.str();
}

void setUpDummy(tlm_generic_payload& payload,
                uint64_t channelPayloadID,
                Rank rank,
                BankGroup bankGroup,
                Bank bank)
{
    payload.set_address(0);
    payload.set_command(TLM_IGNORE_COMMAND);
    payload.set_data_length(0);
    payload.set_response_status(TLM_OK_RESPONSE);
    payload.set_dmi_allowed(false);
    payload.set_byte_enable_length(0);
    payload.set_streaming_width(0);
    ControllerExtension::setExtension(
        payload, channelPayloadID, rank, bankGroup, bank, Row(0), Column(0), 0);
    ArbiterExtension::setExtension(payload, Thread(UINT_MAX), Channel(0), 0, SC_ZERO_TIME);
}

bool isFullCycle(sc_core::sc_time time, sc_core::sc_time cycleTime)
{
    return alignAtNext(time, cycleTime) == time;
}

sc_time alignAtNext(sc_time time, sc_time alignment)
{
    return std::ceil(time / alignment) * alignment;
}

} // namespace DRAMSys
