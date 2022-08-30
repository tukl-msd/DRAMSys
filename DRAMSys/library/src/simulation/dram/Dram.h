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
 *    Peter Ehses
 *    Eder F. Zulian
 *    Felipe S. Prado
 */

#ifndef DRAM_H
#define DRAM_H

#include <memory>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include "../../configuration/Configuration.h"
#include "../../configuration/memspec/MemSpec.h"

class libDRAMPower;

class Dram : public sc_core::sc_module
{
protected:
    Dram(const sc_core::sc_module_name &name, const Configuration& config);
    SC_HAS_PROCESS(Dram);

    const MemSpec& memSpec;

    // Data Storage:
    const Configuration::StoreMode storeMode;
    const bool powerAnalysis;
    unsigned char *memory;
    const bool useMalloc;

#ifdef DRAMPOWER
    std::unique_ptr<libDRAMPower> DRAMPower;
#endif

    virtual tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload &payload,
                                               tlm::tlm_phase &phase, sc_core::sc_time &delay);

    virtual unsigned int transport_dbg(tlm::tlm_generic_payload &trans);

public:
    tlm_utils::simple_target_socket<Dram> tSocket;

    virtual void reportPower();
    ~Dram() override;
};

#endif // DRAM_H

