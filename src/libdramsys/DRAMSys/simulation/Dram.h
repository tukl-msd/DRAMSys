/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#ifndef DRAM_H
#define DRAM_H

#include "DRAMSys/common/Deserialize.h"
#include "DRAMSys/common/Serialize.h"
#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/simulation/SimConfig.h"

#include <memory>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

class libDRAMPower;

namespace DRAMSys
{

class Dram : public sc_core::sc_module, public Serialize, public Deserialize
{
protected:
    const MemSpec& memSpec;

    // Data Storage:
    const Config::StoreModeType storeMode;
    const bool powerAnalysis;
    unsigned char* memory;
    const uint64_t channelSize;
    const bool useMalloc;

#ifdef DRAMPOWER
    std::unique_ptr<libDRAMPower> DRAMPower;
#endif

    virtual tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& delay);
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    virtual unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

    void executeRead(tlm::tlm_generic_payload& trans) const;
    void executeWrite(const tlm::tlm_generic_payload& trans);

public:
    Dram(const sc_core::sc_module_name& name, const SimConfig& simConfig, const MemSpec& memSpec);
    SC_HAS_PROCESS(Dram);

    Dram(const Dram&) = delete;
    Dram(Dram&&) = delete;
    Dram& operator=(const Dram&) = delete;
    Dram& operator=(Dram&&) = delete;
    ~Dram() override;

    static constexpr std::string_view BLOCKING_WARNING =
        "Use the blocking mode of DRAMSys with caution! "
        "The simulated timings do not reflect the real system!";

    tlm_utils::simple_target_socket<Dram> tSocket{"tSocket"};

    virtual void reportPower();

    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;
};

} // namespace DRAMSys

#endif // DRAM_H
