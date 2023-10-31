/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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
 *    Kirill Bykov
 *    Lukas Steiner
 */

#ifndef CONTROLLERIF_H
#define CONTROLLERIF_H

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/configuration/Configuration.h"

#include <iomanip>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

namespace DRAMSys
{

// Utility class to pass around DRAMSys, without having to propagate the template definitions
// throughout all classes
class ControllerIF : public sc_core::sc_module
{
public:
    // Already create and bind sockets to the virtual functions
    tlm_utils::simple_target_socket<ControllerIF> tSocket;    // Arbiter side
    tlm_utils::simple_initiator_socket<ControllerIF> iSocket; // DRAM side

    void end_of_simulation() override
    {
        idleTimeCollector.end();

        sc_core::sc_time activeTime = static_cast<double>(numberOfBeatsServed) / memSpec.dataRate *
                                      memSpec.tCK / memSpec.pseudoChannelsPerChannel;

        double bandwidth = activeTime / sc_core::sc_time_stamp();
        double bandwidthWoIdle =
            activeTime / (sc_core::sc_time_stamp() - idleTimeCollector.getIdleTime());

        double maxBandwidth = (
            // fCK in GHz e.g. 1 [GHz] (tCK in ps):
            (1000 / memSpec.tCK.to_double())
            // DataRate e.g. 2
            * memSpec.dataRate
            // BusWidth e.g. 8 or 64
            * memSpec.bitWidth
            // Number of devices that form a rank, e.g., 8 on a DDR3 DIMM
            * memSpec.devicesPerRank
            // HBM specific, one or two pseudo channels per channel
            * memSpec.pseudoChannelsPerChannel);

        std::cout << name() << std::string("  Total Time:     ")
                  << sc_core::sc_time_stamp().to_string() << std::endl;
        std::cout << name() << std::string("  AVG BW:         ") << std::fixed
                  << std::setprecision(2) << std::setw(6) << (bandwidth * maxBandwidth)
                  << " Gb/s | " << std::setw(6) << (bandwidth * maxBandwidth / 8) << " GB/s | "
                  << std::setw(6) << (bandwidth * 100) << " %" << std::endl;
        std::cout << name() << std::string("  AVG BW\\IDLE:    ") << std::fixed
                  << std::setprecision(2) << std::setw(6) << (bandwidthWoIdle * maxBandwidth)
                  << " Gb/s | " << std::setw(6) << (bandwidthWoIdle * maxBandwidth / 8)
                  << " GB/s | " << std::setw(6) << (bandwidthWoIdle * 100) << " %" << std::endl;
        std::cout << name() << std::string("  MAX BW:         ") << std::fixed
                  << std::setprecision(2) << std::setw(6) << maxBandwidth << " Gb/s | "
                  << std::setw(6) << maxBandwidth / 8 << " GB/s | " << std::setw(6) << 100.0 << " %"
                  << std::endl;
    }

    [[nodiscard]] virtual bool idle() const = 0;

protected:
    const MemSpec& memSpec;

    // Bind sockets with virtual functions
    ControllerIF(const sc_core::sc_module_name& name, const Configuration& config) :
        sc_core::sc_module(name),
        tSocket("tSocket"),
        iSocket("iSocket"),
        memSpec(*config.memSpec)
    {
        tSocket.register_nb_transport_fw(this, &ControllerIF::nb_transport_fw);
        tSocket.register_transport_dbg(this, &ControllerIF::transport_dbg);
        tSocket.register_b_transport(this, &ControllerIF::b_transport);
        iSocket.register_nb_transport_bw(this, &ControllerIF::nb_transport_bw);

        idleTimeCollector.start();
    }
    SC_HAS_PROCESS(ControllerIF);

    // Virtual transport functions
    virtual tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& delay) = 0;
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& delay) = 0;
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) = 0;
    virtual unsigned int transport_dbg(tlm::tlm_generic_payload& trans) = 0;

    // Bandwidth related
    class IdleTimeCollector
    {
    public:
        void start()
        {
            if (!isIdle)
            {
                PRINTDEBUGMESSAGE("IdleTimeCollector", "IDLE start");
                idleStart = sc_core::sc_time_stamp();
                isIdle = true;
            }
        }

        void end()
        {
            if (isIdle)
            {
                PRINTDEBUGMESSAGE("IdleTimeCollector", "IDLE end");
                idleTime += sc_core::sc_time_stamp() - idleStart;
                isIdle = false;
            }
        }

        sc_core::sc_time getIdleTime() { return idleTime; }

    private:
        bool isIdle = false;
        sc_core::sc_time idleTime = sc_core::SC_ZERO_TIME;
        sc_core::sc_time idleStart;
    } idleTimeCollector;

    uint64_t numberOfBeatsServed = 0;
};

} // namespace DRAMSys

#endif // CONTROLLERIF_H
