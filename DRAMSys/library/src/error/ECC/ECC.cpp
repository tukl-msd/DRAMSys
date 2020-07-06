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

#include "ECC.h"

// ************************************************************************************************
// Function which calculates the number of additional bits needed for a given number of data bits
// to store the hamming code and parity bit for a SECDED implementation
unsigned ECC::GetNumParityBits(unsigned nDataBits)
{
    unsigned nParityBits = 0;

    // Function to calculate the nube of bits: n = 2^k - k - 1
    // ( Source: Hacker's Delight; p. 310; math. function 1 )
    while (nDataBits > ((1 << nParityBits) - nParityBits - 1)) {
        ++nParityBits;
    }

    return nParityBits + 1; // +1 for the parity bit
}


// ************************************************************************************************
// Function which extends a given data word to the needed length for a SECDED code
void ECC::ExtendWord(CWord &v)
{
    unsigned end = v.GetLength() + ECC::GetNumParityBits(v.GetLength());

    // Insert x bits for the hamming code at positions where pos = 2^a; a = [0..N]
    // In "Hacker's Delight" the smallest index is 1 - But in this beautiful C-Code it's 0 as it
    // should be. That's why there is a '-1' in the call of v.Insert.
    unsigned i = 1;
    while (i < end) {
        v.Insert(i - 1, CBit());
        i <<= 1;
    }

    // Append one bit for the parity
    v.Append(CBit());
}

// ************************************************************************************************
// Function which calculates the Hamming Code bits of an extended Data word.
// Function ExtendWord must be called before calling this function
// The calculated bits are stored in p, so the length of p should be at least
// 'GetNumParityBits(#data bits)-1'
void ECC::CalculateCheckbits(CWord &v, CWord &p)
{
    unsigned i = 1, l = 0;

    // Last bit is the parity bit - don't use this in the algorithm for hamming code
    unsigned len = v.GetLength() - 1;

    // Following Graph should show you the behaviour of this algorithm
    // #Data bits: 11 #Hamming bits: 4 -> SECDED bits: 16 (incl. parity bit)
    // Hamming Code Bit: | Bits used -> data(X)/Hamming Code(H) // Bit unused -
    //                 0 | H-X-X-X-X-X-X-X
    //                 1 | -HX--XX--XX--XX
    //                 2 | ---HXXX----XXXX
    //                 3 | -------HXXXXXXX
    // For further information read "Hacker's delight" chapter 15
    // ATTENTION: The order of indices is different from the one in the book,
    //            but it doesn't matter in which order your data or check bits are.
    //            But it should be the same for encoding and decoding
    while (i < len) {
        for (unsigned j = (i - 1); j < len; j += (i << 1)) {
            for (unsigned k = 0; k < (i); k++) {
                if (j + k >= len)
                    break;
                p[l] ^= v[j + k];
            }
        }
        l++;
        i <<= 1;
    }
}

// ************************************************************************************************
// Function which inserts the checkbits which were calculated with 'CalculateCheckbits' in the
// extended data word. This is needed to calculate a proper parity of ALL bits to achive a SECDED
// behaviour.
void ECC::InsertCheckbits(CWord &v, CWord p)
{
    unsigned i = 1, j = 0;
    while (i <= v.GetLength() - 1) {
        v[i - 1] = p[j++];
        i <<= 1;
    }
}


// ************************************************************************************************
// Function which extracts the checkbits out of an extended data word. This is needed to check for
// bit error in the data word.
void ECC::ExtractCheckbits(CWord v, CWord &p)
{
    unsigned i = 1, j = 0;
    while (i <= v.GetLength() - 1) {
        p[j++] = v[i - 1];
        i <<= 1;
    }
}

// ************************************************************************************************
// Function which calculates the overal parity
// Simply XOR all bits
void ECC::CalculateParityBit(CWord v, CBit &p)
{
    // Paritybit
    p = CBit::ZERO;
    for (unsigned i = 0; i < v.GetLength(); i++) {
        p ^= v[i];
    }
}

// ************************************************************************************************
// Function to insert the parity bit into the extended data word
void ECC::InsertParityBit(CWord &v, CBit p)
{
    v[v.GetLength() - 1] = p;
}

// ************************************************************************************************
// Function to extract the parity bit out of an extended data word.
void ECC::ExtractParityBit(CWord v, CBit &p)
{
    p = v[v.GetLength() - 1];
}
