/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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
 * Author: Lukas Steiner
 */

#ifndef CONTROLLERRECORDABLE_H
#define CONTROLLERRECORDABLE_H

#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/controller/Controller.h"
#include "DRAMSys/simulation/SimConfig.h"

#include <systemc>
#include <tlm>

namespace DRAMSys
{

class ControllerRecordable final : public Controller
{
public:
    ControllerRecordable(const sc_core::sc_module_name& name,
                         const McConfig& config,
                         const SimConfig& simConfig,
                         const MemSpec& memSpec,
                         const AddressDecoder& addressDecoder,
                         TlmRecorder& tlmRecorder);

protected:
    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) override;
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) override;

    void sendToFrontend(tlm::tlm_generic_payload& payload,
                        tlm::tlm_phase& phase,
                        sc_core::sc_time& delay) override;

    void controllerMethod() override;

private:
    TlmRecorder& tlmRecorder;

    sc_core::sc_event windowEvent;
    const sc_core::sc_time windowSizeTime;
    sc_core::sc_time nextWindowEventTime;
    std::vector<sc_core::sc_time> slidingAverageBufferDepth;
    std::vector<double> windowAverageBufferDepth;
    sc_core::sc_time lastTimeCalled = sc_core::SC_ZERO_TIME;

    uint64_t lastNumberOfBeatsServed = 0;
    const sc_core::sc_time activeTimeMultiplier;
    const bool enableWindowing;
};

} // namespace DRAMSys

#endif // CONTROLLERRECORDABLE_H
