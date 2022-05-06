/*
 * Copyright (c) 2021, Technische Universität Kaiserslautern
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
 *    Derek Christ
 */

#ifndef DRAMSYSCONFIGURATION_UTIL_H
#define DRAMSYSCONFIGURATION_UTIL_H

#include <nlohmann/json.hpp>
#include <optional>
#include <utility>

namespace DRAMSysConfiguration
{
using json = nlohmann::json;

// template <typename T>
// class Optional : public std::pair<T, bool>
// {
// public:
//     Optional() : std::pair<T, bool>{{}, false}
//     {
//     }
//     Optional(const T &v) : std::pair<T, bool>{v, true}
//     {
//     }

//     bool isValid() const
//     {
//         return this->second;
//     }

//     const T &getValue() const
//     {
//         assert(this->second == true);
//         return this->first;
//     }

//     void setValue(const T &v)
//     {
//         this->first = v;
//         this->second = true;
//     }

//     void invalidate()
//     {
//         this->second = false;
//     }

//     /**
//      * This methods only purpose is to make a optional type
//      * valid so that it can be written to by reference.
//      */
//     T &setByReference()
//     {
//         this->second = true;
//         return this->first;
//     }
// };

template <typename T>
void invalidateEnum(T &value)
{
    if (value.has_value() && value.value() == T::value_type::Invalid)
        value.reset();
}

json get_config_json(const json &j, const std::string &configPath, const std::string &objectName);

inline void remove_null_values(json &j)
{
    std::vector<std::string> keysToRemove;

    for (const auto &element : j.items())
    {
        if (element.value() == nullptr)
            keysToRemove.emplace_back(element.key());
    }

    std::for_each(keysToRemove.begin(), keysToRemove.end(), [&](const std::string &key) { j.erase(key); });
}

} // namespace DRAMSysConfiguration

/**
 * Inspired from
 * https://json.nlohmann.me/features/arbitrary_types/#how-can-i-use-get-for-non-default-constructiblenon-copyable-types
 */
namespace nlohmann
{

template <typename T>
void to_json(nlohmann::json &j, const std::optional<T> &v)
{
    if (v.has_value())
        j = v.value();
    else
        j = nullptr;
}

template <typename T>
void from_json(const nlohmann::json &j, std::optional<T> &v)
{
    if (j.is_null())
        v = std::nullopt;
    else
    {
        v = j.get<T>();
    }
}

} // namespace nlohmann

#endif // DRAMSYSCONFIGURATION_UTIL_H
