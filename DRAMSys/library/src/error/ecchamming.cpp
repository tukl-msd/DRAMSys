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

#include "ecchamming.h"

#include "ECC/ECC.h"

unsigned ECCHamming::AllocationSize(unsigned nBytes)
{
    // For this hamming for 8 bytes one extra byte is needed:l
    return nBytes + ceil(nBytes / m_nDatawordSize) * m_nCodewordSize;
}

void ECCHamming::Encode(const unsigned char *pDataIn, const unsigned nDataIn,
                        unsigned char *pDataOut, const unsigned nDataOut)
{
    // Calculate how many 8 byte blocks are there
    unsigned nBlocks = nDataIn / m_nDatawordSize;

    // No partly filled blocks are supported
    assert(nDataIn % m_nDatawordSize == 0);

    // Create ECC data for every block
    for (unsigned i = 0; i < nBlocks; i++) {
        // Create all variables needed for calulation
        CWord dataword(InBits(m_nDatawordSize));    // Size in bits
        CWord codeword(InBits(m_nCodewordSize));   // Size in bits

        // Fill in current data block
        dataword.Set(&pDataIn[i * m_nDatawordSize], InBits(m_nDatawordSize));

        // Extend data word. It grows from m_nDatawordSize to m_nDatawordSize + m_nCodewordSize
        ECC::ExtendWord(dataword);

        // Initialize the codeword with zeros
        codeword = 0;

        // Calculate Checkbits
        ECC::CalculateCheckbits(dataword, codeword);
        ECC::InsertCheckbits(dataword, codeword);

        // Calculate Parity
        ECC::CalculateParityBit(dataword, codeword[7]);

        // Check if there is enough space in the output array (should always be)
        assert((i + 1) * (m_nDatawordSize + m_nCodewordSize) <= nDataOut);

        // Copy old data
        memcpy(&pDataOut[i * (m_nDatawordSize + m_nCodewordSize)],
               &pDataIn[i * m_nDatawordSize], m_nDatawordSize);

        // Save hamming code + parity bit in the last byte
        codeword.Copy(&pDataOut[i * (m_nDatawordSize + m_nCodewordSize) +
                                m_nDatawordSize]);
    }
}

void ECCHamming::Decode(const unsigned char *pDataIn, const unsigned nDataIn,
                        unsigned char *pDataOut, const unsigned nDataOut)
{
    // Calculate how many 9 byte blocks are there
    unsigned nBlocks = nDataIn / (m_nDatawordSize + m_nCodewordSize);

    // No partly filled blocks are supported
    assert(nDataIn % (m_nDatawordSize + m_nCodewordSize) == 0);

    // Verify ECC data for every block
    for (unsigned i = 0; i < nBlocks; i++) {
        // Create all variables needed for calulation
        CWord dataword(InBits(m_nDatawordSize));    // Size in bits
        CWord codeword(InBits(m_nCodewordSize));   // Size in bits

        // Fill in current data block
        dataword.Set(&pDataIn[i * (m_nDatawordSize + m_nCodewordSize)],
                     InBits(m_nDatawordSize));
        codeword.Set(&pDataIn[i * (m_nDatawordSize + m_nCodewordSize) +
                              m_nDatawordSize], InBits(m_nCodewordSize));

        // Extend data word. It grows from m_nDatawordSize to m_nDatawordSize + m_nCodewordSize
        ECC::ExtendWord(dataword);

        // Insert old codeword
        ECC::InsertCheckbits(dataword, codeword);
        ECC::InsertParityBit(dataword, codeword[7]);

        // Reset codeword
        codeword = 0;

        // Calculate Checkbits again
        ECC::CalculateCheckbits(dataword, codeword);

        // Calculate Parity again
        ECC::CalculateParityBit(dataword, codeword[7]);

        // Translate codeword
        bool bParity = codeword[7] == CBit::ONE;

        // calculate syndrome
        unsigned char c = 0;
        codeword.Rotate();
        codeword.Copy(&c);
        c &= 0x7F;

        // Parity Error?
        if (bParity) {
            // Parity Error

            if (c == 0) {
                // Only Parity Bit broken - continue
                std::cout << "Parity Bit error" << std::endl;
            } else {
                // Data or Hamming Code Bit broken
                std::cout << "Single Error Detected" << std::endl;
            }
        } else {
            // No Parity Error

            if (c == 0) {
                // No error at all - continue
            } else {
                // Double error detected
                std::cout << "Double Error Detected (Block " << i << ")." << std::endl;
            }
        }

        // Check if there is enough space in the output array (should always be)
        assert((i + 1) * (m_nDatawordSize) <= nDataOut);

        // Copy data
        memcpy(&pDataOut[i * m_nDatawordSize],
               &pDataIn[i * (m_nDatawordSize + m_nCodewordSize)], m_nDatawordSize);
    }
}
