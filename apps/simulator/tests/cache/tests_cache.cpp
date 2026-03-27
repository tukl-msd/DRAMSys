/*
 * Copyright (c) 2023, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#include "ListInitiator.h"
#include "TargetMemory.h"

#include <simulator/Cache.h>
#include <DRAMSys/common/MemoryManager.h>

#include <gtest/gtest.h>

class SystemCTest : public testing::Test
{
public:
    ~SystemCTest() override { 
        sc_core::sc_curr_simcontext = new sc_core::sc_simcontext();
        sc_core::sc_default_global_context = sc_core::sc_curr_simcontext;
    }
};

class DirectMappedCache : public SystemCTest
{
protected:
    DirectMappedCache() :
        memoryManager(true),
        initiator("ListInitiator", memoryManager),
        target("TargetMemory",
               sc_core::sc_time(1, sc_core::SC_NS),
               sc_core::sc_time(10, sc_core::SC_NS)),
        cache("Cache",
              32768,
              1,
              32,
              8,
              8,
              8,
              true,
              sc_core::sc_time(1, sc_core::SC_NS),
              5,
              memoryManager)
    {
        initiator.iSocket.bind(cache.tSocket);
        cache.iSocket.bind(target.tSocket);
    }

    DRAMSys::MemoryManager memoryManager;
    ListInitiator initiator;
    TargetMemory target;
    Cache cache;
};

TEST_F(DirectMappedCache, Basic)
{
    using sc_core::SC_NS;
    using sc_core::sc_time;
    using Command = ListInitiator::TestTransactionData::Command;

    std::vector<ListInitiator::TestTransactionData> list{
        // Test miss
        {sc_time(0, SC_NS), sc_time(17, SC_NS), Command::Read, 0x0, 4, 0x0},

        // Test secondary miss
        {sc_time(1, SC_NS), sc_time(18, SC_NS), Command::Read, 0x0, 4, 0x0},

        // Test hit
        {sc_time(100, SC_NS), sc_time(106, SC_NS), Command::Read, 0x0, 4, 0x0},

        // Test write hit
        {sc_time(200, SC_NS), sc_time(206, SC_NS), Command::Write, 0x0, 4, 0x8},

        // Test eviction
        {sc_time(300, SC_NS), sc_time(317, SC_NS), Command::Write, 1024 * 32, 4, 0x0}};

    initiator.appendTestTransactionList(list);
    sc_core::sc_start();
}

// Does not work yet
// Unclear if a snoop should even happen when the line eviction fails
// TEST_F(DirectMappedCache, WriteBufferSnooping)
// {
//     using sc_core::SC_NS;
//     using sc_core::sc_time;
//     using Command = ListInitiator::TestTransactionData::Command;

//     std::vector<ListInitiator::TestTransactionData> list{
//         // Allocate line
//         {sc_time(0, SC_NS), sc_time(17, SC_NS), Command::Write, 0x0, 4, 0x0},

//         // Evict line
//         {sc_time(100, SC_NS), sc_time(117, SC_NS), Command::Read, 1024 * 32, 4, 0x0},

//         // Snoop from write buffer
//         {sc_time(102, SC_NS), sc_time(108, SC_NS), Command::Read, 0x0, 4, 0x0},
//     };

//     initiator.appendTestTransactionList(list);
//     sc_core::sc_start();
// }
