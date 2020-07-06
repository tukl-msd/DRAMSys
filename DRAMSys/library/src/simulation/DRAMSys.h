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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Eder F. Zulian
 *    Felipe S. Prado
 *    Lukas Steiner
 */

#ifndef DRAMSYS_H
#define DRAMSYS_H

#include <string>
#include <systemc.h>

#include "dram/Dram.h"
#include "Arbiter.h"
#include "ReorderBuffer.h"
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include "../common/tlm2_base_protocol_checker.h"
#include "../error/eccbaseclass.h"
#include "../controller/ControllerIF.h"

class DRAMSys : public sc_module
{
public:
    tlm_utils::multi_passthrough_target_socket<DRAMSys> tSocket;

    std::vector<tlm_utils::tlm2_base_protocol_checker<>*>
    playersTlmCheckers;

    SC_HAS_PROCESS(DRAMSys);
    DRAMSys(sc_module_name name,
            std::string simulationToRun,
            std::string pathToResources);

    virtual ~DRAMSys();

protected:
    DRAMSys(sc_module_name name,
            std::string simulationToRun,
            std::string pathToResources,
            bool initAndBind);

    //TLM 2.0 Protocol Checkers
    std::vector<tlm_utils::tlm2_base_protocol_checker<>*>
    controllersTlmCheckers;

    // All transactions pass first through the ECC Controller
    ECCBaseClass *ecc;

    // TODO: Each DRAM has a reorder buffer (check this!)
    ReorderBuffer *reorder;

    // All transactions pass through the same arbiter
    Arbiter *arbiter;

    // Each DRAM unit has a controller
    std::vector<ControllerIF *> controllers;

    // DRAM units
    std::vector<Dram *> drams;

    void report(std::string message);

private:
    void logo();

    void instantiateModules(const std::string &pathToResources,
                            const std::string &amconfig);
    void bindSockets();

    void setupDebugManager(const std::string &traceName);
};

#endif // DRAMSYS_H
