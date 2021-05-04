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
 *    Eder F. Zulian
 *    Luiza Correa
 */

#ifndef UTILS_H
#define UTILS_H

#include <systemc.h>
#include <map>
#include <string>
#include <ostream>
#include <tlm.h>
#include <iomanip>
#include "dramExtensions.h"
#include <fstream>
#include <sstream>
#include "../common/third_party/nlohmann/single_include/nlohmann/json.hpp"

class TimeInterval
{
public:
    sc_time start, end;
    TimeInterval() : start(SC_ZERO_TIME), end(SC_ZERO_TIME) {}
    TimeInterval(sc_time start, sc_time end) : start(start), end(end) {}

    sc_time getLength();
    bool timeIsInInterval(sc_time time);
    bool intersects(TimeInterval other);
};

constexpr const char headline[] =
    "===========================================================================";

std::string getPhaseName(tlm::tlm_phase phase);

nlohmann::json parseJSON(std::string path);
bool parseBool(nlohmann::json &obj, std::string name);
unsigned int parseUint(nlohmann::json &obj, std::string name);
double parseUdouble(nlohmann::json &obj, std::string name);
std::string parseString(nlohmann::json &obj, std::string name);

void setUpDummy(tlm::tlm_generic_payload &payload, uint64_t channelPayloadID,
                Rank rank = Rank(0), BankGroup bankgroup = BankGroup(0), Bank bank = Bank(0));

#endif // UTILS_H

