/*
 * Copyright (c) 2026, Fraunhofer IESE
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
 *    Thomas Zimmermann
 */

#pragma once

#include <limits>
#include <cassert>
#include <type_traits>

namespace DRAMSys::Initiators
{

template <class IntType = int>
struct uniform_int_distribution
{
    using result_type = IntType;

    const result_type A, B;

    explicit uniform_int_distribution(const result_type a = 0, const result_type b = std::numeric_limits<result_type>::max())
     : A(a), B(b)
    {}

    template <class Generator>
    result_type operator()(Generator& g) const
    {
        return rnd(g, A, B);
    }
    result_type min() const { return A;}
    result_type max() const { return B; }

private:
    template <class Generator>
    result_type rnd(Generator& g, const result_type a, const result_type b) const
    {
        static_assert(std::is_convertible<typename Generator::result_type, result_type>::value, "Generator result_type must be convertible to result_type");

        const result_type gen_min = g.min();
        const result_type gen_max = g.max();
        const result_type gen_range = gen_max - gen_min;
        const result_type range = b - a + 1;
        assert(gen_range >= range - 1);

        const result_type reject_lim = gen_range % range;
        result_type n;
        do
        {
            n = g() - gen_min;
        }
        while (n <= reject_lim);
        return (n % range) + a;
    }
};

template <class RealType = double>
struct uniform_real_distribution
{
    using result_type = RealType;

    const result_type A, B;

    explicit uniform_real_distribution(const result_type a = 0.0, const result_type b = 1.0)
     : A(a), B(b)
    {}

    template <class Generator>
    result_type operator()(Generator& g) const
    {
        return rnd(g, A, B);
    }
    result_type min() const { return A; }
    result_type max() const { return B; }

private:
    template <class Generator>
    result_type rnd(Generator& g, const result_type a, const result_type b) const
    {
        static_assert(std::is_floating_point<result_type>::value, "uniform_real_distribution requires a floating point type");

        const result_type unit = static_cast<result_type>(g()) / static_cast<result_type>(g.max());
        return a + unit * (b - a);
    }
};

} // namespace DRAMSys::Initiators