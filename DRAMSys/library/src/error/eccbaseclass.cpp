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

#include "eccbaseclass.h"

using namespace sc_core;
using namespace tlm;

tlm::tlm_sync_enum ECCBaseClass::nb_transport_fw( int id,
                                                  tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase, sc_time &delay )
{
    if (trans.get_command() == TLM_WRITE_COMMAND && phase == BEGIN_REQ) {
        // Allocate memory for encoded data using the size provided by AllocationEncode
        unsigned nEncodedDataSize = AllocationSize(trans.get_data_length());
        assert(nEncodedDataSize != 0);
        unsigned char *pEncodedData = new unsigned char[nEncodedDataSize];

        // Save memory pointer and size
        m_mDataPointer[pEncodedData].pData = trans.get_data_ptr();
        m_mDataPointer[pEncodedData].nDataSize = trans.get_data_length();

        // Data Encoding
        Encode(trans.get_data_ptr(), trans.get_data_length(), pEncodedData,
               nEncodedDataSize);

        // Change transport data length and pointer
        trans.set_data_length(nEncodedDataSize);
        trans.set_data_ptr(pEncodedData);
    } else if (trans.get_command() == TLM_READ_COMMAND && phase == BEGIN_REQ) {
        // Allocate memory for reading data using the size provided by AllocationEncode
        unsigned nReadDataSize = AllocationSize(trans.get_data_length());
        assert(nReadDataSize != 0);
        unsigned char *pReadData = new unsigned char[nReadDataSize];

        // Save memory pointer and size
        m_mDataPointer[pReadData].pData = trans.get_data_ptr();
        m_mDataPointer[pReadData].nDataSize = trans.get_data_length();

        // Change transport data length and pointer
        trans.set_data_length(nReadDataSize);
        trans.set_data_ptr(pReadData);
    }

    return i_socket[id]->nb_transport_fw( trans, phase, delay );
}


// Backward interface
tlm::tlm_sync_enum ECCBaseClass::nb_transport_bw( int id,
                                                  tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase, sc_time &delay )
{
    if (trans.get_command() == TLM_READ_COMMAND && phase == BEGIN_RESP) {
        //Look for the corresponding data pointer for decoding
        auto it = m_mDataPointer.find(trans.get_data_ptr());
        assert(it != m_mDataPointer.end());

        // Data Decoding
        Decode(trans.get_data_ptr(), trans.get_data_length(), it->second.pData,
               it->second.nDataSize);

        // delete data pointer from map
        m_mDataPointer.erase(it);

        // Delete data pointer used for encoded data
        delete[] trans.get_data_ptr();

        // Set data pointer and size for decoded data
        trans.set_data_ptr(it->second.pData);
        trans.set_data_length(it->second.nDataSize);
    } else if (trans.get_command() == TLM_WRITE_COMMAND && phase == BEGIN_RESP) {
        //Look for the corresponding data pointer for decoding
        auto it = m_mDataPointer.find(trans.get_data_ptr());
        assert(it != m_mDataPointer.end());

        // delete data pointer from map
        m_mDataPointer.erase(it);

        // Delete data pointer used for encoded data
        delete[] trans.get_data_ptr();

        // Set data pointer and size for decoded data
        trans.set_data_ptr(it->second.pData);
        trans.set_data_length(it->second.nDataSize);
    }

    return t_socket[id]->nb_transport_bw( trans, phase, delay );
}
