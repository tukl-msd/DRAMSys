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

#include "Word.h"

#include <math.h>
#include <iostream>
#include <string.h>

using std::cout;
using std::deque;

CWord::CWord(unsigned nBitLength)
    : m_nBitLength(nBitLength)
{
    m_word.resize(nBitLength);
}


CWord::~CWord()
{
}

CBit *CWord::GetAt(unsigned nBitPos)
{
    if (nBitPos < m_nBitLength) {
        return &m_word.at(nBitPos);
    }

    return nullptr;
}

void CWord::Set(unsigned data)
{
    deque<CBit>::iterator it;
    if (m_nBitLength < sizeof(data)) {
        it = m_word.begin();
        for (unsigned i = 0; i < m_nBitLength; i++) {
            (*it++) = data & 1;
            data >>= 1;
        }
    } else {
        for (it = m_word.begin(); it != m_word.end(); it++) {
            (*it) = data & 1;
            data >>= 1;
        }
    }
}

void CWord::Set(const unsigned char *data, unsigned lengthInBits)
{
    deque<CBit>::iterator it;
    if (m_nBitLength < lengthInBits) {
        it = m_word.begin();
        for (unsigned pos = 0; pos < m_nBitLength; pos++) {
            (*it) = data[pos >> 3] & (1 << (7 - (pos & 7)));
            it++;
        }
    } else {
        unsigned pos = 0;
        for (it = m_word.begin(); it != m_word.end(); it++) {
            (*it) = data[pos >> 3] & (1 << (7 - (pos & 7)));
            ++pos;
        }
    }
}

void CWord::Rotate()
{
    deque<CBit> buffer = m_word;
    for (unsigned i = 0; i < m_nBitLength; i++) {
        m_word.at(m_nBitLength - i - 1) = buffer.at(i);
    }
}

bool CWord::Insert(unsigned npos, CBit b)
{
    if (npos >= m_nBitLength)
        return false;

    deque<CBit>::iterator it = m_word.begin() + npos;
    m_word.insert(it, b);

    m_nBitLength++;

    return true;
}

bool CWord::Delete(unsigned npos)
{
    if (npos >= m_nBitLength)
        return false;

    deque<CBit>::iterator it = m_word.begin() + npos;
    m_word.erase(it);

    m_nBitLength++;

    return true;
}

void CWord::Append(CBit b)
{
    m_word.push_back(b);

    m_nBitLength++;
}

void CWord::Resize(unsigned nsize)
{
    m_word.resize(nsize);
    m_nBitLength = nsize;
}

bool CWord::PartShiftRight(unsigned nPos, unsigned /*nShift*/)
{
    if (nPos >= m_nBitLength)
        return false;

    /*for (unsigned i = 0; i < nShift; i++)
    {
        m_word.insert()
    }*/

    return true;
}

void CWord::Print()
{
    deque<CBit>::iterator it;
    for (it = m_word.begin(); it != m_word.end(); it++) {
        (*it).Print();
    }
    cout << "\r\n";
}

void CWord::Copy(unsigned char *ptr)
{
    unsigned len = ceil(m_word.size() / 8);
    memset(ptr, 0, len);

    unsigned pos = 0;
    for (auto it = m_word.begin(); it != m_word.end(); it++) {
        ptr[pos >> 3] |= (*it).Get() << (7 - (pos & 7));
        ++pos;
    }
}
