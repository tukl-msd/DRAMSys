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
 *    Eder F. Zulian
 *    Luiza Correa
 *    Derek Christ
 */

#ifndef UTILS_H
#define UTILS_H

#include "DRAMSys/common/dramExtensions.h"

#include <string>
#include <systemc>
#include <tlm>

namespace DRAMSys
{

class TimeInterval
{
public:
    sc_core::sc_time start, end;
    TimeInterval() : start(sc_core::SC_ZERO_TIME), end(sc_core::SC_ZERO_TIME) {}
    TimeInterval(const sc_core::sc_time& start, const sc_core::sc_time& end) :
        start(start),
        end(end)
    {
    }

    [[nodiscard]] sc_core::sc_time getLength() const;
    [[nodiscard]] bool timeIsInInterval(const sc_core::sc_time& time) const;
    [[nodiscard]] bool intersects(const TimeInterval& other) const;
};

constexpr const std::string_view headline =
    "===========================================================================";

std::string getPhaseName(const tlm::tlm_phase& phase);

void setUpDummy(tlm::tlm_generic_payload& payload,
                uint64_t channelPayloadID,
                Rank rank = Rank(0),
                BankGroup bankGroup = BankGroup(0),
                Bank bank = Bank(0));

bool isFullCycle(sc_core::sc_time time, sc_core::sc_time cycleTime);
sc_core::sc_time alignAtNext(sc_core::sc_time time, sc_core::sc_time alignment);

} // namespace DRAMSys

#endif // UTILS_H
