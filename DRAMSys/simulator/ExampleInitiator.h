/*
 * Copyright (c) 2016, Technische Universität Kaiserslautern
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
 *    Eder F. Zulian
 */

#ifndef EXAMPLEINITIATOR_H
#define EXAMPLEINITIATOR_H

#include <cstdlib>
#include <iostream>

#include <systemc>
#include "MemoryManager.h"
#include "common/dramExtensions.h"
#include "TracePlayer.h"

struct ExampleInitiator : sc_core::sc_module
{
    // TLM-2 socket, defaults to 32-bits wide, base protocol
    tlm_utils::simple_initiator_socket<ExampleInitiator> socket;

    SC_CTOR(ExampleInitiator)
        : socket("socket"),
          request_in_progress(nullptr),
          m_peq(this, &ExampleInitiator::peq_cb)
    {
        socket.register_nb_transport_bw(this, &ExampleInitiator::nb_transport_bw);
        SC_THREAD(thread_process);
    }

    void thread_process()
    {
        tlm::tlm_generic_payload *trans;
        tlm::tlm_phase phase;
        sc_core::sc_time delay;

        dump_mem();
        init_mem();
        dump_mem();

        for (unsigned char &i : data)
            i = 0x55;

        // Generate 2 write transactions
        for (int i = 0; i < 2; i++) {
            int adr = i * 64;

            tlm::tlm_command cmd = tlm::TLM_WRITE_COMMAND;

            // Grab a new transaction from the memory manager
            trans = m_mm.allocate();
            trans->acquire();

            trans->set_command(cmd);
            trans->set_address(adr);
            trans->set_data_ptr(reinterpret_cast<unsigned char *>(&data[0]));
            trans->set_data_length(64);
            trans->set_streaming_width(4);
            trans->set_byte_enable_ptr(nullptr);
            trans->set_dmi_allowed(false);
            trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            // ExampleInitiator must honor BEGIN_REQ/END_REQ exclusion rule
            if (request_in_progress)
                wait(end_request_event);
            request_in_progress = trans;
            phase = tlm::BEGIN_REQ;

            // Timing annotation models processing time of initiator prior to call
            delay = sc_core::sc_time(100000, sc_core::SC_PS);

            std::cout << "Address " << std::hex << adr << " new, cmd=" << (cmd ? "write" : "read")
                 << ", data=" << std::hex << data[0] << " at time " << sc_core::sc_time_stamp()
                 << " in " << name() << std::endl;

            GenerationExtension *genExtension = new GenerationExtension(sc_core::sc_time_stamp());
            trans->set_auto_extension(genExtension);


            // Non-blocking transport call on the forward path
            tlm::tlm_sync_enum status;
            status = socket->nb_transport_fw( *trans, phase, delay );

            // Check value returned from nb_transport_fw
            if (status == tlm::TLM_UPDATED) {
                // The timing annotation must be honored
                m_peq.notify( *trans, phase, delay );
            } else if (status == tlm::TLM_COMPLETED) {
                // The completion of the transaction necessarily ends the BEGIN_REQ phase
                request_in_progress = nullptr;

                // The target has terminated the transaction
                check_transaction( *trans );

                // Allow the memory manager to free the transaction object
                trans->release();
            }

            sc_core::wait(sc_core::sc_time(500, sc_core::SC_NS));

            dump_mem();
        }

        sc_core::wait(sc_core::sc_time(500, sc_core::SC_NS));
        sc_core::sc_stop();
    }

    static void init_mem()
    {
        unsigned char buffer[64];
        for (unsigned char &i : buffer)
            i = 0xff;

        for (int addr = 0; addr < 128; addr += 64) {
            tlm::tlm_generic_payload trans;
            trans.set_command( tlm::TLM_WRITE_COMMAND );
            trans.set_address( addr );
            trans.set_data_ptr( buffer );
            trans.set_data_length( 64 );

            socket->transport_dbg( trans );
        }
    }

    static void dump_mem()
    {
        for (int addr = 0; addr < 128; addr += 64) {
            unsigned char buffer[64];
            tlm::tlm_generic_payload trans;
            trans.set_command( tlm::TLM_READ_COMMAND );
            trans.set_address( addr );
            trans.set_data_ptr( buffer );
            trans.set_data_length( 64 );

            socket->transport_dbg( trans );

            std::cout << "\nMemory dump\n";
            for (int i = 0; i < 64; i++)
                std::cout << "mem[" << addr + i << "] = " << std::hex << (int)buffer[i] << std::endl;
        }
    }

    // TLM-2 backward non-blocking transport method

    virtual tlm::tlm_sync_enum nb_transport_bw( tlm::tlm_generic_payload &trans,
                                                tlm::tlm_phase &phase, sc_core::sc_time &delay )
    {
        m_peq.notify( trans, phase, delay );
        return tlm::TLM_ACCEPTED;
    }

    // Payload event queue callback to handle transactions from target
    // Transaction could have arrived through return path or backward path

    void peq_cb(tlm::tlm_generic_payload &trans, const tlm::tlm_phase &phase)
    {
        if (phase == tlm::END_REQ || (&trans == request_in_progress
                                      && phase == tlm::BEGIN_RESP)) {
            // The end of the BEGIN_REQ phase
            request_in_progress = nullptr;
            end_request_event.notify();
        } else if (phase == tlm::BEGIN_REQ || phase == tlm::END_RESP)
            SC_REPORT_FATAL("TLM-2", "Illegal transaction phase received by initiator");

        if (phase == tlm::BEGIN_RESP) {
            check_transaction( trans );

            // Send final phase transition to target
            tlm::tlm_phase fw_phase = tlm::END_RESP;
            sc_core::sc_time delay = sc_core::sc_time(60000, sc_core::SC_PS);
            socket->nb_transport_fw( trans, fw_phase, delay );
            // Ignore return value

            // Allow the memory manager to free the transaction object
            trans.release();
        }
    }

    // Called on receiving BEGIN_RESP or TLM_COMPLETED
    void check_transaction(tlm::tlm_generic_payload &trans)
    {
        if ( trans.is_response_error() ) {
            char txt[100];
            sprintf(txt, "Transaction returned with error, response status = %s",
                    trans.get_response_string().c_str());
            SC_REPORT_ERROR("TLM-2", txt);
        }

        tlm::tlm_command cmd = trans.get_command();
        uint64_t    adr = trans.get_address();
        int             *ptr = reinterpret_cast<int *>( trans.get_data_ptr() );

        std::cout << std::hex << adr << " check, cmd=" << (cmd ? "write" : "read")
             << ", data=" << std::hex << *ptr << " at time " << sc_core::sc_time_stamp()
             << " in " << sc_core::name() << std::endl;

        if (cmd == tlm::TLM_READ_COMMAND)
            assert( *ptr == -int(adr) );
    }

    MemoryManager m_mm;
    unsigned char data[64];
    tlm::tlm_generic_payload *request_in_progress;
    sc_core::sc_event end_request_event;
    tlm_utils::peq_with_cb_and_phase<ExampleInitiator> m_peq;
};

#endif // EXAMPLEINITIATOR_H
