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

#include "Dram.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif
#include <tlm.h>
#include <systemc.h>
#include <tlm_utils/simple_target_socket.h>
#include <vector>
#include <array>
#include <cassert>
#include <cstdint>
#include <stdlib.h>
#include "../../common/DebugManager.h"
#include "../../common/dramExtensions.h"
#include "../../configuration/Configuration.h"
#include "../../common/utils.h"
#include "../../common/third_party/DRAMPower/src/libdrampower/LibDRAMPower.h"
#include "../../common/third_party/DRAMPower/src/MemCommand.h"
#include "../../controller/Command.h"

using namespace tlm;
using namespace DRAMPower;

Dram::Dram(sc_module_name name) : sc_module(name), tSocket("socket")
{
    Configuration &config = Configuration::getInstance();
    // Adjust number of bytes per burst dynamically to the selected ecc controller
    bytesPerBurst = config.adjustNumBytesAfterECC(bytesPerBurst);

    if (config.storeMode == "NoStorage")
        storeMode = StorageMode::NoStorage;
    else if (config.storeMode == "Store")
        storeMode = StorageMode::Store;
    else if (config.storeMode == "ErrorModel")
        storeMode = StorageMode::ErrorModel;
    else
        SC_REPORT_FATAL(this->name(), "Unsupported storage mode");

    uint64_t memorySize = Configuration::getInstance().getSimMemSizeInBytes();
    if (storeMode == StorageMode::Store)
    {
        if (Configuration::getInstance().useMalloc)
        {
            memory = (unsigned char *)malloc(memorySize);
            if (!memory)
                SC_REPORT_FATAL(this->name(), "Memory allocation failed");
        }
        else
        {
            // allocate and model storage of one DRAM channel using memory map
            #ifdef _WIN32
                SC_REPORT_FATAL("Dram", "On Windows Storage is not yet supported");
                memory = 0; // FIXME
            #else
                memory = (unsigned char *)mmap(NULL, memorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
            #endif
        }
    }

    tSocket.register_nb_transport_fw(this, &Dram::nb_transport_fw);
    tSocket.register_transport_dbg(this, &Dram::transport_dbg);
}

Dram::~Dram()
{
    if (Configuration::getInstance().powerAnalysis)
    {
        reportPower();
        delete DRAMPower;
    }

    if (Configuration::getInstance().useMalloc)
        free(memory);
}

void Dram::reportPower()
{
    if (!powerReported)
    {
        powerReported = true;
        DRAMPower->calcEnergy();

        // Print the final total energy and the average power for
        // the simulation:
        std::cout << name() << std::string("  Total Energy:   ")
             << std::fixed << std::setprecision( 2 )
             << DRAMPower->getEnergy().total_energy
             * Configuration::getInstance().memSpec->numberOfDevicesOnDIMM
             << std::string(" pJ")
             << std::endl;

        std::cout << name() << std::string("  Average Power:  ")
             << std::fixed << std::setprecision( 2 )
             << DRAMPower->getPower().average_power
             * Configuration::getInstance().memSpec->numberOfDevicesOnDIMM
             << std::string(" mW") << std::endl;
    }
}

tlm_sync_enum Dram::nb_transport_fw(tlm_generic_payload &payload,
                                           tlm_phase &phase, sc_time &)
{
    assert(phase >= 5 && phase <= 19);

    if (Configuration::getInstance().powerAnalysis)
    {
        unsigned int bank = DramExtension::getExtension(payload).getBank().ID();
        unsigned long long cycle = sc_time_stamp() / memSpec->tCK;
        DRAMPower->doCommand(phaseToDRAMPowerCommand(phase), bank, cycle);
    }

    if (storeMode == StorageMode::Store)
    {
        if (phase == BEGIN_RD || phase == BEGIN_RDA)
        {
            unsigned char *phyAddr = memory + payload.get_address();
            memcpy(payload.get_data_ptr(), phyAddr, payload.get_data_length());
        }
        else if (phase == BEGIN_WR || phase == BEGIN_WRA)
        {
            unsigned char *phyAddr = memory + payload.get_address();
            memcpy(phyAddr, payload.get_data_ptr(), payload.get_data_length());
        }
    }

    return TLM_ACCEPTED;
}

unsigned int Dram::transport_dbg(tlm_generic_payload &trans)
{
    PRINTDEBUGMESSAGE(name(), "transport_dgb");

    // TODO: This part is not tested yet, neither with traceplayers nor with GEM5 coupling
    if (storeMode == StorageMode::NoStorage)
    {
        SC_REPORT_FATAL("DRAM",
                        "Debug Transport is used in combination with NoStorage");
    }
    else
    {
        tlm_command cmd = trans.get_command();
        //uint64_t    adr = trans.get_address(); // TODO: - offset;
        unsigned char   *ptr = trans.get_data_ptr();
        unsigned int     len = trans.get_data_length();
        //unsigned int bank    = DramExtension::getExtension(trans).getBank().ID();

        //cout << "cmd " << (cmd ? "write" : "read") << " adr " << hex << adr << " len " << len << endl;

        if (cmd == TLM_READ_COMMAND)
        {
            if (storeMode == StorageMode::Store)
            { // Use Storage
                unsigned char *phyAddr = memory + trans.get_address();
                memcpy(ptr, phyAddr, trans.get_data_length());
            }
            else
            {
                //ememory[bank]->load(trans);
                SC_REPORT_FATAL("DRAM", "Debug transport not supported with error model yet.");
            }
        }
        else if (cmd == TLM_WRITE_COMMAND)
        {
            if (storeMode == StorageMode::Store)
            { // Use Storage
                unsigned char *phyAddr = memory + trans.get_address();
                memcpy(phyAddr, ptr, trans.get_data_length());
            }
            else
            {
                //ememory[bank]->store(trans);
                SC_REPORT_FATAL("DRAM", "Debug transport not supported with error model yet.");
            }
        }
        return len;
    }
    return 0;
}
