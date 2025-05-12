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

#include "Dram.h"

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/config/SimConfig.h"
#include <sysc/kernel/sc_module.h>

#include <cassert>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <DRAMPower/command/Command.h>

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

Dram::Dram(const sc_module_name& name,
           const SimConfig& simConfig,
           const MemSpec& memSpec,
           TlmRecorder* tlmRecorder) :
    sc_module(name),
    memSpec(memSpec),
    storeMode(simConfig.storeMode),
    channelSize(memSpec.getSimMemSizeInBytes() / memSpec.numberOfChannels),
    useMalloc(simConfig.useMalloc),
    tlmRecorder(tlmRecorder),
    powerWindowSize(memSpec.tCK * simConfig.windowSize)
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

    if (simConfig.powerAnalysis)
    {
        DRAMPower = memSpec.toDramPowerObject();
        if (DRAMPower && storeMode == Config::StoreModeType::NoStorage) {
            if (simConfig.togglingRate) {
                DRAMPower->setToggleRate(0, simConfig.togglingRate);
            } else {
                SC_REPORT_FATAL("DRAM", "Toggling rates for power estimation must be provided for storeMode NoStorage");
            }
        }
    }

    if (simConfig.powerAnalysis && simConfig.enableWindowing)
        SC_THREAD(powerWindow);
}

Dram::~Dram()
{
    if (useMalloc)
        free(memory);
}

void Dram::reportPower()
{
    if (!DRAMPower)
        return;

    double energy = DRAMPower->getTotalEnergy(DRAMPower->getLastCommandTime());
    double time = DRAMPower->getLastCommandTime() * memSpec.tCK.to_seconds();

    // Print the final total energy and the average power for
    // the simulation:
    std::cout << name() << std::string("  Total Energy:   ") << std::defaultfloat << std::setprecision(3)
              << energy << std::string(" J")
              << std::endl;

    std::cout << name() << std::string("  Average Power:  ") << std::defaultfloat << std::setprecision(3)
              << energy / time << std::string(" W")
              << std::endl;

    if (tlmRecorder != nullptr)
    {
        tlmRecorder->recordPower(sc_time_stamp().to_seconds(),
                                 energy / time);
    }
}

tlm_sync_enum Dram::nb_transport_fw(tlm_generic_payload& trans, tlm_phase& phase, sc_time& delay)
{
    assert(phase >= BEGIN_RD && phase <= END_SREF);

    if (DRAMPower)
    {
        std::size_t rank = static_cast<std::size_t>(ControllerExtension::getRank(trans)); // relaitve to the channel
        std::size_t bank_group_abs = static_cast<std::size_t>(ControllerExtension::getBankGroup(trans)); // relative to the channel
        std::size_t bank_group = bank_group_abs - rank * memSpec.groupsPerRank; // relative to the rank
        std::size_t bank = static_cast<std::size_t>(ControllerExtension::getBank(trans)) - bank_group_abs * memSpec.banksPerGroup; // relative to the bank_group
        std::size_t row = static_cast<std::size_t>(ControllerExtension::getRow(trans));
        std::size_t column = static_cast<std::size_t>(ControllerExtension::getColumn(trans));
        uint64_t cycle = std::lround((sc_time_stamp() + delay) / memSpec.tCK);

        // DRAMPower:
        // banks are relative to the rank
        // bankgroups are relative to the rank
        bank = bank + (bank_group * memSpec.banksPerGroup);
        
        DRAMPower::TargetCoordinate target(bank, bank_group, rank, row, column);

        // TODO read, write data for interface calculation
        uint8_t * data = trans.get_data_ptr(); // Can be nullptr if no data
        std::size_t datasize = trans.get_data_length() * 8; // Is always set

        DRAMPower::Command command(cycle, phaseToDRAMPowerCommand(phase), target, data, datasize);
        DRAMPower->doCoreInterfaceCommand(command);
    }

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

void Dram::powerWindow()
{
    int64_t clkCycles = 0;
    double previousEnergy = 0;
    double currentEnergy = 0;
    double windowEnergy = 0;
    double powerWindowSizeSeconds = powerWindowSize.to_seconds();

    while (true)
    {
        // At the very beginning (zero clock cycles) the energy is 0, so we wait first
        sc_module::wait(powerWindowSize);

        clkCycles = std::lround(sc_time_stamp() / this->memSpec.tCK);

        currentEnergy = this->DRAMPower->getTotalEnergy(clkCycles);
        windowEnergy = currentEnergy - previousEnergy;

        // During operation the energy should never be zero since the device is always consuming
        assert(!(windowEnergy < 1e-15));

        if (tlmRecorder)
        {
            // Store the time (in seconds) and the current average power (in mW) into the database
            tlmRecorder->recordPower(sc_time_stamp().to_seconds(),
                                     windowEnergy / powerWindowSizeSeconds);
        }

        // Here considering that DRAMPower provides the energy in J and the power in W
        PRINTDEBUGMESSAGE(this->name(),
                          std::string("\tWindow Energy: \t") +
                              std::to_string(windowEnergy) +
                              std::string("\t[J]"));
        PRINTDEBUGMESSAGE(this->name(),
                          std::string("\tWindow Average Power: \t") +
                              std::to_string(windowEnergy / powerWindowSizeSeconds) +
                              std::string("\t[W]"));
    }
}

} // namespace DRAMSys
