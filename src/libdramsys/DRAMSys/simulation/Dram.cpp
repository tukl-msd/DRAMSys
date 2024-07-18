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

#include "Dram.h"

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/config/SimConfig.h"

#ifdef DRAMPOWER
#include "LibDRAMPower.h"
#endif

#include <cassert>
#include <cstdint>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

using namespace sc_core;
using namespace tlm;

#ifdef DRAMPOWER
using namespace DRAMPower;
#endif

namespace DRAMSys
{

Dram::Dram(const sc_module_name& name, const SimConfig& simConfig, const MemSpec& memSpec) :
    sc_module(name),
    memSpec(memSpec),
    storeMode(simConfig.storeMode),
    powerAnalysis(simConfig.powerAnalysis),
    channelSize(memSpec.getSimMemSizeInBytes() / memSpec.numberOfChannels),
    useMalloc(simConfig.useMalloc)
{
    if (storeMode == Config::StoreModeType::Store)
    {
        if (useMalloc)
        {
            memory = (unsigned char*)malloc(channelSize);
            if (memory == nullptr)
                SC_REPORT_FATAL(this->name(), "Memory allocation failed");
        }
        else
        {
// allocate and model storage of one DRAM channel using memory map
#ifdef _WIN32
            SC_REPORT_FATAL("Dram", "On Windows Storage is not yet supported");
            memory = 0; // FIXME
#else
            memory = (unsigned char*)mmap(nullptr,
                                          channelSize,
                                          PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANON | MAP_NORESERVE,
                                          -1,
                                          0);
#endif
        }
    }

    tSocket.register_nb_transport_fw(this, &Dram::nb_transport_fw);
    tSocket.register_b_transport(this, &Dram::b_transport);
    tSocket.register_transport_dbg(this, &Dram::transport_dbg);

#ifdef DRAMPOWER
    if (powerAnalysis)
    {
        DRAMPower = std::make_unique<libDRAMPower>(memSpec.toDramPowerMemSpec(), false);
    }
#endif
}

Dram::~Dram()
{
    if (useMalloc)
        free(memory);
}

void Dram::reportPower()
{
#ifdef DRAMPOWER
    DRAMPower->calcEnergy();

    // Print the final total energy and the average power for
    // the simulation:
    std::cout << name() << std::string("  Total Energy:   ") << std::fixed << std::setprecision(2)
              << DRAMPower->getEnergy().total_energy * memSpec.devicesPerRank << std::string(" pJ")
              << std::endl;

    std::cout << name() << std::string("  Average Power:  ") << std::fixed << std::setprecision(2)
              << DRAMPower->getPower().average_power * memSpec.devicesPerRank << std::string(" mW")
              << std::endl;
#endif
}

tlm_sync_enum Dram::nb_transport_fw(tlm_generic_payload& trans, tlm_phase& phase, sc_time& delay)
{
    assert(phase >= BEGIN_RD && phase <= END_SREF);

#ifdef DRAMPOWER
    if (powerAnalysis)
    {
        int bank = static_cast<int>(ControllerExtension::getBank(trans));
        int64_t cycle = std::lround((sc_time_stamp() + delay) / memSpec.tCK);
        DRAMPower->doCommand(phaseToDRAMPowerCommand(phase), bank, cycle);
    }
#endif

    if (storeMode == Config::StoreModeType::Store)
    {
        if (phase == BEGIN_RD || phase == BEGIN_RDA)
        {
            executeRead(trans);
        }
        else if (phase == BEGIN_WR || phase == BEGIN_WRA || phase == BEGIN_MWR ||
                 phase == BEGIN_MWRA)
        {
            executeWrite(trans);
        }
    }

    return TLM_ACCEPTED;
}

unsigned int Dram::transport_dbg(tlm_generic_payload& trans)
{
    PRINTDEBUGMESSAGE(name(), "transport_dgb");

    // TODO: This part is not tested yet, neither with traceplayers nor with GEM5 coupling
    if (storeMode == Config::StoreModeType::NoStorage)
    {
        SC_REPORT_FATAL("DRAM", "Debug Transport is used in combination with NoStorage");
    }
    else if (storeMode == Config::StoreModeType::Store)
    {
        if (trans.is_read())
        {
            executeRead(trans);
        }
        else if (trans.is_write())
        {
            executeWrite(trans);
        }

        return trans.get_data_length();
    }

    return 0;
}

void Dram::b_transport(tlm_generic_payload& trans, [[maybe_unused]] sc_time& delay)
{
    static bool printedWarning = false;

    if (!printedWarning)
    {
        SC_REPORT_WARNING("DRAM", BLOCKING_WARNING.data());
        printedWarning = true;
    }

    if (storeMode == Config::StoreModeType::Store)
    {
        if (trans.is_read())
        {
            executeRead(trans);
        }
        else if (trans.is_write())
        {
            executeWrite(trans);
        }
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

void Dram::executeRead(tlm::tlm_generic_payload& trans) const
{
    unsigned char* phyAddr = memory + trans.get_address();

    if (trans.get_byte_enable_ptr() == nullptr)
    {
        memcpy(trans.get_data_ptr(), phyAddr, trans.get_data_length());
    }
    else
    {
        for (std::size_t i = 0; i < trans.get_data_length(); i++)
        {
            std::size_t byteEnableIndex = i % trans.get_byte_enable_length();
            if (trans.get_byte_enable_ptr()[byteEnableIndex] == TLM_BYTE_ENABLED)
            {
                trans.get_data_ptr()[i] = phyAddr[i];
            }
        }
    }
}

void Dram::executeWrite(const tlm::tlm_generic_payload& trans)
{
    unsigned char* phyAddr = memory + trans.get_address();

    if (trans.get_byte_enable_ptr() == nullptr)
    {
        memcpy(phyAddr, trans.get_data_ptr(), trans.get_data_length());
    }
    else
    {
        for (std::size_t i = 0; i < trans.get_data_length(); i++)
        {
            std::size_t byteEnableIndex = i % trans.get_byte_enable_length();
            if (trans.get_byte_enable_ptr()[byteEnableIndex] == TLM_BYTE_ENABLED)
            {
                phyAddr[i] = trans.get_data_ptr()[i];
            }
        }
    }
}

void Dram::serialize(std::ostream& stream) const
{
    stream.write(reinterpret_cast<char const*>(memory), channelSize);
}

void Dram::deserialize(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(memory), channelSize);
}

} // namespace DRAMSys
