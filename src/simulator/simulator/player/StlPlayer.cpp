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
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 *    Éder F. Zulian
 *    Felipe S. Prado
 *    Derek Christ
 */

#include "StlPlayer.h"

#include <sstream>

StlPlayer::StlPlayer(std::string_view tracePath,
                     unsigned int clkMhz,
                     unsigned int defaultDataLength,
                     TraceType traceType,
                     bool storageEnabled) :
    traceType(traceType),
    storageEnabled(storageEnabled),
    playerPeriod(sc_core::sc_time(1.0 / static_cast<double>(clkMhz), sc_core::SC_US)),
    defaultDataLength(defaultDataLength),
    traceFile(tracePath.data()),
    lineBuffers(
        {std::make_shared<std::vector<Request>>(), std::make_shared<std::vector<Request>>()}),
    parseBuffer(lineBuffers.at(1)),
    readoutBuffer(lineBuffers.at(0))
{
    readoutBuffer->reserve(LINE_BUFFER_SIZE);
    parseBuffer->reserve(LINE_BUFFER_SIZE);

    if (!traceFile.is_open())
        SC_REPORT_FATAL("StlPlayer",
                        (std::string("Could not open trace ") + tracePath.data()).c_str());

    {
        std::string line;
        while (std::getline(traceFile, line))
        {
            if (line.size() > 1 && line[0] != '#')
                numberOfLines++;
        }
        if (numberOfLines == 0)
            SC_REPORT_FATAL("StlPlayer", (std::string("Empty trace ") + tracePath.data()).c_str());
        traceFile.clear();
        traceFile.seekg(0);
    }

    parseTraceFile();
    readoutIt = readoutBuffer->cend();
}

Request StlPlayer::nextRequest()
{
    if (readoutIt == readoutBuffer->cend())
    {
        readoutIt = swapBuffers();
        if (readoutIt == readoutBuffer->cend())
        {
            if (parserThread.joinable())
                parserThread.join();

            // The file is read in completely. Nothing more to do.
            return Request{Request::Command::Stop};
        }
    }

    sc_core::sc_time delay;
    if (traceType == TraceType::Absolute)
    {
        bool behindSchedule = sc_core::sc_time_stamp() > readoutIt->delay;
        delay =
            behindSchedule ? sc_core::SC_ZERO_TIME : readoutIt->delay - sc_core::sc_time_stamp();
    }
    else // if (traceType == TraceType::Relative)
    {
        delay = readoutIt->delay;
    }

    Request request(*readoutIt);
    request.delay = delay;

    readoutIt++;
    return request;
}

void StlPlayer::parseTraceFile()
{
    unsigned parsedLines = 0;
    parseBuffer->clear();

    while (traceFile && !traceFile.eof() && parsedLines < LINE_BUFFER_SIZE)
    {
        // Get a new line from the input file.
        std::string line;
        std::getline(traceFile, line);
        currentLine++;

        // If the line is empty (\n or \r\n) or starts with '#' (comment) the transaction is
        // ignored.
        if (line.size() <= 1 || line.at(0) == '#')
            continue;

        parsedLines++;
        parseBuffer->emplace_back();
        Request& content = parseBuffer->back();

        // Trace files MUST provide timestamp, command and address for every
        // transaction. The data information depends on the storage mode
        // configuration.
        std::string element;
        std::istringstream iss;

        iss.str(line);

        try
        {
            // Get the timestamp for the transaction.
            iss >> element;
            if (element.empty())
                SC_REPORT_FATAL(
                    "StlPlayer",
                    ("Malformed trace file line " + std::to_string(currentLine) + ".").c_str());

            content.delay = playerPeriod * static_cast<double>(std::stoull(element));

            // Get the optional burst length and command
            iss >> element;
            if (element.empty())
                SC_REPORT_FATAL(
                    "StlPlayer",
                    ("Malformed trace file line " + std::to_string(currentLine) + ".").c_str());

            if (element.at(0) == '(')
            {
                element.erase(0, 1);
                content.length = std::stoul(element);
                iss >> element;
                if (element.empty())
                    SC_REPORT_FATAL(
                        "StlPlayer",
                        ("Malformed trace file line " + std::to_string(currentLine) + ".").c_str());
            }
            else
                content.length = defaultDataLength;

            if (element == "read")
                content.command = Request::Command::Read;
            else if (element == "write")
                content.command = Request::Command::Write;
            else
                SC_REPORT_FATAL(
                    "StlPlayer",
                    ("Malformed trace file line " + std::to_string(currentLine) + ".").c_str());

            // Get the address.
            iss >> element;
            if (element.empty())
                SC_REPORT_FATAL(
                    "StlPlayer",
                    ("Malformed trace file line " + std::to_string(currentLine) + ".").c_str());
            content.address = std::stoull(element, nullptr, 16);

            // Get the data if necessary.
            if (storageEnabled && content.command == Request::Command::Write)
            {
                // The input trace file must provide the data to be stored into the memory.
                iss >> element;

                // Check if data length in the trace file is correct.
                // We need two characters to represent 1 byte in hexadecimal. Offset for 0x
                // prefix.
                if (element.length() != (content.length * 2 + 2))
                    SC_REPORT_FATAL(
                        "StlPlayer",
                        ("Malformed trace file line " + std::to_string(currentLine) + ".").c_str());

                // Set data
                for (unsigned i = 0; i < content.length; i++)
                    content.data.emplace_back(static_cast<unsigned char>(
                        std::stoi(element.substr(i * 2 + 2, 2), nullptr, 16)));
            }
        }
        catch (...)
        {
            SC_REPORT_FATAL(
                "StlPlayer",
                ("Malformed trace file line " + std::to_string(currentLine) + ".").c_str());
        }
    }
}

std::vector<Request>::const_iterator StlPlayer::swapBuffers()
{
    // Wait for parser to finish
    if (parserThread.joinable())
        parserThread.join();

    // Swap buffers
    std::swap(readoutBuffer, parseBuffer);

    // Start new parser thread
    parserThread = std::thread(&StlPlayer::parseTraceFile, this);

    return readoutBuffer->cbegin();
}
