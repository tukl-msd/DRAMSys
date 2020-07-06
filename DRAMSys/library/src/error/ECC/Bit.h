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
class CBit
{
public:
    enum VALUE {
        ZERO = 0,
        ONE = 1
    };

protected:
    VALUE m_nValue;

public:
    CBit(VALUE nVal = ZERO);
    virtual ~CBit();

    inline void Set()
    {
        m_nValue = ONE;
    };
    inline void Clear()
    {
        m_nValue = ZERO;
    };
    inline unsigned Get()
    {
        if (m_nValue == ONE)
            return 1;
        else
            return 0;
    };

    void Print();

    CBit &operator=(unsigned d)
    {
        if (d == 0 ) {
            m_nValue = ZERO;
        } else {
            m_nValue = ONE;
        }
        return *this;
    }

    friend CBit operator^(CBit l, const CBit &r)
    {
        if (l.m_nValue == r.m_nValue) {
            return CBit(ZERO);
        } else {
            return CBit(ONE);
        }
    }

    CBit &operator^=(const CBit &r)
    {
        if (m_nValue == r.m_nValue) {
            m_nValue = ZERO;
        } else {
            m_nValue = ONE;
        }
        return *this;
    }

    inline bool operator==(const CBit::VALUE &r)
    {
        return m_nValue == r;
    }
};

