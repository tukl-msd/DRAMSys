/*
 * Copyright (c) 2017, Technische Universität Kaiserslautern
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
 *    Johannes Feldmann
 *    Eder F. Zulian
 */

#ifndef ECCBASECLASS_H
#define ECCBASECLASS_H

#include <systemc>
#include <tlm>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include "ECC/ECC.h"

#include "../common/DebugManager.h"

class ECCBaseClass : sc_core::sc_module
{
public:
    struct DataStruct {
        unsigned char *pData;
        unsigned int nDataSize;
    };

private:
    std::map<unsigned char *, DataStruct> m_mDataPointer;

public:
    // Function prototype for calculated the size of memory needed for saving the encoded data
    // Input nBytes: Number of bytes which have to be encoded
    // Return value: Number of bytes which have to be allocated for storing the encoded data
    virtual unsigned AllocationSize(unsigned nBytes) = 0;

protected:
    // Function prototype for encoding data.
    // Data pointer is provided in pDataIn, length in Bytes provided in nDataIn
    // Result should be written in pDataOut, which has a size of nDataOut.
    // pDataOut is already allocated with a size given by function AllocationEncode
    virtual void Encode(const unsigned char *pDataIn, unsigned nDataIn,
                        unsigned char *pDataOut, unsigned nDataOut) = 0;


    // Function prototype for decoding data.
    // Data pointer is provided in pDataIn, length in Bytes provided in nDataIn
    // Result should be written in pDataOut, which has a size of nDataOut.
    // pDataOut is already allocated with a size given by function AllocationDecode
    virtual void Decode(const unsigned char *pDataIn, unsigned nDataIn,
                        unsigned char *pDataOut, unsigned nDataOut) = 0;

public:
    tlm_utils::multi_passthrough_target_socket<ECCBaseClass>    t_socket;
    tlm_utils::multi_passthrough_initiator_socket<ECCBaseClass> i_socket;

    SC_CTOR(ECCBaseClass)
        : t_socket("t_socket")
        , i_socket("i_socket")
    {
        t_socket.register_nb_transport_fw(this, &ECCBaseClass::nb_transport_fw);
        i_socket.register_nb_transport_bw(this, &ECCBaseClass::nb_transport_bw);
    }
    // Forward interface
    tlm::tlm_sync_enum nb_transport_fw( int id, tlm::tlm_generic_payload &trans,
                                        tlm::tlm_phase &phase, sc_core::sc_time &delay );

    // Backward interface
    tlm::tlm_sync_enum nb_transport_bw( int id, tlm::tlm_generic_payload &trans,
                                        tlm::tlm_phase &phase, sc_core::sc_time &delay );
};

#endif // ECCBASECLASS_H
