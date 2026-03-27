/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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
 *    Marco Mörz
 */

#include <DRAMSys/configuration/json/DRAMSysConfiguration.h>
#include <DRAMSys/configuration/json/MemSpec.h>

#include <DRAMUtils/util/json.h>
#include <DRAMUtils/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>

#include <fstream>
#include <gtest/gtest.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

using namespace DRAMSys::Config;

TEST(Configuration, FromToJson)
{
    std::ifstream file("configuration/reference.json");
    ASSERT_TRUE(file.is_open());

    json_t reference_json = json_t::parse(file);
    DRAMSys::Config::Configuration reference_configuration =
        reference_json["simulation"].get<DRAMSys::Config::Configuration>();

    json_t new_json;
    new_json["simulation"] = reference_configuration;

    EXPECT_EQ(new_json, reference_json);
}

TEST(Configuration, FromPath)
{
    Configuration config = from_path("configuration/reference.json");
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
