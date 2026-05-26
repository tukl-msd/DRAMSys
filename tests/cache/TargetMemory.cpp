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

#include "TargetMemory.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>

DECLARE_EXTENDED_PHASE(INTERNAL);

TargetMemory::TargetMemory(const sc_core::sc_module_name& name,
                           sc_core::sc_time acceptDelay,
                           sc_core::sc_time memoryLatency,
                           std::size_t bufferSize) :
    sc_core::sc_module(name),
    tSocket("tSocket"),
    bufferSize(bufferSize),
    acceptDelay(acceptDelay),
    memoryLatency(memoryLatency),
    peq(this, &TargetMemory::peqCallback)
{
    tSocket.register_nb_transport_fw(this, &TargetMemory::nb_transport_fw);

    memory.resize(SIZE);
    std::fill(memory.begin(), memory.end(), 0);
}

tlm::tlm_sync_enum TargetMemory::nb_transport_fw(tlm::tlm_generic_payload& trans,
                                                 tlm::tlm_phase& phase,
                                                 sc_core::sc_time& delay)
{
    peq.notify(trans, phase, delay);
    return tlm::TLM_ACCEPTED;
}

void TargetMemory::printBuffer(int max, int n)
{
    std::cout << "\033[1;35m(" << name() << ")@" << std::setfill(' ') << std::setw(12)
              << sc_core::sc_time_stamp() << " Target Buffer: "
              << "[";
    for (int i = 0; i < n; i++)
    {
        std::cout << "█";
    }
    for (int i = 0; i < max - n; i++)
    {
        std::cout << " ";
    }
    std::cout << "]"
              << " (Max:" << max << ") "
              << "\033[0m" << std::endl;
    std::cout.flush();
}

void TargetMemory::peqCallback(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase)
{
    sc_core::sc_time delay;

    if (phase == tlm::BEGIN_REQ)
    {
        trans.acquire();

        if (currentTransactions < bufferSize)
        {
            sendEndRequest(trans);
        }
        else
        {
            endRequestPending = &trans;
        }
    }
    else if (phase == tlm::END_RESP)
    {
        if (!responseInProgress)
        {
            SC_REPORT_FATAL(name(), "Illegal transaction phase END_RESP received by target");
        }

        currentTransactions--;
        printBuffer(bufferSize, currentTransactions);

        responseInProgress = false;
        if (!responseQueue.empty())
        {
            tlm::tlm_generic_payload* next = responseQueue.front();
            responseQueue.pop();
            sendResponse(*next);
        }

        if (endRequestPending != nullptr)
        {
            sendEndRequest(*endRequestPending);
            endRequestPending = nullptr;
        }
    }
    else if (phase == INTERNAL)
    {
        executeTransaction(trans);

        if (responseInProgress)
        {
            responseQueue.push(&trans);
        }
        else
        {
            sendResponse(trans);
        }
    }
    else // tlm::END_REQ or tlm::BEGIN_RESP
    {
        SC_REPORT_FATAL(name(), "Illegal transaction phase received");
    }
}

void TargetMemory::sendEndRequest(tlm::tlm_generic_payload& trans)
{
    tlm::tlm_phase bw_phase;
    sc_core::sc_time delay;

    // Queue the acceptance and the response with the appropriate latency
    bw_phase = tlm::END_REQ;
    delay = acceptDelay;

    tSocket->nb_transport_bw(trans, bw_phase, delay);

    // Queue internal event to mark beginning of response
    delay = delay + memoryLatency; // MEMORY Latency
    peq.notify(trans, INTERNAL, delay);

    currentTransactions++;
    printBuffer(bufferSize, currentTransactions);
}

void TargetMemory::sendResponse(tlm::tlm_generic_payload& trans)
{
    sc_assert(responseInProgress == false);

    responseInProgress = true;
    tlm::tlm_phase bw_phase = tlm::BEGIN_RESP;
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    tlm::tlm_sync_enum status = tSocket->nb_transport_bw(trans, bw_phase, delay);

    if (status == tlm::TLM_UPDATED)
    {
        peq.notify(trans, bw_phase, delay);
    }
    else if (status == tlm::TLM_COMPLETED)
    {
        SC_REPORT_FATAL(name(), "This transition is deprecated since TLM2.0.1");
    }

    trans.release();
}

void TargetMemory::executeTransaction(tlm::tlm_generic_payload& trans)
{
    tlm::tlm_command command = trans.get_command();
    sc_dt::uint64 address = trans.get_address();
    unsigned char* data_ptr = trans.get_data_ptr();
    unsigned int data_length = trans.get_data_length();
    unsigned char* byte_enable_ptr = trans.get_byte_enable_ptr();
    unsigned int streaming_width = trans.get_streaming_width();

    if (address >= SIZE - 64)
    {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    if (byte_enable_ptr != nullptr)
    {
        trans.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
        return;
    }
    if (data_length > 64 || streaming_width < data_length)
    {
        trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return;
    }

    if (command == tlm::TLM_READ_COMMAND)
    {
        std::memcpy(trans.get_data_ptr(), &memory[trans.get_address()], trans.get_data_length());
    }
    else if (command == tlm::TLM_WRITE_COMMAND)
    {
        memcpy(&memory[trans.get_address()], trans.get_data_ptr(), trans.get_data_length());
    }

    std::cout << "\033[1;32m(" << name() << ")@" << std::setfill(' ') << std::setw(12)
              << sc_core::sc_time_stamp() << ": " << std::setw(12)
              << (command == tlm::TLM_WRITE_COMMAND ? "Exec. Write " : "Exec. Read ")
              << "Addr = " << std::setfill('0') << std::setw(8) << std::dec << address << " Data = "
              << "0x" << std::setfill('0') << std::setw(8) << std::hex
              << *reinterpret_cast<int*>(data_ptr) << "\033[0m" << std::endl;

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}
