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
#include <vector>

#include <systemc>
#include <tlm>

namespace DRAMSys
{

enum class Thread : std::size_t;
enum class Channel : std::size_t;
enum class Rank : std::size_t;
enum class LogicalRank : std::size_t;
enum class PhysicalRank : std::size_t;
enum class DimmRank : std::size_t;
enum class BankGroup : std::size_t;
enum class Bank : std::size_t;
enum class Row : std::size_t;
enum class Column : std::size_t;

template <typename IndexType, typename ValueType>
class ControllerVector : private std::vector<ValueType>
{
public:
    using std::vector<ValueType>::vector;
    using std::vector<ValueType>::push_back;
    using std::vector<ValueType>::begin;
    using std::vector<ValueType>::end;
    using std::vector<ValueType>::front;

    typename std::vector<ValueType>::const_reference operator[](IndexType index) const
    {
        return std::vector<ValueType>::operator[](static_cast<std::size_t>(index));
    }

    typename std::vector<ValueType>::reference operator[](IndexType index)
    {
        return std::vector<ValueType>::operator[](static_cast<std::size_t>(index));
    }
};

class ArbiterExtension : public tlm::tlm_extension<ArbiterExtension>
{
public:
    static void setAutoExtension(tlm::tlm_generic_payload& trans, Thread thread, Channel channel);
    static void setExtension(tlm::tlm_generic_payload& trans,
                             Thread thread,
                             Channel channel,
                             uint64_t threadPayloadID,
                             const sc_core::sc_time& timeOfGeneration);
    static void setIDAndTimeOfGeneration(tlm::tlm_generic_payload& trans,
                                         uint64_t threadPayloadID,
                                         const sc_core::sc_time& timeOfGeneration);

    [[nodiscard]] tlm::tlm_extension_base* clone() const override;
    void copy_from(const tlm::tlm_extension_base& ext) override;

    [[nodiscard]] Thread getThread() const;
    [[nodiscard]] Channel getChannel() const;
    [[nodiscard]] uint64_t getThreadPayloadID() const;
    [[nodiscard]] sc_core::sc_time getTimeOfGeneration() const;

    static const ArbiterExtension& getExtension(const tlm::tlm_generic_payload& trans);
    static Thread getThread(const tlm::tlm_generic_payload& trans);
    static Channel getChannel(const tlm::tlm_generic_payload& trans);
    static uint64_t getThreadPayloadID(const tlm::tlm_generic_payload& trans);
    static sc_core::sc_time getTimeOfGeneration(const tlm::tlm_generic_payload& trans);

private:
    ArbiterExtension(Thread thread,
                     Channel channel,
                     uint64_t threadPayloadID,
                     const sc_core::sc_time& timeOfGeneration);
    Thread thread;
    Channel channel;
    uint64_t threadPayloadID;
    sc_core::sc_time timeOfGeneration;
};

class ControllerExtension : public tlm::tlm_extension<ControllerExtension>
{
public:
    static void setAutoExtension(tlm::tlm_generic_payload& trans,
                                 uint64_t channelPayloadID,
                                 Rank rank,
                                 BankGroup bankGroup,
                                 Bank bank,
                                 Row row,
                                 Column column,
                                 unsigned burstLength);

    static void setExtension(tlm::tlm_generic_payload& trans,
                             uint64_t channelPayloadID,
                             Rank rank,
                             BankGroup bankGroup,
                             Bank bank,
                             Row row,
                             Column column,
                             unsigned burstLength);

    // static ControllerExtension& getExtension(const tlm::tlm_generic_payload& trans);

    [[nodiscard]] tlm::tlm_extension_base* clone() const override;
    void copy_from(const tlm::tlm_extension_base& ext) override;

    [[nodiscard]] uint64_t getChannelPayloadID() const;
    [[nodiscard]] Rank getRank() const;
    [[nodiscard]] BankGroup getBankGroup() const;
    [[nodiscard]] Bank getBank() const;
    [[nodiscard]] Row getRow() const;
    [[nodiscard]] Column getColumn() const;
    [[nodiscard]] unsigned getBurstLength() const;

    static const ControllerExtension& getExtension(const tlm::tlm_generic_payload& trans);
    static uint64_t getChannelPayloadID(const tlm::tlm_generic_payload& trans);
    static Rank getRank(const tlm::tlm_generic_payload& trans);
    static BankGroup getBankGroup(const tlm::tlm_generic_payload& trans);
    static Bank getBank(const tlm::tlm_generic_payload& trans);
    static Row getRow(const tlm::tlm_generic_payload& trans);
    static Column getColumn(const tlm::tlm_generic_payload& trans);
    static unsigned getBurstLength(const tlm::tlm_generic_payload& trans);

private:
    ControllerExtension(uint64_t channelPayloadID,
                        Rank rank,
                        BankGroup bankGroup,
                        Bank bank,
                        Row row,
                        Column column,
                        unsigned burstLength);
    uint64_t channelPayloadID;
    Rank rank;
    BankGroup bankGroup;
    Bank bank;
    Row row;
    Column column;
    unsigned burstLength;
};

class ChildExtension : public tlm::tlm_extension<ChildExtension>
{
private:
    tlm::tlm_generic_payload* parentTrans;
    explicit ChildExtension(tlm::tlm_generic_payload& parentTrans) : parentTrans(&parentTrans) {}

public:
    // ChildExtension() = delete;

    [[nodiscard]] tlm::tlm_extension_base* clone() const override;
    void copy_from(const tlm::tlm_extension_base& ext) override;
    tlm::tlm_generic_payload& getParentTrans();
    static tlm::tlm_generic_payload& getParentTrans(tlm::tlm_generic_payload& childTrans);
    static void setExtension(tlm::tlm_generic_payload& childTrans,
                             tlm::tlm_generic_payload& parentTrans);
    static bool isChildTrans(const tlm::tlm_generic_payload& trans);
};

class ParentExtension : public tlm::tlm_extension<ParentExtension>
{
private:
    std::vector<tlm::tlm_generic_payload*> childTranses;
    unsigned completedChildTranses = 0;
    explicit ParentExtension(std::vector<tlm::tlm_generic_payload*> _childTranses) :
        childTranses(std::move(_childTranses))
    {
    }

public:
    ParentExtension() = delete;

    [[nodiscard]] tlm_extension_base* clone() const override;
    void copy_from(const tlm_extension_base& ext) override;
    static void setExtension(tlm::tlm_generic_payload& parentTrans,
                             std::vector<tlm::tlm_generic_payload*> childTranses);
    const std::vector<tlm::tlm_generic_payload*>& getChildTranses();
    bool notifyChildTransCompletion();
    static bool notifyChildTransCompletion(tlm::tlm_generic_payload& trans);
};

class EccExtension : public tlm::tlm_extension<EccExtension>
{
public:
    [[nodiscard]] tlm_extension_base* clone() const override { return new EccExtension; }

    void copy_from([[maybe_unused]] tlm_extension_base const& ext) override {}
};

} // namespace DRAMSys

#endif // DRAMEXTENSIONS_H
