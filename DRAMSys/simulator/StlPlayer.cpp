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
 *    Derek Christ
 */

#include "StlPlayer.h"

using namespace sc_core;
using namespace tlm;

StlPlayer::StlPlayer(const sc_module_name &name, const Configuration& config, const std::string &pathToTrace,
                     const sc_time &playerClk, unsigned int maxPendingReadRequests, unsigned int maxPendingWriteRequests,
                     bool addLengthConverter, TraceSetup& setup, bool relative) :
    TrafficInitiator(name, config, setup, maxPendingReadRequests, maxPendingWriteRequests,
                     config.memSpec->defaultBytesPerBurst, addLengthConverter),
    file(pathToTrace), relative(relative), playerClk(playerClk)
{
    currentBuffer = &lineContents[0];
    parseBuffer = &lineContents[1];

    if (!file.is_open())
        SC_REPORT_FATAL("StlPlayer", (std::string("Could not open trace ") + pathToTrace).c_str());
    else
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.size() > 1 && line[0] != '#')
                numberOfLines++;
        }
        file.clear();
        file.seekg(0);

        if (numberOfLines == 0)
            SC_REPORT_FATAL("StlPlayer", "Trace file is empty");
    }

    currentBuffer->reserve(lineBufferSize);
    parseBuffer->reserve(lineBufferSize);

    parseTraceFile();
    lineIterator = currentBuffer->cend();
}

StlPlayer::~StlPlayer()
{
    if (parserThread.joinable())
        parserThread.join();
}

void StlPlayer::sendNextPayload()
{
    if (lineIterator == currentBuffer->cend())
    {
        lineIterator = swapBuffers();
        if (lineIterator == currentBuffer->cend())
        {
            // The file is empty. Nothing more to do.
            finished = true;
            return;
        }
    }

    // Allocate a generic payload for this request.
    tlm_generic_payload& payload = setup.allocatePayload(lineIterator->dataLength);
    payload.acquire();

    // Fill up the payload.
    payload.set_address(lineIterator->address);
    payload.set_response_status(TLM_INCOMPLETE_RESPONSE);
    payload.set_dmi_allowed(false);
    payload.set_byte_enable_length(0);
    payload.set_data_length(lineIterator->dataLength);
    payload.set_command(lineIterator->command);
    std::copy(lineIterator->data.begin(), lineIterator->data.end(), payload.get_data_ptr());

    sc_time sendingTime;
    sc_time sendingOffset;

    if (transactionsSent == 0)
        sendingOffset = SC_ZERO_TIME;
    else
        sendingOffset = playerClk - (sc_time_stamp() % playerClk);

    if (!relative)
        sendingTime = std::max(sc_time_stamp() + sendingOffset, lineIterator->sendingTime);
    else
        sendingTime = sc_time_stamp() + sendingOffset + lineIterator->sendingTime;

    sendToTarget(payload, BEGIN_REQ, sendingTime - sc_time_stamp());

    transactionsSent++;

    if (payload.get_command() == tlm::TLM_READ_COMMAND)
        pendingReadRequests++;
    else if (payload.get_command() == tlm::TLM_WRITE_COMMAND)
        pendingWriteRequests++;

    PRINTDEBUGMESSAGE(name(), "Performing request #" + std::to_string(transactionsSent));
    lineIterator++;
}

void StlPlayer::parseTraceFile()
{
    unsigned parsedLines = 0;
    parseBuffer->clear();
    while (file && !file.eof() && parsedLines < lineBufferSize)
    {
        // Get a new line from the input file.
        std::string line;
        std::getline(file, line);
        lineCnt++;

        // If the line is empty (\n or \r\n) or starts with '#' (comment) the transaction is ignored.
        if (line.size() <= 1 || line.at(0) == '#')
            continue;

        parsedLines++;
        parseBuffer->emplace_back();
        LineContent &content = parseBuffer->back();

        // Trace files MUST provide timestamp, command and address for every
        // transaction. The data information depends on the storage mode
        // configuration.
        std::string element;
        std::istringstream iss;

        iss.str(line);

        // Get the timestamp for the transaction.
        iss >> element;
        if (element.empty())
            SC_REPORT_FATAL("StlPlayer", ("Malformed trace file line " + std::to_string(lineCnt) + ".").c_str());
        content.sendingTime = playerClk * static_cast<double>(std::stoull(element));

        // Get the optional burst length and command
        iss >> element;
        if (element.empty())
            SC_REPORT_FATAL("StlPlayer", ("Malformed trace file line " + std::to_string(lineCnt) + ".").c_str());
        if (element.at(0) == '(')
        {
            element.erase(0, 1);
            content.dataLength = std::stoul(element);
            iss >> element;
            if (element.empty())
                SC_REPORT_FATAL("StlPlayer", ("Malformed trace file line " + std::to_string(lineCnt) + ".").c_str());
        }
        else
            content.dataLength = defaultDataLength;

        if (element == "read")
            content.command = TLM_READ_COMMAND;
        else if (element == "write")
            content.command = TLM_WRITE_COMMAND;
        else
            SC_REPORT_FATAL("StlPlayer", ("Malformed trace file line " + std::to_string(lineCnt) + ".").c_str());

        // Get the address.
        iss >> element;
        if (element.empty())
            SC_REPORT_FATAL("StlPlayer", ("Malformed trace file line " + std::to_string(lineCnt) + ".").c_str());
        content.address = std::stoull(element, nullptr, 16);

        // Get the data if necessary.
        if (storageEnabled && content.command == TLM_WRITE_COMMAND)
        {
            // The input trace file must provide the data to be stored into the memory.
            iss >> element;

            // Check if data length in the trace file is correct.
            // We need two characters to represent 1 byte in hexadecimal. Offset for 0x prefix.
            if (element.length() != (content.dataLength * 2 + 2))
                SC_REPORT_FATAL("StlPlayer", ("Malformed trace file line " + std::to_string(lineCnt) + ".").c_str());

            // Set data
            for (unsigned i = 0; i < content.dataLength; i++)
                content.data.emplace_back(static_cast<unsigned char>
                                          (std::stoi(element.substr(i * 2 + 2, 2), nullptr, 16)));
        }
    }
}

std::vector<LineContent>::const_iterator StlPlayer::swapBuffers()
{
    // Wait for parser to finish
    if (parserThread.joinable())
        parserThread.join();

    // Swap buffers
    std::swap(currentBuffer, parseBuffer);

    // Start new parser thread
    parserThread = std::thread(&StlPlayer::parseTraceFile, this);

    return currentBuffer->cbegin();
}

uint64_t StlPlayer::getNumberOfLines() const
{
    return numberOfLines;
}
