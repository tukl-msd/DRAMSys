/*
 * Copyright (c) 2026, RPTU Kaiserslautern-Landau
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
 *    Marco Mörz
 */

#ifndef STANDARDMAPPING_H
#define STANDARDMAPPING_H

#include <DRAMUtils/memspec/MemSpec.h>

// Forward declares DRAMPower
#ifdef USE_DRAMPOWER
namespace DRAMPower
{

struct DDR4Types;
struct DDR5Types;
struct LPDDR4Types;
struct LPDDR5Types;

} // namespace DRAMPower
#endif

namespace DRAMSys
{

// Forward declares DRAMSys
template <typename T>
class DRAMPowerWrapper;

class MemSpecDDR3;
class CheckerDDR3;
class MemSpecDDR4;
class CheckerDDR4;
#ifdef USE_DRAMPOWER
using DRAMPowerDDR4 = DRAMPowerWrapper<DRAMPower::DDR4Types>;
#endif
#ifdef DDR5_SIM
class MemSpecDDR5;
class CheckerDDR5;
#ifdef USE_DRAMPOWER
using DRAMPowerDDR5 = DRAMPowerWrapper<DRAMPower::DDR5Types>;
#endif
#endif
class MemSpecLPDDR4;
class CheckerLPDDR4;
#ifdef USE_DRAMPOWER
using DRAMPowerLPDDR4 = DRAMPowerWrapper<DRAMPower::LPDDR4Types>;
#endif
#ifdef LPDDR5_SIM
class MemSpecLPDDR5;
class CheckerLPDDR5;
#ifdef USE_DRAMPOWER
using DRAMPowerLPDDR5 = DRAMPowerWrapper<DRAMPower::LPDDR5Types>;
#endif
#endif
#ifdef LPDDR6_SIM
class MemSpecLPDDR6;
class CheckerLPDDR6;
#endif
class MemSpecWideIO;
class CheckerWideIO;
class MemSpecWideIO2;
class CheckerWideIO2;
class MemSpecHBM2;
class CheckerHBM2;
#ifdef HBM3_4_SIM
class MemSpecHBM3_4;
class CheckerHBM3_4;
#endif
class MemSpecGDDR5;
class CheckerGDDR5;
class MemSpecGDDR5X;
class CheckerGDDR5X;
class MemSpecGDDR6;
class CheckerGDDR6;
class MemSpecSTTMRAM;
class CheckerSTTMRAM;

namespace StandardMapping
{

template <typename T>
struct Mapping
{
};

// Specialization for DDR3
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecDDR3>
{
    using CheckerType = CheckerDDR3;
    using MemSpecType = MemSpecDDR3;
};

// Specialization for DDR4
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecDDR4>
{
    using CheckerType = CheckerDDR4;
    using MemSpecType = MemSpecDDR4;
#ifdef USE_DRAMPOWER
    using PowerType = DRAMPowerDDR4;
#endif
};

#ifdef DDR5_SIM
// Specialization for DDR5
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecDDR5>
{
    using CheckerType = CheckerDDR5;
    using MemSpecType = MemSpecDDR5;
#ifdef USE_DRAMPOWER
    using PowerType = DRAMPowerDDR5;
#endif
};
#endif

// Specialization for LPDDR4
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecLPDDR4>
{
    using MemSpecType = MemSpecLPDDR4;
    using CheckerType = CheckerLPDDR4;
#ifdef USE_DRAMPOWER
    using PowerType = DRAMPowerLPDDR4;
#endif
};

#ifdef LPDDR5_SIM
// Specialization for LPDDR5
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecLPDDR5>
{
    using MemSpecType = MemSpecLPDDR5;
    using CheckerType = CheckerLPDDR5;
#ifdef USE_DRAMPOWER
    using PowerType = DRAMPowerLPDDR5;
#endif
};
#endif

#ifdef LPDDR6_SIM
// Specialization for LPDDR6
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecLPDDR6>
{
    using MemSpecType = MemSpecLPDDR6;
    using CheckerType = CheckerLPDDR6;
};
#endif

// Specialization for MemSpecWideIO
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecWideIO>
{
    using MemSpecType = MemSpecWideIO;
    using CheckerType = CheckerWideIO;
};

// Specialization for MemSpecWideIO2
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecWideIO2>
{
    using MemSpecType = MemSpecWideIO2;
    using CheckerType = CheckerWideIO2;
};

// Specialization for HBM2
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecHBM2>
{
    using MemSpecType = MemSpecHBM2;
    using CheckerType = CheckerHBM2;
};

#ifdef HBM3_4_SIM
// Specialization for HBM3
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecHBM3>
{
    using MemSpecType = MemSpecHBM3_4;
    using CheckerType = CheckerHBM3_4;
};

// Specialization for HBM4
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecHBM4>
{
    using MemSpecType = MemSpecHBM3_4;
    using CheckerType = CheckerHBM3_4;
};
#endif

// Specialization for GDDR5
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecGDDR5>
{
    using MemSpecType = MemSpecGDDR5;
    using CheckerType = CheckerGDDR5;
};

// Specialization for GDDR5X
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecGDDR5X>
{
    using MemSpecType = MemSpecGDDR5X;
    using CheckerType = CheckerGDDR5X;
};

// Specialization for GDDR6
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecGDDR6>
{
    using MemSpecType = MemSpecGDDR6;
    using CheckerType = CheckerGDDR6;
};

// Specialization for STTMRAM
template <>
struct Mapping<DRAMUtils::MemSpec::MemSpecSTTMRAM>
{
    using MemSpecType = MemSpecSTTMRAM;
    using CheckerType = CheckerSTTMRAM;
};

template <typename T, typename = void>
struct has_MemSpecType : std::false_type
{
};
template <typename T>
struct has_MemSpecType<T, std::void_t<typename StandardMapping ::Mapping<T>::MemSpecType>>
    : std::true_type
{
};
template <typename T>
inline constexpr bool has_MemSpecType_v = has_MemSpecType<T>::value;

template <typename T, typename = void>
struct has_CheckerType : std::false_type
{
};
template <typename T>
struct has_CheckerType<T, std::void_t<typename StandardMapping ::Mapping<T>::CheckerType>>
    : std::true_type
{
};
template <typename T>
inline constexpr bool has_CheckerType_v = has_CheckerType<T>::value;

template <typename T, typename = void>
struct has_PowerType : std::false_type
{
};
template <typename T>
struct has_PowerType<T, std::void_t<typename StandardMapping ::Mapping<T>::PowerType>>
    : std::true_type
{
};
template <typename T>
inline constexpr bool has_PowerType_v = has_PowerType<T>::value;

} // namespace StandardMapping

} // namespace DRAMSys

#endif /* STANDARDMAPPING_H */
