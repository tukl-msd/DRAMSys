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

struct LineContent
{
    sc_time sendingTime;
    tlm::tlm_command cmd;
    uint64_t addr;
    std::vector<unsigned char> data;
};

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
        burstlength = Configuration::getInstance().memSpec->burstLength;
        dataLength = Configuration::getInstance().getBytesPerBurst();
        lineCnt = 0;

        parseTraceFile();
    }

    void parseTraceFile()
    {
        std::string line;
        unsigned parsedLines = 0;
        lineContents.clear();
        while (file && !file.eof() && parsedLines < 10000)
        {
            // Get a new line from the input file.
            std::getline(file, line);
            lineCnt++;
            // If the line starts with '#' (commented lines) the transaction is ignored.
            if (line.empty() || line.at(0) == '#')
                continue;

            parsedLines++;
            lineContents.emplace_back();
            LineContent &content = lineContents.back();

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
            content.sendingTime = std::stoull(time.c_str()) * playerClk;

            // Get the command.
            iss >> command;
            if (command.empty())
                SC_REPORT_FATAL("StlPlayer",
                ("Malformed trace file. Command could not be found (line " + std::to_string(
                    lineCnt) + ").").c_str());

            if (command == "read")
                content.cmd = tlm::TLM_READ_COMMAND;
            else if (command == "write")
                content.cmd = tlm::TLM_WRITE_COMMAND;
            else
                SC_REPORT_FATAL("StlPlayer",
                    (std::string("Corrupted tracefile, command ") + command +
                        std::string(" unknown")).c_str());

            // Get the address.
            iss >> address;
            if (address.empty())
                SC_REPORT_FATAL("StlPlayer",
                ("Malformed trace file. Address could not be found (line "
                    + std::to_string(lineCnt) + ").").c_str());
            content.addr = std::stoull(address.c_str(), nullptr, 16);

            // Get the data if necessary.
            if (storageEnabled && content.cmd == tlm::TLM_WRITE_COMMAND)
            {
                // The input trace file must provide the data to be stored into the memory.
                iss >> dataStr;
                if (dataStr.empty())
                    SC_REPORT_FATAL("StlPlayer",
                    ("Malformed trace file. Data information could not be found (line " + std::to_string(
                        lineCnt) + ").").c_str());

                // Check if data length in the trace file is correct.
                // We need two characters to represent 1 byte in hexadecimal. Offset for 0x prefix.
                if (dataStr.length() != (dataLength * 2 + 2))
                    SC_REPORT_FATAL("StlPlayer",
                    ("Data in the trace file has an invalid length (line " + std::to_string(
                        lineCnt) + ").").c_str());

                // Set data
                for (unsigned i = 0; i < dataLength; i++)
                    content.data.emplace_back((unsigned char)std::stoi(dataStr.substr(i * 2 + 2, 2).c_str(), nullptr, 16));
            }
        }
        lineIterator = lineContents.cbegin();
    }

    void nextPayload()
    {
        if (lineIterator == lineContents.cend())
        {
            // Read new lines of file.
            parseTraceFile();
            if (lineIterator == lineContents.cend())
            {
                // The file is empty. Nothing more to do.
                this->finish();
                return;
            }
        }

        numberOfTransactions++;

        // Allocate a generic payload for this request.
        tlm::tlm_generic_payload *payload = this->allocatePayload();
        payload->acquire();

        // Fill up the payload.
        payload->set_address(lineIterator->addr);
        payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        payload->set_dmi_allowed(false);
        payload->set_byte_enable_length(0);
        payload->set_streaming_width(burstlength);
        payload->set_data_length(dataLength);
        payload->set_command(lineIterator->cmd);
        std::copy(lineIterator->data.begin(), lineIterator->data.end(), payload->get_data_ptr());

        if (relative == false)
        {
            // Send the transaction directly or schedule it to be sent in the future.
            if (lineIterator->sendingTime <= sc_time_stamp())
                this->payloadEventQueue.notify(*payload, tlm::BEGIN_REQ, SC_ZERO_TIME);
            else
                this->payloadEventQueue.notify(*payload, tlm::BEGIN_REQ,
                                               lineIterator->sendingTime - sc_time_stamp());
        }
        else
            payloadEventQueue.notify(*payload, tlm::BEGIN_REQ, lineIterator->sendingTime);

        lineIterator++;
    }

private:
    std::ifstream file;
    unsigned int lineCnt;

    unsigned int burstlength;
    unsigned int dataLength;
    sc_time playerClk;  // May be different from the memory clock!

    std::vector<LineContent> lineContents;
    std::vector<LineContent>::const_iterator lineIterator;
};

#endif // STLPLAYER_H
