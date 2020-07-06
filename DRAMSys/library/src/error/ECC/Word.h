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

#include <deque>
#include "Bit.h"

class CWord
{
protected:

    unsigned m_nBitLength;
    std::deque<CBit> m_word;


public:
    CWord(unsigned nBitLength);
    virtual ~CWord();

    CBit *GetAt(unsigned nBitPos);

    void Set(unsigned data);
    void Set(const unsigned char *data, unsigned lengthInBits);
    void Rotate();

    bool Insert(unsigned npos, CBit b);
    bool Delete(unsigned npos);

    void Copy(unsigned char *ptr);

    void Append(CBit b);

    void Resize(unsigned nsize);

    bool PartShiftRight(unsigned nPos, unsigned nShift);

    inline unsigned GetLength() const
    {
        return m_nBitLength;
    };

    void Print();

    CWord &operator=(unsigned d)
    {
        Set(d);
        return *this;
    }

    CBit &operator[](unsigned nPos)
    {
        return m_word.at(nPos);
    }

    friend CWord operator >> (CWord l, const unsigned &r)
    {
        for (unsigned i = 0; i < r; i++) {
            l.m_word.pop_front();
            l.m_word.push_back(CBit(CBit::VALUE::ZERO));
        }
        return l;
    }
};

