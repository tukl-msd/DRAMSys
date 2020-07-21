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
 *    Éder F. Zulian
 *    Felipe S. Prado
 */

#ifndef STLPLAYER_H
#define STLPLAYER_H

#include <sstream>
#include "TracePlayer.h"

template<bool relative>
class StlPlayer : public TracePlayer
{
public:
    StlPlayer(sc_module_name name,
              std::string pathToTrace,
              sc_time playerClk,
              TracePlayerListener *listener) :
        TracePlayer(name, listener),
        file(pathToTrace)
    {
        if (!file.is_open())
            SC_REPORT_FATAL(0, (std::string("Could not open trace ") + pathToTrace).c_str());

        this->playerClk = playerClk;
        this->burstlength = Configuration::getInstance().memSpec->burstLength;
        this->dataLength = Configuration::getInstance().getBytesPerBurst();
        this->lineCnt = 0;
    }

    void nextPayload()
    {
        std::string line;
        while (line.empty() && file) {
            // Get a new line from the input file.
            std::getline(file, line);
            lineCnt++;
            // If the line starts with '#' (commented lines) the transaction is ignored.
            if (!line.empty() && line.at(0) == '#')
                line.clear();
        }

        if (!file) {
            // The file is empty. Nothing more to do.
            this->finish();
            return;
        } else {
            numberOfTransactions++;
        }
        // Allocate a generic payload for this request.
        tlm::tlm_generic_payload *payload = this->allocatePayload();
        payload->acquire();
        unsigned char *data = payload->get_data_ptr();

        // Trace files MUST provide timestamp, command and address for every
        // transaction. The data information depends on the storage mode
        // configuration.
        std::string time;
        std::string command;
        std::string address;
        std::string dataStr;

        std::istringstream iss(line);

        // Get the timestamp for the transaction.
        iss >> time;
        if (time.empty())
            SC_REPORT_FATAL("StlPlayer",
                            ("Malformed trace file. Timestamp could not be found (line " + std::to_string(
                                 lineCnt) + ").").c_str());
        sc_time sendingTime = std::stoull(time.c_str()) * playerClk;

        // Get the command.
        iss >> command;
        if (command.empty())
            SC_REPORT_FATAL("StlPlayer",
                            ("Malformed trace file. Command could not be found (line " + std::to_string(
                                 lineCnt) + ").").c_str());
        enum tlm::tlm_command cmd;
        if (command == "read") {
            cmd = tlm::TLM_READ_COMMAND;
        } else if (command == "write") {
            cmd = tlm::TLM_WRITE_COMMAND;
        } else {
            SC_REPORT_FATAL("StlPlayer",
                            (std::string("Corrupted tracefile, command ") + command +
                             std::string(" unknown")).c_str());
        }

        // Get the address.
        iss >> address;
        if (address.empty())
            SC_REPORT_FATAL("StlPlayer",
                            ("Malformed trace file. Address could not be found (line "
                             + std::to_string(lineCnt) + ").").c_str());
        unsigned long long addr = std::stoull(address.c_str(), 0, 16);

        // Get the data if necessary.
        if (storageEnabled && cmd == tlm::TLM_WRITE_COMMAND)
        {
            // The input trace file must provide the data to be stored into the memory.
            iss >> dataStr;
            if (dataStr.empty())
                SC_REPORT_FATAL("StlPlayer",
                                ("Malformed trace file. Data information could not be found (line " + std::to_string(
                                     lineCnt) + ").").c_str());

            // Check if data length in the trace file is correct. We need two characters to represent 1 byte in hexadecimal. Offset for 0x prefix.
            if (dataStr.length() != (dataLength * 2 + 2))
                SC_REPORT_FATAL("StlPlayer",
                                ("Data in the trace file has an invalid length (line " + std::to_string(
                                     lineCnt) + ").").c_str());

            // Set data
            for (unsigned i = 0; i < dataLength; i++)
                data[i] = (unsigned char)std::stoi(dataStr.substr(i * 2 + 2, 2).c_str(), 0, 16);
        }

        // Fill up the payload.
        payload->set_address(addr);
        payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        payload->set_dmi_allowed(false);
        payload->set_byte_enable_length(0);
        payload->set_streaming_width(burstlength);
        payload->set_data_length(dataLength);
        payload->set_data_ptr(data);
        payload->set_command(cmd);

        if (relative == false)
        {
            // Send the transaction directly or schedule it to be sent in the future.
            if (sendingTime <= sc_time_stamp())
                this->payloadEventQueue.notify(*payload, tlm::BEGIN_REQ, SC_ZERO_TIME);
            else
                this->payloadEventQueue.notify(*payload, tlm::BEGIN_REQ,
                                               sendingTime - sc_time_stamp());
        }
        else
            payloadEventQueue.notify(*payload, tlm::BEGIN_REQ, sendingTime);
    }

private:
    std::ifstream file;
    unsigned int lineCnt;

    unsigned int burstlength;
    unsigned int dataLength;
    sc_time playerClk;  // May be different from the memory clock!
};

#endif // STLPLAYER_H

