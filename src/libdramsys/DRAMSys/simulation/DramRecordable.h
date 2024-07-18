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
 * Authors:
 *    Lukas Steiner
 */

#ifndef DRAMRECORDABLE_H
#define DRAMRECORDABLE_H

#include "DRAMSys/common/TlmRecorder.h"
#include "Dram.h"

#ifdef DRAMPOWER
#include "LibDRAMPower.h"
#endif

#include <systemc>
#include <tlm>

namespace DRAMSys
{

class DramRecordable : public Dram
{
public:
    DramRecordable(const sc_core::sc_module_name& name,
                   const SimConfig& simConfig,
                   const MemSpec& memSpec,
                   TlmRecorder& tlmRecorder);
    SC_HAS_PROCESS(DramRecordable);

    void reportPower() override;

private:
    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) override;

    TlmRecorder& tlmRecorder;

    sc_core::sc_time powerWindowSize;

    // When working with floats, we have to decide ourselves what is an
    // acceptable definition for "equal". Here the number is compared with a
    // suitable error margin (0.00001).
    static bool isEqual(double a, double b, const double epsilon = 1e-05)
    {
        return std::fabs(a - b) < epsilon;
    }

#ifdef DRAMPOWER
    // This Thread is only triggered when Power Simulation is enabled.
    // It estimates the current average power which will be stored in the trace database for
    // visualization purposes.
    void powerWindow();
#endif
};

} // namespace DRAMSys

#endif // DRAMRECORDABLE_H
