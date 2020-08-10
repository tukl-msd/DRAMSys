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
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 *    Luiza Correa
 */

#include "utils.h"
#include <string>
#include <tlm.h>
#include <fstream>
#include "dramExtensions.h"
#include <sstream>

using namespace tlm;
using json = nlohmann::json;

bool TimeInterval::timeIsInInterval(sc_time time)
{
    return (start < time && time < end);
}

bool TimeInterval::intersects(TimeInterval other)
{
    return other.timeIsInInterval(this->start)
           || this->timeIsInInterval(other.start);
}

sc_time TimeInterval::getLength()
{
    if (end > start)
        return end - start;
    else
        return start - end;
}

std::string getPhaseName(tlm_phase phase)
{
    std::ostringstream oss;
    oss << phase;
    return oss.str();
}

json parseJSON(std::string path)
{
    try
    {
        // parsing input with a syntax error
        json j = json::parse(std::ifstream(path));
        return j;
    }
    catch (json::parse_error& e)
    {
        // output exception information
        std::cout << "Error while trying to parse file: " << path << '\n'
                  << "message: " << e.what() << std::endl;
    }
}

bool parseBool(json &obj, std::string name)
{
    if (!obj.empty())
    {
        if (obj.is_boolean())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': bool");
    }
    else
        SC_REPORT_FATAL("Query json", ("Parameter '" + name + "' does not exist.").c_str());
}

unsigned int parseUint(json &obj, std::string name)
{
    if (!obj.empty())
    {
        if (obj.is_number_unsigned())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': unsigned int");
    }
    else
        SC_REPORT_FATAL("Query json", ("Parameter '" + name + "' does not exist.").c_str());
}

double parseUdouble(json &obj, std::string name)
{
    if (!obj.empty())
    {
        if (obj.is_number() && (obj > 0))
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': positive double");
    }
    else
        SC_REPORT_FATAL("Query json", ("Parameter '" + name + "' does not exist.").c_str());
}

std::string parseString(json &obj, std::string name)
{
    if (!obj.empty())
    {
        if (obj.is_string())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': string");
    }
    else
        SC_REPORT_FATAL("Query json", ("Parameter '" + name + "' does not exist.").c_str());
}

void setUpDummy(tlm_generic_payload &payload, uint64_t payloadID, Rank rank, BankGroup bankgroup, Bank bank)
{
    payload.set_address(bank.getStartAddress());
    payload.set_command(TLM_READ_COMMAND);
    payload.set_data_length(0);
    payload.set_response_status(TLM_OK_RESPONSE);
    payload.set_dmi_allowed(false);
    payload.set_byte_enable_length(0);
    payload.set_streaming_width(0);
    payload.set_extension(new DramExtension(Thread(UINT_MAX), rank, bankgroup,
                                            bank, Row(0), Column(0), 0, payloadID));
}
