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

#include <tlm.h>
#include <iostream>
#include <systemc.h>

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
    Bank(unsigned int id) : id(id) {}

    unsigned int ID() const
    {
        return id;
    }

    unsigned int getStartAddress()
    {
        return 0;
    }

    std::string toString()
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

    const Row operator++();

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
    DramExtension(const Thread &thread, const Rank &rank, const BankGroup &bankgroup,
                  const Bank &bank, const Row &row, const Column &column,
                  unsigned int burstlength, uint64_t payloadID);
    DramExtension(const Thread &thread, const Channel &channel, const Rank &rank,
                  const BankGroup &bankgroup, const Bank &bank, const Row &row,
                  const Column &column, unsigned int burstlength, uint64_t payloadID);

    virtual tlm::tlm_extension_base *clone() const;
    virtual void copy_from(const tlm::tlm_extension_base &ext);

    static DramExtension &getExtension(const tlm::tlm_generic_payload *payload);
    static DramExtension &getExtension(const tlm::tlm_generic_payload &payload);

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
    static uint64_t getPayloadID(const tlm::tlm_generic_payload *payload);
    static uint64_t getPayloadID(const tlm::tlm_generic_payload &payload);

    Thread getThread() const;
    Channel getChannel() const;
    Rank getRank() const;
    BankGroup getBankGroup() const;
    Bank getBank() const;
    Row getRow() const;
    Column getColumn() const;

    unsigned int getBurstlength() const;
    uint64_t getPayloadID() const;
    void incrementRow();

private:
    Thread thread;
    Channel channel;
    Rank rank;
    BankGroup bankgroup;
    Bank bank;
    Row row;
    Column column;
    unsigned int burstlength;
    uint64_t payloadID;
};


// Used to indicate the time when a payload is created (in a traceplayer or in a core)
// Note that this time can be different from the time the payload enters the DRAM system
//(at that time the phase BEGIN_REQ is recorded), so timeOfGeneration =< time(BEGIN_REQ)
class GenerationExtension : public tlm::tlm_extension<GenerationExtension>
{
public:
    GenerationExtension(sc_time timeOfGeneration)
        : timeOfGeneration(timeOfGeneration) {}
    virtual tlm::tlm_extension_base *clone() const;
    virtual void copy_from(const tlm::tlm_extension_base &ext);
    static GenerationExtension
            &getExtension(const tlm::tlm_generic_payload *payload);
    sc_time TimeOfGeneration() const
    {
        return timeOfGeneration;
    }
    static sc_time getTimeOfGeneration(const tlm::tlm_generic_payload *payload);
    static sc_time getTimeOfGeneration(const tlm::tlm_generic_payload &payload);

private:
    sc_time timeOfGeneration;
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
