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
 *    Marco Mörz
 */

#ifndef DRAM_H
#define DRAM_H

#include <DRAMSys/common/DramATRecorder.h>
#include <DRAMSys/configuration/memspec/MemSpec.h>
#include <DRAMSys/controller/Command.h>

#include "DRAMSys/simulation/AddressDecoder.h"
#include "pim-vm-cxx/lib.h"

#ifdef USE_DRAMPOWER
#include <DRAMSys/power/DRAMPowerAdapter.h>
#else
class DRAMPowerAdapter;
#endif

#include <systemc>
#include <tlm>
#include <tlm_utils/multi_passthrough_target_socket.h>

namespace DRAMSys
{

class Dram : sc_core::sc_module
{
public:
    tlm_utils::multi_passthrough_target_socket<Dram> tSocket{"tSocket"};

    Dram(sc_core::sc_module_name const& name,
         unsigned char** backingStore,
         MemSpec const* memspec,
         bool recording,
         std::vector<std::unique_ptr<DramATRecorder>>* dramATRecorders,
         bool power,
         std::vector<DRAMPowerAdapter*>* DRAMPowerMappings,
         AddressDecoder* addressDecoder) :
        sc_core::sc_module(name),
        backingStore(backingStore),
        memspec(memspec),
        recording(recording),
        dramATRecorders(dramATRecorders),
        power(power),
        DRAMPowerMappings(DRAMPowerMappings),
        addressDecoder(addressDecoder),
        pimVM(pim_vm::new_pim_vm(memspec->banksPerChannel))
    {
        tSocket.register_b_transport(this, &Dram::b_transport);
        tSocket.register_transport_dbg(this, &Dram::transport_dbg);

        pim_vm::init_logger();
    }

private:
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans);

    void executeRead(tlm::tlm_generic_payload& trans) const;
    void executeWrite(const tlm::tlm_generic_payload& trans);

    std::vector<uint8_t> read(uint64_t address, std::size_t length);
    void write(uint64_t address, std::vector<uint8_t> const& data);

    unsigned char** backingStore;
    MemSpec const* memspec;

    bool recording;
    std::vector<std::unique_ptr<DramATRecorder>>* dramATRecorders;

    bool power;
    std::vector<DRAMPowerAdapter*>* DRAMPowerMappings;

    AddressDecoder* addressDecoder;

    static constexpr unsigned int PIM_CONFIG_ADDR = 0x40000000;
    static constexpr unsigned int PIM_DATA_ADDR = 0x40004000;
    std::string message;
    rust::Box<pim_vm::PimVM> pimVM;
};

} // namespace DRAMSys

#endif // DRAM_H
