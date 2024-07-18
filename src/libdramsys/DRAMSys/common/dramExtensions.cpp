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
 */

#include "dramExtensions.h"

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

ArbiterExtension::ArbiterExtension(Thread thread,
                                   Channel channel,
                                   uint64_t threadPayloadID,
                                   const sc_core::sc_time& timeOfGeneration) :
    thread(thread),
    channel(channel),
    threadPayloadID(threadPayloadID),
    timeOfGeneration(timeOfGeneration)
{
}

void ArbiterExtension::setAutoExtension(tlm::tlm_generic_payload& trans,
                                        Thread thread,
                                        Channel channel)
{
    auto* extension = trans.get_extension<ArbiterExtension>();

    if (extension != nullptr)
    {
        extension->thread = thread;
        extension->channel = channel;
        extension->threadPayloadID = 0;
        extension->timeOfGeneration = SC_ZERO_TIME;
    }
    else
    {
        extension = new ArbiterExtension(thread, channel, 0, SC_ZERO_TIME);
        trans.set_auto_extension(extension);
    }
}

void ArbiterExtension::setExtension(tlm::tlm_generic_payload& trans,
                                    Thread thread,
                                    Channel channel,
                                    uint64_t threadPayloadID,
                                    const sc_core::sc_time& timeOfGeneration)
{
    assert(trans.get_extension<ArbiterExtension>() == nullptr);
    auto* extension = new ArbiterExtension(thread, channel, threadPayloadID, timeOfGeneration);
    trans.set_extension(extension);
}

void ArbiterExtension::setIDAndTimeOfGeneration(tlm::tlm_generic_payload& trans,
                                                uint64_t threadPayloadID,
                                                const sc_core::sc_time& timeOfGeneration)
{
    assert(trans.get_extension<ArbiterExtension>() != nullptr);

    auto* extension = trans.get_extension<ArbiterExtension>();
    extension->threadPayloadID = threadPayloadID;
    extension->timeOfGeneration = timeOfGeneration;
}

tlm_extension_base* ArbiterExtension::clone() const
{
    return new ArbiterExtension(thread, channel, threadPayloadID, timeOfGeneration);
}

void ArbiterExtension::copy_from(const tlm_extension_base& ext)
{
    const auto& cpyFrom = dynamic_cast<const ArbiterExtension&>(ext);
    thread = cpyFrom.thread;
    channel = cpyFrom.channel;
    threadPayloadID = cpyFrom.threadPayloadID;
    timeOfGeneration = cpyFrom.timeOfGeneration;
}

Thread ArbiterExtension::getThread() const
{
    return thread;
}

Channel ArbiterExtension::getChannel() const
{
    return channel;
}

uint64_t ArbiterExtension::getThreadPayloadID() const
{
    return threadPayloadID;
}

sc_core::sc_time ArbiterExtension::getTimeOfGeneration() const
{
    return timeOfGeneration;
}

const ArbiterExtension& ArbiterExtension::getExtension(const tlm::tlm_generic_payload& trans)
{
    return *trans.get_extension<ArbiterExtension>();
}

Thread ArbiterExtension::getThread(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ArbiterExtension>()->thread;
}

Channel ArbiterExtension::getChannel(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ArbiterExtension>()->channel;
}

uint64_t ArbiterExtension::getThreadPayloadID(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ArbiterExtension>()->threadPayloadID;
}

sc_time ArbiterExtension::getTimeOfGeneration(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ArbiterExtension>()->timeOfGeneration;
}

ControllerExtension::ControllerExtension(uint64_t channelPayloadID,
                                         Rank rank,
                                         BankGroup bankGroup,
                                         Bank bank,
                                         Row row,
                                         Column column,
                                         unsigned int burstLength) :
    channelPayloadID(channelPayloadID),
    rank(rank),
    bankGroup(bankGroup),
    bank(bank),
    row(row),
    column(column),
    burstLength(burstLength)
{
}

void ControllerExtension::setAutoExtension(tlm::tlm_generic_payload& trans,
                                           uint64_t channelPayloadID,
                                           Rank rank,
                                           BankGroup bankGroup,
                                           Bank bank,
                                           Row row,
                                           Column column,
                                           unsigned int burstLength)
{
    auto* extension = trans.get_extension<ControllerExtension>();

    if (extension != nullptr)
    {
        extension->channelPayloadID = channelPayloadID;
        extension->rank = rank;
        extension->bankGroup = bankGroup;
        extension->bank = bank;
        extension->row = row;
        extension->column = column;
        extension->burstLength = burstLength;
    }
    else
    {
        extension = new ControllerExtension(
            channelPayloadID, rank, bankGroup, bank, row, column, burstLength);
        trans.set_auto_extension(extension);
    }
}

