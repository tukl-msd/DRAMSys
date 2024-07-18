/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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
 * Author: Lukas Steiner
 */

#ifndef REFRESHMANAGERIF_H
#define REFRESHMANAGERIF_H

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/controller/Command.h"
#include "DRAMSys/controller/ManagerIF.h"

#include <cmath>
#include <systemc>

namespace DRAMSys
{

class RefreshManagerIF : public ManagerIF
{
public:
    virtual sc_core::sc_time getTimeForNextTrigger() = 0;

protected:
    static sc_core::sc_time getTimeForFirstTrigger(const sc_core::sc_time& tCK,
                                                   const sc_core::sc_time& refreshInterval,
                                                   Rank rank,
                                                   unsigned numberOfRanks)
    {
        // Calculate bit-reversal rank ID
        auto rankID = static_cast<unsigned>(rank);
        unsigned reverseRankID = 0;
        unsigned rankBits = 0;
        unsigned rankShift = numberOfRanks;

        while (rankShift >>= 1)
            rankBits++;

        rankBits--;

        while (rankID != 0)
        {
            reverseRankID |= (rankID & 1) << rankBits;
            rankID >>= 1;
            rankBits--;
        }

        // Use bit-reversal order for refreshes on ranks
        sc_core::sc_time timeForFirstTrigger =
            refreshInterval - reverseRankID * (refreshInterval / numberOfRanks);
        timeForFirstTrigger = std::ceil(timeForFirstTrigger / tCK) * tCK;

        return timeForFirstTrigger;
    }
};

} // namespace DRAMSys

#endif // REFRESHMANAGERIF_H
