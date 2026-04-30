/*
 * Copyright (c) 2026, Julius-Maximilians-Universität Würzburg
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

#include "DramMonitor.h"

#include "database/DatabaseRecorder.h"

#include <DRAMSys/common/dramExtensions.h>
#include <DRAMSys/configuration/memspec/MemSpec.h>

namespace DRAMSys::Recording
{

DramMonitor::DramMonitor(sc_core::sc_module_name const& name,
                         unsigned command_bus_id,
                         unsigned data_bus_id,
                         MemSpec const* memspec,
                         DatabaseRecorder* recorder) :
    sc_core::sc_module(name),
    command_bus_id(command_bus_id),
    data_bus_id(data_bus_id),
    memspec(memspec),
    recorder(recorder)
{
    tSocket.register_b_transport(this, &DramMonitor::b_transport);
}

void DramMonitor::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    auto* ext = trans.get_extension<CommandExtension>();
    auto arbiter_ext = ArbiterExtension::getExtension(trans);
    auto controller_ext = ControllerExtension::getExtension(trans);

    auto command_begin = sc_core::sc_time_stamp() + delay;

    auto command_strobe_id = recorder->record(CommandStrobeRecord{
        command_begin.value(),
        ext->command,
        command_bus_id,
        static_cast<unsigned>(arbiter_ext.transactionId),
        static_cast<unsigned>(controller_ext.rank),
        static_cast<unsigned>(controller_ext.bankGroup),
        static_cast<unsigned>(controller_ext.bank),
        static_cast<unsigned>(controller_ext.dualBank),
        static_cast<unsigned>(controller_ext.row),
        static_cast<unsigned>(controller_ext.column),
    });

    recorder->record(DramStateRecord{
        command_begin.value(),
        (command_begin + memspec->getExecutionTime(ext->command, trans)).value(),
        command_strobe_id,
    });

    if (ext->command.isCasCommand())
    {
        auto interval = memspec->getIntervalOnDataStrobe(ext->command, trans);
        recorder->record(DataStrobeRecord{
            (command_begin + interval.start).value(),
            (command_begin + interval.end).value(),
            ControllerExtension::getBurstLength(trans),
            data_bus_id,
            static_cast<unsigned>(command_strobe_id),
        });
    }

    iSocket->b_transport(trans, delay);
}

} // namespace DRAMSys::Recording
