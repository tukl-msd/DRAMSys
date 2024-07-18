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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Eder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 *    Derek Christ
 */

#ifndef DRAMSYS_H
#define DRAMSYS_H

#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/common/tlm2_base_protocol_checker.h"
#include "DRAMSys/config/DRAMSysConfiguration.h"
#include "DRAMSys/controller/Controller.h"
#include "DRAMSys/controller/ControllerRecordable.h"
#include "DRAMSys/controller/McConfig.h"
#include "DRAMSys/simulation/AddressDecoder.h"
#include "DRAMSys/simulation/Arbiter.h"
#include "DRAMSys/simulation/Dram.h"
#include "DRAMSys/simulation/DramRecordable.h"
#include "DRAMSys/simulation/SimConfig.h"

#include <list>
#include <memory>
#include <string>
#include <systemc>
#include <tlm>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

namespace DRAMSys
{

class DRAMSys : public sc_core::sc_module
{
public:
    tlm_utils::multi_passthrough_target_socket<DRAMSys> tSocket{"DRAMSys_tSocket"};

    SC_HAS_PROCESS(DRAMSys);
    DRAMSys(const sc_core::sc_module_name& name, const Config::Configuration& config);

    const auto& getSimConfig() const { return simConfig; }
    const auto& getMcConfig() const { return mcConfig; }
    const auto& getMemSpec() const { return *memSpec; }
    const auto& getAddressDecoder() const { return *addressDecoder; }

    /**
     * Returns true if all memory controllers are in idle state.
     */
    [[nodiscard]] bool idle() const;

    /**
     * Registers a callback that is called whenever a memory controller switches to the idle state.
     * Check afterwards with idle() if all memory controllers are now idle.
     */
    void registerIdleCallback(const std::function<void()>& idleCallback);

private:
    static void logo();
    static std::unique_ptr<const MemSpec> createMemSpec(const Config::MemSpec& memSpec);
    static std::unique_ptr<Arbiter> createArbiter(const SimConfig& simConfig,
                                                  const McConfig& mcConfig,
                                                  const MemSpec& memSpec,
                                                  const AddressDecoder& addressDecoder);

    void end_of_simulation() override;

    void setupDebugManager(const std::string& traceName) const;
    void setupTlmRecorders(const std::string& traceName, const Config::Configuration& configLib);

    void report();

    std::unique_ptr<const MemSpec> memSpec;
    SimConfig simConfig;
    McConfig mcConfig;

    std::unique_ptr<AddressDecoder> addressDecoder;

    // TLM 2.0 Protocol Checkers
    std::vector<std::unique_ptr<tlm_utils::tlm2_base_protocol_checker<>>> controllersTlmCheckers;

    // All transactions pass through the same arbiter
    std::unique_ptr<Arbiter> arbiter;

    // Each DRAM unit has a controller
    std::vector<std::unique_ptr<Controller>> controllers;

    // DRAM units
    std::vector<std::unique_ptr<Dram>> drams;

    // Transaction Recorders (one per channel).
    // They generate the output databases.
    std::vector<TlmRecorder> tlmRecorders;
};

} // namespace DRAMSys

#endif // DRAMSYS_H
