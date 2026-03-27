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

#include <DRAMSys/common/MemoryManager.h>

#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>

class ListInitiator : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<ListInitiator> iSocket;

    ListInitiator(const sc_core::sc_module_name& name, DRAMSys::MemoryManager& memoryManager);

    struct TestTransactionData
    {
        sc_core::sc_time startTime;
        sc_core::sc_time expectedEndTime;

        enum class Command
        {
            Read,
            Write,
        } command;

        uint64_t address;
        uint32_t dataLength;
        uint64_t data;
    };

    void appendTestTransactionList(const std::vector<TestTransactionData>& testTransactionList)
    {
        std::copy(testTransactionList.cbegin(),
                  testTransactionList.cend(),
                  std::back_inserter(this->testTransactionList));
    }

private:
    class TestExtension : public tlm::tlm_extension<TestExtension>
    {
    public:
        TestExtension(TestTransactionData data) : data(std::move(data)) {}

        tlm_extension_base* clone() const { return new TestExtension(data); }

        void copy_from(const tlm_extension_base& ext)
        {
            const TestExtension& cpyFrom = static_cast<const TestExtension&>(ext);
            data = cpyFrom.getTestData();
        }

        TestTransactionData getTestData() const { return data; }

    private:
        TestTransactionData data;
    };

    void process();

    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay);
    void peqCallback(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase);
    void checkTransaction(tlm::tlm_generic_payload& trans);

    std::vector<TestTransactionData> testTransactionList;

    sc_core::sc_event endRequest;
    tlm_utils::peq_with_cb_and_phase<ListInitiator> peq;
    tlm::tlm_generic_payload* requestInProgress = nullptr;
    DRAMSys::MemoryManager& memoryManager;
};
