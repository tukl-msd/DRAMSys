/*
 * Copyright (c) 2022, RPTU Kaiserslautern-Landau
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
 *    Christian Malek
 *    Derek Christ
 */

#ifndef INLINEECC_H
#define INLINEECC_H

#include "EccScheme.h"

#include <DRAMSys/simulation/AddressDecoder.h>
#include <DRAMSys/simulation/SimConfig.h>

#include <memory>
#include <systemc>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

namespace DRAMSys
{

class EccExtension : public tlm::tlm_extension<EccExtension>
{
public:
    [[nodiscard]] tlm_extension_base* clone() const override { return new EccExtension; }

    void copy_from([[maybe_unused]] tlm_extension_base const& ext) override {}
};

class InlineEcc : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<InlineEcc> iSocket;
    tlm_utils::simple_target_socket<InlineEcc> tSocket;

    InlineEcc(sc_core::sc_module_name const& name,
              Config::InlineEccType inlineEccType,
              std::shared_ptr<const AddressDecoder> addressDecoder,
              sc_core::sc_time tCK,
              unsigned int atomSize);

private:
    static std::unique_ptr<EccScheme>
    createEccScheme(Config::InlineEccType inlineEccType,
                    std::shared_ptr<const AddressDecoder> addressDecoder,
                    unsigned int atomSize);

    void peqCallback(tlm::tlm_generic_payload& payload, const tlm::tlm_phase& phase);

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& payload,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& fwDelay);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& payload,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& bwDelay);

    void sendResponse(tlm::tlm_generic_payload* payload);

    tlm_utils::peq_with_cb_and_phase<InlineEcc> payloadEventQueue;

    tlm::tlm_generic_payload* backPressureRequest = nullptr;
    tlm::tlm_generic_payload* pendingRequest = nullptr;
    tlm::tlm_generic_payload* pendingResponse = nullptr;
    bool targetBusy = false;
    bool initiatorBusy = false;

    sc_core::sc_time tCK;
    std::shared_ptr<const AddressDecoder> addressDecoder;

    std::unique_ptr<EccScheme> eccScheme;
};

} // namespace DRAMSys

#endif // INLINECC_H
