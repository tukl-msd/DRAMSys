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

#include "ListInitiator.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <systemc>
#include <tlm>
#include <utility>

ListInitiator::ListInitiator(const sc_core::sc_module_name& name, DRAMSys::MemoryManager& memoryManager) :
    sc_core::sc_module(name),
    iSocket("iSocket"),
    peq(this, &ListInitiator::peqCallback),
    memoryManager(memoryManager)
{
    iSocket.register_nb_transport_bw(this, &ListInitiator::nb_transport_bw);
    SC_THREAD(process);
}

void ListInitiator::process()
{
    for (auto& testTransactionData : testTransactionList)
    {
        wait(testTransactionData.startTime - sc_core::sc_time_stamp());

        tlm::tlm_command command =
            testTransactionData.command == TestTransactionData::Command::Write
                ? tlm::TLM_WRITE_COMMAND
                : tlm::TLM_READ_COMMAND;

        auto* trans = memoryManager.allocate(testTransactionData.dataLength);
        trans->acquire();

        TestExtension* ext = new TestExtension(testTransactionData);
        trans->set_auto_extension(ext);

        trans->set_command(command);
        trans->set_address(testTransactionData.address);
        trans->set_data_length(testTransactionData.dataLength);
        trans->set_streaming_width(testTransactionData.dataLength);
        trans->set_byte_enable_ptr(nullptr);
        trans->set_dmi_allowed(false);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        if (trans->is_write())
            std::memcpy(
                trans->get_data_ptr(), &testTransactionData.data, testTransactionData.dataLength);

        if (requestInProgress != nullptr)
        {
            wait(endRequest);
        }

        requestInProgress = trans;
        tlm::tlm_phase phase = tlm::BEGIN_REQ;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

        std::cout << "\033[1;31m(" << name() << ")@" << std::setfill(' ') << std::setw(12)
                  << sc_core::sc_time_stamp() << ": " << std::setw(12)
                  << (command == tlm::TLM_WRITE_COMMAND ? "Write to " : "Read from ")
                  << "Addr = " << std::setfill('0') << std::setw(8) << std::dec
                  << testTransactionData.address << " Data = "
                  << "0x" << std::setfill('0') << std::setw(8) << std::hex
                  << testTransactionData.data << "(nb_transport) \033[0m" << std::endl;

        tlm::tlm_sync_enum status = iSocket->nb_transport_fw(*trans, phase, delay);

        if (status == tlm::TLM_UPDATED)
        {
            peq.notify(*trans, phase, delay);
        }
        else if (status == tlm::TLM_COMPLETED)
        {
            requestInProgress = nullptr;
            checkTransaction(*trans);
            trans->release();
        }
    }
}

tlm::tlm_sync_enum ListInitiator::nb_transport_bw(tlm::tlm_generic_payload& trans,
                                                  tlm::tlm_phase& phase,
                                                  sc_core::sc_time& delay)
{
    peq.notify(trans, phase, delay);
    return tlm::TLM_ACCEPTED;
}

void ListInitiator::peqCallback(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase)
{
    if (phase == tlm::END_REQ || (&trans == requestInProgress && phase == tlm::BEGIN_RESP))
    {
        requestInProgress = nullptr;
        endRequest.notify();
    }
    else if (phase == tlm::BEGIN_REQ || phase == tlm::END_RESP)
    {
        SC_REPORT_FATAL(name(), "Illegal transaction phase received");
    }

    if (phase == tlm::BEGIN_RESP)
    {
        checkTransaction(trans);

        tlm::tlm_phase fw_phase = tlm::END_RESP;
        sc_core::sc_time delay = sc_core::sc_time(1, sc_core::SC_NS);
        iSocket->nb_transport_fw(trans, fw_phase, delay);

        trans.release();
    }
}

void ListInitiator::checkTransaction(tlm::tlm_generic_payload& trans)
{
    if (trans.is_response_error())
    {
        SC_REPORT_ERROR(name(), "Transaction returned with error!");
    }

    tlm::tlm_command command = trans.get_command();

    uint64_t transData{};

    if (trans.get_data_length() == 1)
        transData = *reinterpret_cast<uint8_t*>(trans.get_data_ptr());
    else if (trans.get_data_length() == 2)
        transData = *reinterpret_cast<uint16_t*>(trans.get_data_ptr());
    else if (trans.get_data_length() == 4)
        transData = *reinterpret_cast<uint32_t*>(trans.get_data_ptr());
    else if (trans.get_data_length() == 8)
        transData = *reinterpret_cast<uint64_t*>(trans.get_data_ptr());

    TestExtension* ext = nullptr;
    trans.get_extension(ext);
    TestTransactionData data = ext->getTestData();

    std::cout << "\033[1;31m(" << name() << ")@" << std::setfill(' ') << std::setw(12)
              << sc_core::sc_time_stamp() << ": " << std::setw(12)
              << (command == tlm::TLM_WRITE_COMMAND ? "Check Write " : "Check Read ")
              << "Addr = " << std::setfill('0') << std::setw(8) << std::dec << data.address
              << " Data = "
              << "0x" << std::setfill('0') << std::setw(8) << std::hex << transData << "\033[0m"
              << std::endl;

    if (data.expectedEndTime != sc_core::sc_time_stamp())
    {
        SC_REPORT_FATAL(name(), "NOT EXPECTED TIME");
    }

    if (trans.is_read() && data.data != transData)
    {
        SC_REPORT_FATAL(name(), "NOT EXPECTED DATA");
    }
}
