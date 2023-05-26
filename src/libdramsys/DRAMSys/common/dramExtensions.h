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
 */

#ifndef DRAMEXTENSIONS_H
#define DRAMEXTENSIONS_H

#include <iostream>

#include <systemc>
#include <tlm>

namespace DRAMSys
{

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

class ArbiterExtension : public tlm::tlm_extension<ArbiterExtension>
{
public:
    static void setAutoExtension(tlm::tlm_generic_payload& trans, Thread thread, Channel channel);
    static void setExtension(tlm::tlm_generic_payload& trans, Thread thread, Channel channel,
                             uint64_t threadPayloadID, const sc_core::sc_time& timeOfGeneration);
    static void setIDAndTimeOfGeneration(tlm::tlm_generic_payload& trans, uint64_t threadPayloadID,
                                         const sc_core::sc_time& timeOfGeneration);

    tlm::tlm_extension_base* clone() const override;
    void copy_from(const tlm::tlm_extension_base& ext) override;

    Thread getThread() const;
    Channel getChannel() const;
    uint64_t getThreadPayloadID() const;
    sc_core::sc_time getTimeOfGeneration() const;

    static const ArbiterExtension& getExtension(const tlm::tlm_generic_payload& trans);
    static Thread getThread(const tlm::tlm_generic_payload& trans);
    static Channel getChannel(const tlm::tlm_generic_payload& trans);
    static uint64_t getThreadPayloadID(const tlm::tlm_generic_payload& trans);
    static sc_core::sc_time getTimeOfGeneration(const tlm::tlm_generic_payload& trans);

private:
    ArbiterExtension(Thread thread, Channel channel, uint64_t threadPayloadID, const sc_core::sc_time& timeOfGeneration);
    Thread thread;
    Channel channel;
    uint64_t threadPayloadID;
    sc_core::sc_time timeOfGeneration;
};

class ControllerExtension : public tlm::tlm_extension<ControllerExtension>
{
public:
    static void setAutoExtension(tlm::tlm_generic_payload& trans, uint64_t channelPayloadID, Rank rank, BankGroup bankGroup,
                             Bank bank, Row row, Column column, unsigned burstLength);

    static void setExtension(tlm::tlm_generic_payload& trans, uint64_t channelPayloadID, Rank rank, BankGroup bankGroup,
                                 Bank bank, Row row, Column column, unsigned burstLength);

    //static ControllerExtension& getExtension(const tlm::tlm_generic_payload& trans);

    tlm::tlm_extension_base* clone() const override;
    void copy_from(const tlm::tlm_extension_base& ext) override;

    uint64_t getChannelPayloadID() const;
    Rank getRank() const;
    BankGroup getBankGroup() const;
    Bank getBank() const;
    Row getRow() const;
    Column getColumn() const;
    unsigned getBurstLength() const;

    static const ControllerExtension& getExtension(const tlm::tlm_generic_payload& trans);
    static uint64_t getChannelPayloadID(const tlm::tlm_generic_payload& trans);
    static Rank getRank(const tlm::tlm_generic_payload& trans);
    static BankGroup getBankGroup(const tlm::tlm_generic_payload& trans);
    static Bank getBank(const tlm::tlm_generic_payload& trans);
    static Row getRow(const tlm::tlm_generic_payload& trans);
    static Column getColumn(const tlm::tlm_generic_payload& trans);
    static unsigned getBurstLength(const tlm::tlm_generic_payload& trans);

private:
    ControllerExtension(uint64_t channelPayloadID, Rank rank, BankGroup bankGroup, Bank bank, Row row, Column column,
                        unsigned burstLength);
    uint64_t channelPayloadID;
    Rank rank;
    BankGroup bankGroup;
    Bank bank;
    Row row;
    Column column;
    unsigned burstLength;
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

class ChildExtension : public tlm::tlm_extension<ChildExtension>
{
private:
    tlm::tlm_generic_payload* parentTrans;
    explicit ChildExtension(tlm::tlm_generic_payload& parentTrans) : parentTrans(&parentTrans) {}

public:
    //ChildExtension() = delete;

    tlm::tlm_extension_base* clone() const override;
    void copy_from(const tlm::tlm_extension_base& ext) override;
    tlm::tlm_generic_payload& getParentTrans();
    static tlm::tlm_generic_payload& getParentTrans(tlm::tlm_generic_payload& childTrans);
    static void setExtension(tlm::tlm_generic_payload& childTrans, tlm::tlm_generic_payload& parentTrans);
    static bool isChildTrans(const tlm::tlm_generic_payload& trans);
};

class ParentExtension : public tlm::tlm_extension<ParentExtension>
{
private:
    std::vector<tlm::tlm_generic_payload*> childTranses;
    unsigned completedChildTranses = 0;
    explicit ParentExtension(std::vector<tlm::tlm_generic_payload*> _childTranses)
            : childTranses(std::move(_childTranses)) {}

public:
    ParentExtension() = delete;

    tlm_extension_base* clone() const override;
    void copy_from(const tlm_extension_base& ext) override;
    static void setExtension(tlm::tlm_generic_payload& parentTrans, std::vector<tlm::tlm_generic_payload*> childTranses);
    const std::vector<tlm::tlm_generic_payload*>& getChildTranses();
    bool notifyChildTransCompletion();
    static bool notifyChildTransCompletion(tlm::tlm_generic_payload& trans);
};

class EccExtension : public tlm::tlm_extension<EccExtension>
{
public:
    tlm_extension_base* clone() const override
    {
        return new EccExtension;
    }

    void copy_from(tlm_extension_base const &ext) override
    {    
        auto const &cpyFrom = static_cast<EccExtension const &>(ext);
    }
};

} // namespace DRAMSys

#endif // DRAMEXTENSIONS_H
