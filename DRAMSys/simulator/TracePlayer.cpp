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
 *    Robert Gernhardt
 *    Matthias Jung
 *    Éder F. Zulian
 *    Felipe S. Prado
 */

#include "TracePlayer.h"
#include "TraceSetup.h"

using namespace tlm;

TracePlayer::TracePlayer(sc_module_name name, TraceSetup *setup) :
    sc_module(name),
    payloadEventQueue(this, &TracePlayer::peqCallback),
    setup(setup)
{
    SC_METHOD(nextPayload);
    iSocket.register_nb_transport_bw(this, &TracePlayer::nb_transport_bw);

    if (Configuration::getInstance().storeMode == Configuration::StoreMode::NoStorage)
        storageEnabled = false;
    else
        storageEnabled = true;
}

void TracePlayer::terminate()
{
    std::cout << sc_time_stamp() << " " << this->name() << " terminated " << std::endl;
    setup->tracePlayerTerminates();
}

tlm_sync_enum TracePlayer::nb_transport_bw(tlm_generic_payload &payload,
                                           tlm_phase &phase, sc_time &bwDelay)
{
    payloadEventQueue.notify(payload, phase, bwDelay);
    return TLM_ACCEPTED;
}

void TracePlayer::peqCallback(tlm_generic_payload &payload,
                              const tlm_phase &phase)
{
    if (phase == END_REQ)
        nextPayload();
    else if (phase == BEGIN_RESP)
    {
        payload.release();
        sendToTarget(payload, END_RESP, SC_ZERO_TIME);
        if (Configuration::getInstance().simulationProgressBar)
            setup->transactionFinished();

        transactionsReceived++;

        // If all answers were received:
        if (finished == true && numberOfTransactions == transactionsReceived)
            terminate();
    }
    else
    {
        SC_REPORT_FATAL("TracePlayer", "PEQ was triggered with unknown phase");
    }
}

void TracePlayer::sendToTarget(tlm_generic_payload &payload, const tlm_phase &phase, const sc_time &delay)
{
    tlm_phase TPhase = phase;
    sc_time TDelay = delay;
    iSocket->nb_transport_fw(payload, TPhase, TDelay);
}

uint64_t TracePlayer::getNumberOfLines(std::string pathToTrace)
{
    std::ifstream file;
    file.open(pathToTrace);

    if (file.is_open())
    {
        uint64_t lineCount = 0;
        std::string line;
        while (std::getline(file, line))
        {
            if (line.size() > 1 && line[0] != '#')
                lineCount++;
        }
        file.close();

        if (lineCount == 0)
            SC_REPORT_FATAL("TracePlayer", "Trace file is empty");
        return lineCount;
    }
    else
    {
        SC_REPORT_FATAL("TracePlayer", "Unable to open trace file");
        return 0;
    }
}
