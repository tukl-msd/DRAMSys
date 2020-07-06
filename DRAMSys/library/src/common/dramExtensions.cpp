/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
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
 */

#include "dramExtensions.h"
#include "../configuration/Configuration.h"
#include "map"
#include "utils.h"

using namespace tlm;

DramExtension::DramExtension() :
    thread(0), channel(0), rank(0), bankgroup(0), bank(0),
    row(0), column(0), burstlength(0), payloadID(0) {}

DramExtension::DramExtension(const Thread &thread, const Rank &rank, const BankGroup &bankgroup,
                             const Bank &bank, const Row &row, const Column &column,
                             unsigned int burstlength, uint64_t payloadID) :
    thread(thread), channel(0), rank(rank), bankgroup(bankgroup), bank(bank),
    row(row), column(column), burstlength(burstlength), payloadID(payloadID) {}

DramExtension::DramExtension(const Thread &thread, const Channel &channel, const Rank &rank,
                             const BankGroup &bankgroup, const Bank &bank, const Row &row,
                             const Column &column, unsigned int burstlength, uint64_t payloadID) :
    thread(thread), channel(channel), rank(rank), bankgroup(bankgroup), bank(bank),
    row(row), column(column), burstlength(burstlength), payloadID(payloadID) {}

DramExtension &DramExtension::getExtension(const tlm_generic_payload *payload)
{
    DramExtension *result = NULL;
    payload->get_extension(result);
    sc_assert(result != NULL);

    return *result;
}

DramExtension &DramExtension::getExtension(const tlm_generic_payload &payload)
{
    return DramExtension::getExtension(&payload);
}

Thread DramExtension::getThread(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getThread();
}

Thread DramExtension::getThread(const tlm_generic_payload &payload)
{
    return DramExtension::getThread(&payload);
}

Channel DramExtension::getChannel(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getChannel();
}

Channel DramExtension::getChannel(const tlm_generic_payload &payload)
{
    return DramExtension::getChannel(&payload);
}

Rank DramExtension::getRank(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getRank();
}

Rank DramExtension::getRank(const tlm_generic_payload &payload)
{
    return DramExtension::getRank(&payload);
}

BankGroup DramExtension::getBankGroup(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getBankGroup();
}

BankGroup DramExtension::getBankGroup(const tlm_generic_payload &payload)
{
    return DramExtension::getBankGroup(&payload);
}

Bank DramExtension::getBank(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getBank();
}

Bank DramExtension::getBank(const tlm_generic_payload &payload)
{
    return DramExtension::getBank(&payload);
}

Row DramExtension::getRow(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getRow();
}

Row DramExtension::getRow(const tlm_generic_payload &payload)
{
    return DramExtension::getRow(&payload);
}

Column DramExtension::getColumn(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getColumn();
}

Column DramExtension::getColumn(const tlm_generic_payload &payload)
{
    return DramExtension::getColumn(&payload);
}

uint64_t DramExtension::getPayloadID(const tlm_generic_payload *payload)
{
    return DramExtension::getExtension(payload).getPayloadID();
}

uint64_t DramExtension::getPayloadID(const tlm_generic_payload &payload)
{
    return DramExtension::getPayloadID(&payload);
}

tlm_extension_base *DramExtension::clone() const
{
    return new DramExtension(thread, channel, rank, bankgroup, bank, row, column, burstlength, payloadID);
}

void DramExtension::copy_from(const tlm_extension_base &ext)
{
    const DramExtension &cpyFrom = static_cast<const DramExtension &>(ext);
    thread = cpyFrom.thread;
    channel = cpyFrom.channel;
    rank = cpyFrom.rank;
    bankgroup = cpyFrom.bankgroup;
    bank = cpyFrom.bank;
    row = cpyFrom.row;
    column = cpyFrom.column;
    burstlength = cpyFrom.burstlength;
}

Thread DramExtension::getThread() const
{
    return thread;
}

Channel DramExtension::getChannel() const
{
    return channel;
}

Rank DramExtension::getRank() const
{
    return rank;
}

BankGroup DramExtension::getBankGroup() const
{
    return bankgroup;
}

Bank DramExtension::getBank() const
{
    return bank;
}

Row DramExtension::getRow() const
{
    return row;
}

Column DramExtension::getColumn() const
{
    return column;
}

unsigned int DramExtension::getBurstlength() const
{
    return burstlength;
}

uint64_t DramExtension::getPayloadID() const
{
    return payloadID;
}

void DramExtension::incrementRow()
{
    ++row;
}

tlm_extension_base *GenerationExtension::clone() const
{
    return new GenerationExtension(timeOfGeneration);
}

void GenerationExtension::copy_from(const tlm_extension_base &ext)
{
    const GenerationExtension &cpyFrom = static_cast<const GenerationExtension &>(ext);
    timeOfGeneration = cpyFrom.timeOfGeneration;

}

GenerationExtension &GenerationExtension::getExtension(const tlm_generic_payload *payload)
{
    GenerationExtension *result = NULL;
    payload->get_extension(result);
    sc_assert(result != NULL);
    return *result;
}

sc_time GenerationExtension::getTimeOfGeneration(const tlm_generic_payload *payload)
{
    return GenerationExtension::getExtension(payload).TimeOfGeneration();
}

sc_time GenerationExtension::getTimeOfGeneration(const tlm_generic_payload &payload)
{
    return GenerationExtension::getTimeOfGeneration(&payload);
}

//THREAD
bool operator ==(const Thread &lhs, const Thread &rhs)
{
    return lhs.ID() == rhs.ID();
}

bool operator !=(const Thread &lhs, const Thread &rhs)
{
    return !(lhs == rhs);
}

bool operator <(const Thread &lhs, const Thread &rhs)
{
    return lhs.ID() < rhs.ID();
}

//CHANNEL
bool operator ==(const Channel &lhs, const Channel &rhs)
{
    return lhs.ID() == rhs.ID();
}

bool operator !=(const Channel &lhs, const Channel &rhs)
{
    return !(lhs == rhs);
}

//RANK
bool operator ==(const Rank &lhs, const Rank &rhs)
{
    return lhs.ID() == rhs.ID();
}

bool operator !=(const Rank &lhs, const Rank &rhs)
{
    return !(lhs == rhs);
}

//BANKGROUP
bool operator ==(const BankGroup &lhs, const BankGroup &rhs)
{
    return lhs.ID() == rhs.ID();
}

bool operator !=(const BankGroup &lhs, const BankGroup &rhs)
{
    return !(lhs == rhs);
}

//BANK
bool operator ==(const Bank &lhs, const Bank &rhs)
{
    return lhs.ID() == rhs.ID();
}

bool operator !=(const Bank &lhs, const Bank &rhs)
{
    return !(lhs == rhs);
}

bool operator <(const Bank &lhs, const Bank &rhs)
{
    return lhs.ID() < rhs.ID();
}

//ROW
const Row Row::NO_ROW;

bool operator ==(const Row &lhs, const Row &rhs)
{
    if (lhs.isNoRow != rhs.isNoRow)
        return false;
    return lhs.ID() == rhs.ID();
}

bool operator !=(const Row &lhs, const Row &rhs)
{
    return !(lhs == rhs);
}

const Row Row::operator ++()
{
    id = (id + 1) % Configuration::getInstance().memSpec->numberOfRows;
    return *this;
}


//COLUMN
bool operator ==(const Column &lhs, const Column &rhs)
{
    return lhs.ID() == rhs.ID();
}

bool operator !=(const Column &lhs, const Column &rhs)
{
    return !(lhs == rhs);
}
