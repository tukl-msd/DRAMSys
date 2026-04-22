/*
 * Copyright (c) 2022, RPTU Kaiserslautern-Landau
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

#include "AddressDecoderConfigs.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <DRAMSys/configuration/memspec/MemSpecDDR4.h>
#include <DRAMSys/simulation/AddressDecoder.h>

class AddressDecoderFixture : public ::testing::Test
{
protected:
    AddressDecoderFixture() :
        addressMappingConfig(nlohmann::json::parse(addressMappingJsonString)
                                 .at("addressmapping")
                                 .get<DRAMSys::Config::AddressMapping>()),
        addressDecoder(addressMappingConfig)
    {
    }

    DRAMSys::Config::AddressMapping addressMappingConfig;
    DRAMSys::AddressDecoder addressDecoder;
};


TEST_F(AddressDecoderFixture, Decoding)
{
    uint64_t address = 0x3A59'1474;
    auto decodedAddress = addressDecoder.decodeAddress(address);

    unsigned int channel = decodedAddress.channel;
    unsigned int rank = decodedAddress.rank;
    unsigned int bankgroup = decodedAddress.bankgroup;
    unsigned int bank = decodedAddress.bank;
    unsigned int row = decodedAddress.row;
    unsigned int column = decodedAddress.column;

    EXPECT_EQ(channel, 0);
    EXPECT_EQ(rank, 0);
    EXPECT_EQ(bankgroup, 3);
    EXPECT_EQ(bank, 12);
    EXPECT_EQ(row, 29874);
    EXPECT_EQ(column, 170);
}

TEST_F(AddressDecoderFixture, DecodingNP2Failure)
{
    addressMappingConfig = DRAMSys::Config::AddressMapping(nlohmann::json::parse(validAddressMappingJsonString)
                                 .at("addressmapping")
                                 .get<DRAMSys::Config::AddressMapping>());

    auto memSpec = std::make_unique<const DRAMSys::MemSpecDDR4>(
                    nlohmann::json::parse(validNP2MemSpecJsonString)
                    .at("memspec").get< DRAMUtils::MemSpec::MemSpecDDR4>());

    addressDecoder = DRAMSys::AddressDecoder(addressMappingConfig);
    addressDecoder.plausibilityCheck(*memSpec);

    uint64_t address = 0x3A59'1478;
    std::ignore = addressDecoder.decodeAddress(address);
    // EXPECT_EQ(trans.get_response_status(), tlm::TLM_ADDRESS_ERROR_RESPONSE);

}

TEST_F(AddressDecoderFixture, DecodingNP2Success)
{
    addressMappingConfig = DRAMSys::Config::AddressMapping(nlohmann::json::parse(validAddressMappingJsonString)
                                 .at("addressmapping")
                                 .get<DRAMSys::Config::AddressMapping>());

    auto memSpec = std::make_unique<const DRAMSys::MemSpecDDR4>(
                    nlohmann::json::parse(validNP2MemSpecJsonString)
                    .at("memspec").get< DRAMUtils::MemSpec::MemSpecDDR4>());

    addressDecoder = DRAMSys::AddressDecoder(addressMappingConfig);
    addressDecoder.plausibilityCheck(*memSpec);

    uint64_t address = 0x3A59'1477;
    auto decodedAddress = addressDecoder.decodeAddress(address);

    unsigned int channel = decodedAddress.channel;
    unsigned int rank = decodedAddress.rank;
    unsigned int bankgroup = decodedAddress.bankgroup;
    unsigned int bank = decodedAddress.bank;
    unsigned int row = decodedAddress.row;
    unsigned int column = decodedAddress.column;

    EXPECT_EQ(channel, 0);
    EXPECT_EQ(rank, 0);
    EXPECT_EQ(bankgroup, 1);
    EXPECT_EQ(bank, 4);
    EXPECT_EQ(row, 7468);
    EXPECT_EQ(column, 558);
    // EXPECT_EQ(trans.get_response_status(), tlm::TLM_INCOMPLETE_RESPONSE);
}

TEST_F(AddressDecoderFixture, Encoding)
{
    unsigned int channel = 0;
    unsigned int rank = 0;
    unsigned int stack = 0;
    unsigned int bankgroup = 3;
    unsigned int bank = 12;
    unsigned int row = 29874;
    unsigned int column = 170;

    DRAMSys::DecodedAddress decodedAddress(channel, rank, stack, bankgroup, bank, row, column);

    uint64_t address = addressDecoder.encodeAddress(decodedAddress);
    EXPECT_EQ(address, 0x3A59'1474);
}

TEST_F(AddressDecoderFixture, DeEncoding)
{
    std::array testAddresses{std::uint64_t(0x3A59'1474),
                             std::uint64_t(0x0000'0000),
                             std::uint64_t(0x2FFA'1230),
                             std::uint64_t(0x0001'FFF0)};

    for (auto address : testAddresses)
    {
        DRAMSys::DecodedAddress decodedAddress = addressDecoder.decodeAddress(address);
        uint64_t encodedAddress = addressDecoder.encodeAddress(decodedAddress);

        EXPECT_EQ(encodedAddress, address);
    }
}

class AddressDecoderPlausibilityFixture : public ::testing::Test
{
protected:
    std::unique_ptr<DRAMSys::AddressDecoder> addressDecoder = nullptr;
    std::unique_ptr<DRAMSys::MemSpecDDR4> memSpecDDR4 = nullptr;

    void setupDecoder(const std::string_view& memSpecJson,
                      const std::string_view& addressMappingJson)
    {
         DRAMUtils::MemSpec::MemSpecDDR4 memSpec =
            nlohmann::json::parse(memSpecJson).at("memspec").get< DRAMUtils::MemSpec::MemSpecDDR4>();
        memSpecDDR4 = std::make_unique<DRAMSys::MemSpecDDR4>(memSpec);

        DRAMSys::Config::AddressMapping addressMappingConfig =
            nlohmann::json::parse(addressMappingJson)
                .at("addressmapping")
                .get<DRAMSys::Config::AddressMapping>();
        addressDecoder = std::make_unique<DRAMSys::AddressDecoder>(addressMappingConfig);
    }
};

TEST_F(AddressDecoderPlausibilityFixture, ValidPlausibilityCheck)
{
    setupDecoder(validMemSpecJsonString, validAddressMappingJsonString);

    EXPECT_NO_THROW(addressDecoder->plausibilityCheck(*memSpecDDR4));
}

TEST_F(AddressDecoderPlausibilityFixture, ValidNP2PlausibilityCheck)
{
    setupDecoder(validNP2MemSpecJsonString, validAddressMappingJsonString);

    EXPECT_NO_THROW(addressDecoder->plausibilityCheck(*memSpecDDR4));
}

TEST_F(AddressDecoderPlausibilityFixture, InvalidMaxAddress)
{
    setupDecoder(invalidMaxAddressMemSpecJsonString, validAddressMappingJsonString);

    EXPECT_DEATH(addressDecoder->plausibilityCheck(*memSpecDDR4), "");
}

TEST_F(AddressDecoderPlausibilityFixture, DuplicateBits)
{
    setupDecoder(validMemSpecJsonString, addressMappingWithDuplicatesJsonString);

    EXPECT_DEATH(addressDecoder->plausibilityCheck(*memSpecDDR4), "");
}

TEST_F(AddressDecoderPlausibilityFixture, NonContinuousByteBits)
{
    setupDecoder(validMemSpecJsonString, nonContinuousByteBitsAddressMappingJsonString);

    EXPECT_DEATH(addressDecoder->plausibilityCheck(*memSpecDDR4), "");
}

TEST_F(AddressDecoderPlausibilityFixture, InvalidChannelMapping)
{
    setupDecoder(invalidChannelMemSpecJsonString, validAddressMappingJsonString);

    EXPECT_DEATH(addressDecoder->plausibilityCheck(*memSpecDDR4), "");
}

TEST_F(AddressDecoderPlausibilityFixture, InvalidBankGroups)
{
    setupDecoder(invalidBankGroupMemSpecJsonString, validAddressMappingJsonString);

    EXPECT_DEATH(addressDecoder->plausibilityCheck(*memSpecDDR4), "");
}

TEST_F(AddressDecoderPlausibilityFixture, InvalidRanks)
{
    setupDecoder(invalidRanksMemSpecJsonString, validAddressMappingJsonString);

    EXPECT_DEATH(addressDecoder->plausibilityCheck(*memSpecDDR4), "");
}
