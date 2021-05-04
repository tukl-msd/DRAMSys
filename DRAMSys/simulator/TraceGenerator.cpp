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
 */

#include "TraceGenerator.h"

TraceGenerator::TraceGenerator(sc_module_name name,
        unsigned int fCKMhz, TraceSetup *setup)
    : TracePlayer(name, setup), transCounter(0)
{
    if (fCKMhz == 0)
        tCK = Configuration::getInstance().memSpec->tCK;
    else
        tCK = sc_time(1.0 / fCKMhz, SC_US);

    burstlenght = Configuration::getInstance().memSpec->burstLength;
}

void TraceGenerator::nextPayload()
{
    if (transCounter >= 1000) // TODO set limit!
        terminate();

    tlm::tlm_generic_payload *payload = setup->allocatePayload();
    payload->acquire();
    unsigned char *dataElement = new unsigned char[16];
    // TODO: column / burst breite

    payload->set_address(0x0);
    payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload->set_dmi_allowed(false);
    payload->set_byte_enable_length(0);
    payload->set_streaming_width(burstlenght);
    payload->set_data_ptr(dataElement);
    payload->set_data_length(16);
    payload->set_command(tlm::TLM_READ_COMMAND);
    transCounter++;

    // TODO: do not send two requests in the same cycle
    sendToTarget(*payload, tlm::BEGIN_REQ, SC_ZERO_TIME);
    transactionsSent++;
    PRINTDEBUGMESSAGE(name(), "Performing request #" + std::to_string(transactionsSent));
}
