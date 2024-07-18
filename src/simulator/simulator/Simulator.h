/*
 * Copyright (c) 2023, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#pragma once

#include "Initiator.h"
#include "MemoryManager.h"

#include <DRAMSys/config/DRAMSysConfiguration.h>
#include <DRAMSys/simulation/DRAMSys.h>

static constexpr std::string_view TRACE_DIRECTORY = "traces";

class Simulator
{
public:
    Simulator(DRAMSys::Config::Configuration configuration,
              std::filesystem::path resourceDirectory);

    static void run();

private:
    std::unique_ptr<Initiator> instantiateInitiator(const DRAMSys::Config::Initiator& initiator);

    const bool storageEnabled;
    MemoryManager memoryManager;

    DRAMSys::Config::Configuration configuration;
    std::filesystem::path resourceDirectory;

    std::unique_ptr<DRAMSys::DRAMSys> dramSys;
    std::vector<std::unique_ptr<Initiator>> initiators;

    std::function<void()> terminateInitiator;
    std::function<void()> finishTransaction;

    unsigned int terminatedInitiators = 0;
    uint64_t totalTransactions{};
    uint64_t transactionsFinished = 0;
};
