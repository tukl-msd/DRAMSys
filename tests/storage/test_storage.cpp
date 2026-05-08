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

#include <gtest/gtest.h>

#include <DRAMSys/DRAMSys.h>
#include <DRAMSys/controller/McConfig.h>
#include <DRAMSys/simulation/Dram.h>

#include <tlm_utils/simple_initiator_socket.h>

class SystemCTest : public testing::Test
{
public:
    ~SystemCTest() override
    {
        sc_core::sc_curr_simcontext = new sc_core::sc_simcontext();
        sc_core::sc_default_global_context = sc_core::sc_curr_simcontext;
    }
};

class StorageTests : public SystemCTest
{
protected:
    StorageTests() :
        dramsys("StorageDRAMSys", DRAMSys::Config::from_path("storage/config.json")),
        mm(true),
        initiator("initiator", mm)
    {
        initiator.iSocket.bind(dramsys.tSocket);
    }

    DRAMSys::DRAMSys dramsys;
    DRAMSys::MemoryManager mm;
    ListInitiator initiator;
};

TEST_F(StorageTests, Basic)
{
    using sc_core::SC_NS;
    using sc_core::sc_time;
    using Command = ListInitiator::TestTransactionData::Command;

    std::vector<ListInitiator::TestTransactionData> list{
        {sc_time(0, SC_NS), Command::Write, 0x0, {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                  0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                  0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                  0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF}},
        {sc_time(1, SC_NS), Command::Write, 0x20, {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                   0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                   0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                   0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF}},
        {sc_time(2, SC_NS), Command::Read, 0x0, {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF}},
        {sc_time(3, SC_NS), Command::Read, 0x20, {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                  0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                  0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                                  0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF}},
        {sc_time(5, SC_NS), Command::Write, 1024, std::vector<uint8_t>(32, 0xAA)},
        {sc_time(6, SC_NS), Command::Read, 1024, std::vector<uint8_t>(32, 0xAA)}};

    for (auto trans : list)
        initiator.appendTestTransaction(trans);

    sc_core::sc_start();
}

TEST_F(StorageTests, Block)
{
    using sc_core::SC_NS;
    using sc_core::sc_time;
    using Command = ListInitiator::TestTransactionData::Command;
    for (uint i = 0; i < 256; i++)
    {
        ListInitiator::TestTransactionData writeTrans{
            sc_time(i, SC_NS), Command::Write, i * 32, std::vector<uint8_t>(32, i)};
        ListInitiator::TestTransactionData readTrans = writeTrans;
        readTrans.command = Command::Read;
        readTrans.startTime = sc_time(2000 + i, SC_NS);
        initiator.appendTestTransaction(writeTrans);
        initiator.appendTestTransaction(readTrans);
    }
    sc_core::sc_start();
}
