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
 *    Robert Gernhardt
 *    Matthias Jung
 */

#ifndef DRAMEXTENSIONS_H
#define DRAMEXTENSIONS_H

#include <iostream>

#include <systemc>
#include <tlm>

class Thread
{
public:
    explicit Thread(unsigned int id) : id(id) {}

    unsigned int ID() const
    {
        return id;
    }

private:
    unsigned int id;
};

class Channel
{
public:
    explicit Channel(unsigned int id) : id(id) {}

    unsigned int ID() const
    {
        return id;
    }

private:
    unsigned int id;
};

class Rank
{
public:
    explicit Rank(unsigned int id) : id(id) {}

    unsigned int ID() const
    {
        return id;
    }

private:
    unsigned int id;
};

class BankGroup
{
public:
    explicit BankGroup(unsigned int id) : id(id) {}

    unsigned int ID() const
    {
        return id;
    }

private:
    unsigned int id;
};

class Bank
{
public:
    explicit Bank(unsigned int id) : id(id) {}

    unsigned int ID() const
    {
        return id;
    }

    std::string toString() const
    {
        return std::to_string(id);
    }

private:
    unsigned int id;
};

class Row
{
public:
    static const Row NO_ROW;

    Row() : id(0), isNoRow(true) {}

    explicit Row(unsigned int id) : id(id), isNoRow(false) {}

    unsigned int ID() const
    {
        return id;
    }

    Row operator++();

private:
    unsigned int id;
    bool isNoRow;

    friend bool operator==(const Row &lhs, const Row &rhs);
};

class Column
{
public:
    explicit Column(unsigned int id) : id(id) {}

    unsigned int ID() const
    {
        return id;
    }

private:
    unsigned int id;
};


class DramExtension : public tlm::tlm_extension<DramExtension>
{
public:
    DramExtension();
    DramExtension(Thread thread, Channel channel, Rank rank,
                  BankGroup bankGroup, Bank bank, Row row,
                  Column column, unsigned int burstLength,
                  uint64_t threadPayloadID, uint64_t channelPayloadID);
    tlm::tlm_extension_base *clone() const override;
    void copy_from(const tlm::tlm_extension_base &ext) override;

    static void setExtension(tlm::tlm_generic_payload *payload,
                             Thread thread, Channel channel, Rank rank,
                             BankGroup bankGroup, Bank bank, Row row,
                             Column column, unsigned int burstLength,
                             uint64_t threadPayloadID, uint64_t channelPayloadID);
    static void setExtension(tlm::tlm_generic_payload &payload,
                             Thread thread, Channel channel, Rank rank,
                             BankGroup bankGroup, Bank bank, Row row,
                             Column column, unsigned int burstLength,
                             uint64_t threadPayloadID, uint64_t channelPayloadID);

    static DramExtension &getExtension(const tlm::tlm_generic_payload *payload);
    static DramExtension &getExtension(const tlm::tlm_generic_payload &payload);

    static void setPayloadIDs(tlm::tlm_generic_payload *payload,
                                 uint64_t threadPayloadID, uint64_t channelPayloadID);
    static void setPayloadIDs(tlm::tlm_generic_payload &payload,
                                 uint64_t threadPayloadID, uint64_t channelPayloadID);

