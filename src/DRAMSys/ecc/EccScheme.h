/*
 * Copyright (c) 2024, RPTU Kaiserslautern-Landau
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
 * Author: Derek Christ
 */

#ifndef ECCSCHEME_H
#define ECCSCHEME_H

#include <tlm>

namespace DRAMSys
{

class EccScheme
{
public:
    EccScheme() = default;
    EccScheme(const EccScheme&) = delete;
    EccScheme(EccScheme&&) = delete;
    EccScheme& operator=(const EccScheme&) = delete;
    EccScheme& operator=(EccScheme&&) = delete;
    virtual ~EccScheme() = default;

    // Returns whether transaction could be registered
    [[nodiscard]] virtual bool registerRequest(tlm::tlm_generic_payload& payload) = 0;

    // Whether response should be forwarded to initiator
    [[nodiscard]] virtual bool registerResponse(tlm::tlm_generic_payload* payload) = 0;

    [[nodiscard]] virtual tlm::tlm_generic_payload* getNextRequest() = 0;
    [[nodiscard]] virtual tlm::tlm_generic_payload* getNextResponse() = 0;
};

} // namespace DRAMSys

#endif // ECCSCHEME_H
