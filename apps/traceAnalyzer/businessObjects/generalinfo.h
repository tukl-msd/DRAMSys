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
 *    Derek Christ
 */

#ifndef GENERALINFO_H
#define GENERALINFO_H

#include "timespan.h"

#include <QString>

struct GeneralInfo
{
    uint64_t numberOfTransactions = 0;
    Timespan span = Timespan();
    unsigned int numberOfRanks = 1;
    unsigned int numberOfBankGroups = 1;
    unsigned int numberOfBanks = 1;
    unsigned int banksPerRank = 1;
    unsigned int groupsPerRank = 1;
    unsigned int banksPerGroup = 1;
    QString unitOfTime = "PS";
    uint64_t clkPeriod = 1000;
    unsigned int per2BankOffset = 1;
    bool rowColumnCommandBus = false;
    bool pseudoChannelMode = false;

    GeneralInfo() = default;
    GeneralInfo(uint64_t numberOfTransactions,
                Timespan span,
                unsigned int numberOfRanks,
                unsigned int numberOfBankgroups,
                unsigned int numberOfBanks,
                QString unitOfTime,
                uint64_t clkPeriod,
                unsigned int per2BankOffset,
                bool rowColumnCommandBus,
                bool pseudoChannelMode) :
        numberOfTransactions(numberOfTransactions),
        span(span),
        numberOfRanks(numberOfRanks),
        numberOfBankGroups(numberOfBankgroups),
        numberOfBanks(numberOfBanks),
        banksPerRank(numberOfBanks / numberOfRanks),
        groupsPerRank(numberOfBankgroups / numberOfRanks),
        banksPerGroup(numberOfBanks / numberOfBankgroups),
        unitOfTime(std::move(unitOfTime)),
        clkPeriod(clkPeriod),
        per2BankOffset(per2BankOffset),
        rowColumnCommandBus(rowColumnCommandBus),
        pseudoChannelMode(pseudoChannelMode)
    {
    }
};

#endif // GENERALINFO_H