void ControllerExtension::setExtension(tlm::tlm_generic_payload& trans,
                                       uint64_t channelPayloadID,
                                       Rank rank,
                                       BankGroup bankGroup,
                                       Bank bank,
                                       Row row,
                                       Column column,
                                       unsigned int burstLength)
{
    assert(trans.get_extension<ControllerExtension>() == nullptr);
    auto* extension =
        new ControllerExtension(channelPayloadID, rank, bankGroup, bank, row, column, burstLength);
    trans.set_extension(extension);
}

tlm_extension_base* ControllerExtension::clone() const
{
    return new ControllerExtension(
        channelPayloadID, rank, bankGroup, bank, row, column, burstLength);
}

void ControllerExtension::copy_from(const tlm_extension_base& ext)
{
    const auto& cpyFrom = dynamic_cast<const ControllerExtension&>(ext);
    channelPayloadID = cpyFrom.channelPayloadID;
    rank = cpyFrom.rank;
    bankGroup = cpyFrom.bankGroup;
    bank = cpyFrom.bank;
    row = cpyFrom.row;
    column = cpyFrom.column;
    burstLength = cpyFrom.burstLength;
}

uint64_t ControllerExtension::getChannelPayloadID() const
{
    return channelPayloadID;
}

Rank ControllerExtension::getRank() const
{
    return rank;
}

BankGroup ControllerExtension::getBankGroup() const
{
    return bankGroup;
}

Bank ControllerExtension::getBank() const
{
    return bank;
}

Row ControllerExtension::getRow() const
{
    return row;
}

Column ControllerExtension::getColumn() const
{
    return column;
}

unsigned ControllerExtension::getBurstLength() const
{
    return burstLength;
}

const ControllerExtension& ControllerExtension::getExtension(const tlm::tlm_generic_payload& trans)
{
    return *trans.get_extension<ControllerExtension>();
}

uint64_t ControllerExtension::getChannelPayloadID(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ControllerExtension>()->channelPayloadID;
}

Rank ControllerExtension::getRank(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ControllerExtension>()->rank;
}

BankGroup ControllerExtension::getBankGroup(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ControllerExtension>()->bankGroup;
}

Bank ControllerExtension::getBank(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ControllerExtension>()->bank;
}

Row ControllerExtension::getRow(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ControllerExtension>()->row;
}

Column ControllerExtension::getColumn(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ControllerExtension>()->column;
}

unsigned ControllerExtension::getBurstLength(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ControllerExtension>()->burstLength;
}

tlm::tlm_extension_base* ChildExtension::clone() const
{
    return new ChildExtension(*parentTrans);
}

void ChildExtension::copy_from(const tlm::tlm_extension_base& ext)
{
    const auto& cpyFrom = dynamic_cast<const ChildExtension&>(ext);
    parentTrans = cpyFrom.parentTrans;
}

tlm::tlm_generic_payload& ChildExtension::getParentTrans()
{
    return *parentTrans;
}

tlm::tlm_generic_payload& ChildExtension::getParentTrans(tlm::tlm_generic_payload& childTrans)
{
    return childTrans.get_extension<ChildExtension>()->getParentTrans();
}

void ChildExtension::setExtension(tlm::tlm_generic_payload& childTrans,
                                  tlm::tlm_generic_payload& parentTrans)
{
    auto* extension = childTrans.get_extension<ChildExtension>();

    if (extension != nullptr)
    {
        extension->parentTrans = &parentTrans;
    }
    else
    {
        extension = new ChildExtension(parentTrans);
        childTrans.set_auto_extension(extension);
    }
}

bool ChildExtension::isChildTrans(const tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ChildExtension>() != nullptr;
}

tlm_extension_base* ParentExtension::clone() const
{
    return new ParentExtension(childTranses);
}

void ParentExtension::copy_from(const tlm_extension_base& ext)
{
    const auto& cpyFrom = dynamic_cast<const ParentExtension&>(ext);
    childTranses = cpyFrom.childTranses;
}

void ParentExtension::setExtension(tlm::tlm_generic_payload& parentTrans,
                                   std::vector<tlm::tlm_generic_payload*> childTranses)
{
    auto* extension = parentTrans.get_extension<ParentExtension>();

    if (extension != nullptr)
    {
        extension->childTranses = std::move(childTranses);
        extension->completedChildTranses = 0;
    }
    else
    {
        extension = new ParentExtension(std::move(childTranses));
        parentTrans.set_auto_extension(extension);
    }
}

const std::vector<tlm::tlm_generic_payload*>& ParentExtension::getChildTranses()
{
    return childTranses;
}

bool ParentExtension::notifyChildTransCompletion()
{
    completedChildTranses++;
    if (completedChildTranses == childTranses.size())
    {
        std::for_each(childTranses.begin(),
                      childTranses.end(),
                      [](tlm::tlm_generic_payload* childTrans) { childTrans->release(); });
        childTranses.clear();
        return true;
    }

    return false;
}

bool ParentExtension::notifyChildTransCompletion(tlm::tlm_generic_payload& trans)
{
    return trans.get_extension<ParentExtension>()->notifyChildTransCompletion();
}

} // namespace DRAMSys
