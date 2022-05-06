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

#ifndef ECCHAMMIMG_H
#define ECCHAMMIMG_H

#include "eccbaseclass.h"


class ECCHamming : public ECCBaseClass
{
private:
    // Hamming constants for this special implementation
    const unsigned m_nDatawordSize = 8;     // bytes
    const unsigned m_nCodewordSize = 1;     // bytes

    inline unsigned InBits(unsigned n)
    {
        return n << 3;
    };    // use this if constants are needed in bits

public:
    // Function prototype for calculated the size of memory needed for saving the encoded data
    // Input nBytes: Number of bytes which have to be encoded
    // Return value: Number of bytes which have to be allocated for storing the encoded data
    virtual unsigned AllocationSize(unsigned nBytes);

protected:
    // Function prototype for encoding data.
    // Data pointer is provided in pDataIn, length in Bytes provided in nDataIn
    // Result should be written in pDataOut, which has a size of nDataOut.
    // pDataOut is already allocated with a size given by function AllocationEncode
    virtual void Encode(const unsigned char *pDataIn, unsigned nDataIn,
                        unsigned char *pDataOut, unsigned nDataOut);


    // Function prototype for decoding data.
    // Data pointer is provided in pDataIn, length in Bytes provided in nDataIn
    // Result should be written in pDataOut, which has a size of nDataOut.
    // pDataOut is already allocated with a size given by function AllocationDecode
    virtual void Decode(const unsigned char *pDataIn, unsigned nDataIn,
                        unsigned char *pDataOut, unsigned nDataOut);

public:
    ECCHamming(::sc_core::sc_module_name name) : ECCBaseClass(name)
    {};
};

#endif // ECCHAMMIMG_H
