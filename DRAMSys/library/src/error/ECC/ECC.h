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

#pragma once

#include "Word.h"
#include "Bit.h"

// ************************************************************************************************
//
// | __/ __/ __| | __|  _ _ _  __| |_(_)___ _ _  ___
// | _| (_| (__  | _| || | ' \/ _|  _| / _ \ ' \(_-<
// |___\___\___| |_| \_,_|_||_\__|\__|_\___/_||_/__/
//
// ------------------------------------------------------------------------------------------------
// ECC Implementation as described in "Hacker's Delight - second Edition" by Henry S. Warren, Jr.
// ------------------------------------------------------------------------------------------------
// This namespace gives you some handy functions to get a SEC-DED implementation.
//
// SEC-DED: Single Error Correction - Double Error Detection
// This is the most common error correction code (ECC).
// It consists of two different correction codes: Haming code + parity code.
//
// For further details read chapter 15 of "Hacker's Delight - second Edition"
// ************************************************************************************************

namespace ECC {
unsigned GetNumParityBits(unsigned nDataBits);

// Extends the data word that it can be used with hamming code
// Several bits will be included at specific places
void ExtendWord(CWord &v);

void CalculateCheckbits(CWord &v, CWord &p);
void InsertCheckbits(CWord &v, CWord p);
void ExtractCheckbits(CWord v, CWord &p);

void CalculateParityBit(CWord v, CBit &p);
void InsertParityBit(CWord &v, CBit p);
void ExtractParityBit(CWord v, CBit &p);
}