    // Used for convience, caller could also use getExtension(..) to access these field
    static Thread getThread(const tlm::tlm_generic_payload *payload);
    static Thread getThread(const tlm::tlm_generic_payload &payload);
    static Channel getChannel(const tlm::tlm_generic_payload *payload);
    static Channel getChannel(const tlm::tlm_generic_payload &payload);
    static Rank getRank(const tlm::tlm_generic_payload *payload);
    static Rank getRank(const tlm::tlm_generic_payload &payload);
    static BankGroup getBankGroup(const tlm::tlm_generic_payload *payload);
    static BankGroup getBankGroup(const tlm::tlm_generic_payload &payload);
    static Bank getBank(const tlm::tlm_generic_payload *payload);
    static Bank getBank(const tlm::tlm_generic_payload &payload);
    static Row getRow(const tlm::tlm_generic_payload *payload);
    static Row getRow(const tlm::tlm_generic_payload &payload);
    static Column getColumn(const tlm::tlm_generic_payload *payload);
    static Column getColumn(const tlm::tlm_generic_payload &payload);
    static unsigned getBurstLength(const tlm::tlm_generic_payload *payload);
    static unsigned getBurstLength(const tlm::tlm_generic_payload &payload);
    static uint64_t getThreadPayloadID(const tlm::tlm_generic_payload *payload);
    static uint64_t getThreadPayloadID(const tlm::tlm_generic_payload &payload);
    static uint64_t getChannelPayloadID(const tlm::tlm_generic_payload *payload);
    static uint64_t getChannelPayloadID(const tlm::tlm_generic_payload &payload);

    Thread getThread() const;
    Channel getChannel() const;
    Rank getRank() const;
    BankGroup getBankGroup() const;
    Bank getBank() const;
    Row getRow() const;
    Column getColumn() const;

    unsigned int getBurstLength() const;
    uint64_t getThreadPayloadID() const;
    uint64_t getChannelPayloadID() const;

private:
    Thread thread;
    Channel channel;
    Rank rank;
    BankGroup bankGroup;
    Bank bank;
    Row row;
    Column column;
    unsigned int burstLength;
    uint64_t threadPayloadID;
    uint64_t channelPayloadID;
};


// Used to indicate the time when a payload is created (in a traceplayer or in a core)
// Note that this time can be different from the time the payload enters the DRAM system
//(at that time the phase BEGIN_REQ is recorded), so timeOfGeneration =< time(BEGIN_REQ)
class GenerationExtension : public tlm::tlm_extension<GenerationExtension>
{
public:
    explicit GenerationExtension(const sc_core::sc_time &timeOfGeneration)
        : timeOfGeneration(timeOfGeneration) {}
    tlm::tlm_extension_base *clone() const override;
    void copy_from(const tlm::tlm_extension_base &ext) override;
    static void setExtension(tlm::tlm_generic_payload *payload, const sc_core::sc_time &timeOfGeneration);
    static void setExtension(tlm::tlm_generic_payload &payload, const sc_core::sc_time &timeOfGeneration);
    static GenerationExtension &getExtension(const tlm::tlm_generic_payload *payload);
    static GenerationExtension &getExtension(const tlm::tlm_generic_payload &payload);
    static sc_core::sc_time getTimeOfGeneration(const tlm::tlm_generic_payload *payload);
    static sc_core::sc_time getTimeOfGeneration(const tlm::tlm_generic_payload &payload);

private:
    sc_core::sc_time timeOfGeneration;
};


bool operator==(const Thread &lhs, const Thread &rhs);
bool operator!=(const Thread &lhs, const Thread &rhs);
bool operator<(const Thread &lhs, const Thread &rhs);

bool operator==(const Channel &lhs, const Channel &rhs);
bool operator!=(const Channel &lhs, const Channel &rhs);

bool operator==(const Rank &lhs, const Rank &rhs);
bool operator!=(const Rank &lhs, const Rank &rhs);

bool operator==(const BankGroup &lhs, const BankGroup &rhs);
bool operator!=(const BankGroup &lhs, const BankGroup &rhs);

bool operator==(const Bank &lhs, const Bank &rhs);
bool operator!=(const Bank &lhs, const Bank &rhs);
bool operator<(const Bank &lhs, const Bank &rhs);

bool operator==(const Row &lhs, const Row &rhs);
bool operator!=(const Row &lhs, const Row &rhs);

bool operator==(const Column &lhs, const Column &rhs);
bool operator!=(const Column &lhs, const Column &rhs);

#endif // DRAMEXTENSIONS_H
