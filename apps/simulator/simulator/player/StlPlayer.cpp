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

static constexpr std::size_t LINE_BUFFER_SIZE = 10000;

StlPlayer::StlPlayer(DRAMSys::Config::TracePlayer const& config,
                     std::filesystem::path const& trace,
                     TraceType traceType,
                     bool storageEnabled) :
    traceType(traceType),
    storageEnabled(storageEnabled),
    playerPeriod(sc_core::sc_time(1.0 / static_cast<double>(config.clkMhz), sc_core::SC_US)),
    defaultDataLength(config.dataLength),
    traceFile(trace)
{
    if (!traceFile.is_open())
        SC_REPORT_FATAL("StlPlayer",
                        (std::string("Could not open trace ") + trace.string()).c_str());

    {
        std::string line;
        while (std::getline(traceFile, line))
        {
            if (line.size() > 1 && line[0] != '#')
                numberOfLines++;
        }
        if (numberOfLines == 0)
            SC_REPORT_FATAL("StlPlayer", (std::string("Empty trace ") + trace.string()).c_str());
        traceFile.clear();
        traceFile.seekg(0);
    }

    parseTraceFile();
    swapBuffers();
    readoutIt = lineBuffers.at(consumeIndex).cbegin();
}

void StlPlayer::incrementLine()
{
    readoutIt++;

    if (readoutIt == lineBuffers.at(consumeIndex).cend())
    {
        swapBuffers();
        readoutIt = lineBuffers.at(consumeIndex).cbegin();

        if (lineBuffers.at(consumeIndex).empty())
        {
            if (parserThread.joinable())
                parserThread.join();
        }
    }
}

std::optional<StlPlayer::LineContent> StlPlayer::currentLine() const
{
    if (readoutIt == lineBuffers.at(consumeIndex).cend())
        return std::nullopt;

    return *readoutIt;
}

Request StlPlayer::nextRequest()
{
    auto currentLineContent = currentLine();

    if (!currentLineContent.has_value())
    {
        // The file is read in completely. Nothing more to do.
         return Request{Request::Command::Stop, 0, 0, {}};
    }

    auto command = currentLineContent->command == LineContent::Command::Read
                       ? Request::Command::Read
                       : Request::Command::Write;
    auto dataLength = currentLineContent->dataLength.has_value()
                          ? currentLineContent->dataLength.value()
                          : defaultDataLength;

    Request request{command, currentLineContent->address, dataLength, currentLineContent->data};

    incrementLine();

    return request;
}

sc_core::sc_time StlPlayer::nextTrigger()
{
    auto currentLineContent = currentLine();
    sc_core::sc_time nextTrigger = sc_core::SC_ZERO_TIME;
    if (currentLineContent.has_value())
    {
        if (traceType == TraceType::Absolute)
        {
            sc_core::sc_time cycleTime = currentLineContent->cycle * playerPeriod;
            bool behindSchedule = sc_core::sc_time_stamp() > cycleTime;
            nextTrigger =
                behindSchedule ? sc_core::SC_ZERO_TIME : cycleTime - sc_core::sc_time_stamp();
        }
        else // if (traceType == TraceType::Relative)
        {
            nextTrigger = currentLineContent->cycle * playerPeriod;
        }
    }

    return nextTrigger;
}

void StlPlayer::parseTraceFile()
{
    unsigned parsedLines = 0;
    auto& parseBuffer = lineBuffers.at(parseIndex);
    parseBuffer.clear();

    while (traceFile && !traceFile.eof() && parsedLines < LINE_BUFFER_SIZE)
    {
        // Get a new line from the input file.
        std::string line;
        std::getline(traceFile, line);
        currentParsedLine++;

        // If the line is empty (\n or \r\n) or starts with '#' (comment) the transaction is
        // ignored.
        if (line.size() <= 1 || line.at(0) == '#')
            continue;

        parsedLines++;
        parseBuffer.emplace_back();
        LineContent& content = parseBuffer.back();

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
            content.cycle = std::stoull(element);

            // Get the optional burst length and command
            iss >> element;

            if (element.at(0) == '(')
            {
                element.erase(0, 1);
                content.dataLength = std::stoul(element);
                iss >> element;
            }

            if (element == "read")
                content.command = LineContent::Command::Read;
            else if (element == "write")
                content.command = LineContent::Command::Write;
            else
                throw std::runtime_error("Unable to parse command");

            // Get the address.
            iss >> element;
            static constexpr unsigned HEX = 16;
            content.address = std::stoull(element, nullptr, HEX);

            // Get the data if necessary.
            if (storageEnabled && content.command == LineContent::Command::Write)
            {
                // The input trace file must provide the data to be stored into the memory.
                iss >> element;

                // We need two characters to represent 1 byte in hexadecimal.
                // Offset for 0x prefix.
                for (std::size_t i = 2; i < element.length(); i += 2)
                {
                    uint8_t byte = std::stoi(element.substr(i, 2), nullptr, HEX);
                    content.data.push_back(byte);
                }
            }
        }
        catch (...)
        {
            SC_REPORT_FATAL(
                "StlPlayer",
                ("Malformed trace file line " + std::to_string(currentParsedLine) + ".").c_str());
        }
    }
}

void StlPlayer::swapBuffers()
{
    // Wait for parser to finish
    if (parserThread.joinable())
        parserThread.join();

    // Swap buffers
    std::swap(parseIndex, consumeIndex);

    // Start new parser thread
    parserThread = std::thread(&StlPlayer::parseTraceFile, this);
}
