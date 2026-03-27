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

#include <gtest/gtest.h>

#include <DRAMSys/DRAMSys.h>
#include <DRAMSys/controller/McConfig.h>
#include <DRAMSys/simulation/Dram.h>

#include <tlm_utils/simple_initiator_socket.h>

class SystemCTest : public testing::Test
{
public:
    ~SystemCTest() override { sc_core::sc_get_curr_simcontext()->reset(); }
};

class BTransportNoStorage : public SystemCTest
{
protected:
    BTransportNoStorage() :
        no_storage_config(DRAMSys::Config::from_path("b_transport/configs/no_storage.json")),
        dramSysNoStorage("NoStorageDRAMSys", no_storage_config)
    {
    }

    DRAMSys::Config::Configuration no_storage_config;
    DRAMSys::DRAMSys dramSysNoStorage;
};

class BTransportStorage : public SystemCTest
{
protected:
    BTransportStorage() :
        storage_config(DRAMSys::Config::from_path("b_transport/configs/storage.json")),
        dramSysStorage("StorageDRAMSys", storage_config)
    {
    }

    DRAMSys::Config::Configuration storage_config;
    DRAMSys::DRAMSys dramSysStorage;
};

struct BlockingInitiator : sc_core::sc_module
{
    tlm_utils::simple_initiator_socket<BlockingInitiator> iSocket;
    static constexpr std::array<uint64_t, 8> TEST_DATA = {0xDEADBEEF};
    DRAMSys::DRAMSys const& dramSys;

    BlockingInitiator(sc_core::sc_module_name const& name, DRAMSys::DRAMSys const& dramSys) :
        sc_core::sc_module(name),
        dramSys(dramSys)
    {
        SC_THREAD(readAccess);
        SC_THREAD(writeAccess);
    }

    void readAccess()
    {
        tlm::tlm_generic_payload payload;
        payload.set_command(tlm::TLM_READ_COMMAND);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        iSocket->b_transport(payload, delay);

        EXPECT_EQ(delay, dramSys.getMcConfig().blockingReadDelay);
    }

    void writeAccess()
    {
        std::array<uint64_t, 8> data{TEST_DATA};
        tlm::tlm_generic_payload payload;
        payload.set_command(tlm::TLM_WRITE_COMMAND);
        payload.set_data_length(64);
        payload.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        iSocket->b_transport(payload, delay);

        EXPECT_EQ(delay, dramSys.getMcConfig().blockingWriteDelay);
    }
};

TEST_F(BTransportNoStorage, RWDelay)
{
    BlockingInitiator initiator("initiator", dramSysNoStorage);
    initiator.iSocket.bind(dramSysNoStorage.tSocket);

    sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_US));
}

TEST_F(BTransportStorage, RWDelay)
{
    BlockingInitiator initiator("initiator", dramSysStorage);
    initiator.iSocket.bind(dramSysStorage.tSocket);

    sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_US));
}

TEST_F(BTransportStorage, DataWritten)
{
    BlockingInitiator initiator("initiator", dramSysStorage);
    initiator.iSocket.bind(dramSysStorage.tSocket);

    sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_US));

    // Debug transaction to check if data really has been written
    std::array<uint64_t, 8> data{};
    tlm::tlm_generic_payload payload;
    payload.set_command(tlm::TLM_READ_COMMAND);
    payload.set_data_length(64);
    payload.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
    initiator.iSocket->transport_dbg(payload);

    EXPECT_EQ(data, BlockingInitiator::TEST_DATA);
}

TEST_F(BTransportNoStorage, Warning)
{
    BlockingInitiator initiator("initiator", dramSysNoStorage);
    initiator.iSocket.bind(dramSysNoStorage.tSocket);

    // Redirect stdout to buffer
    std::stringstream buffer;
    std::streambuf* sbuf = std::cout.rdbuf();
    std::cout.rdbuf(buffer.rdbuf());

    sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_US));

    // Try to find the warning string
    std::string output = buffer.str();
    auto warning_pos = output.find(DRAMSys::Dram::BLOCKING_WARNING);

    // Warning should be printed once ...
    EXPECT_NE(warning_pos, std::string::npos);

    // ... but not twice
    warning_pos = output.find(DRAMSys::Dram::BLOCKING_WARNING, warning_pos + 1);
    EXPECT_EQ(warning_pos, std::string::npos);

    // Restore stdout
    std::cout.rdbuf(sbuf);
}
