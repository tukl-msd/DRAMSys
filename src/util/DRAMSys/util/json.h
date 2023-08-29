/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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
 *    Thomas Psota
 *    Derek Christ
 */

#ifndef DRAMSYS_UTIL_JSON_H
#define DRAMSYS_UTIL_JSON_H

#include "nlohmann/json.hpp"

#include <optional>
#include <string>
#include <variant>

using json_t = nlohmann::json;

namespace DRAMSys::util
{
// See https://www.kdab.com/jsonify-with-nlohmann-json/
// Try to set the value of type T into the variant data if it fails, do nothing
template <typename T, typename... Ts>
void variant_from_json(const nlohmann::json& j, std::variant<Ts...>& data)
{
    try
    {
        data = j.get<T>();
    }
    catch (...)
    {
    }
}

template <typename T>
void optional_to_json(nlohmann::json& j, std::string_view name, const std::optional<T>& value)
{
    if (value)
        j[name] = *value;
}

template <class T>
void optional_from_json(const nlohmann::json& j, std::string_view name, std::optional<T>& value)
{
    const auto it = j.find(name);

    if (it != j.end())
        value = it->get<T>();
    else
        value = std::nullopt;
}

template <typename> constexpr bool is_optional = false;
template <typename T> constexpr bool is_optional<std::optional<T>> = true;

template <typename T> void extended_to_json(const char* key, nlohmann::json& j, const T& value)
{
    if constexpr (is_optional<T>)
        optional_to_json(j, key, value);
    else
        j[key] = value;
}

template <typename T> void extended_from_json(const char* key, const nlohmann::json& j, T& value)
{
    if constexpr (is_optional<T>)
        optional_from_json(j, key, value);
    else
        j.at(key).get_to(value);
}

} // namespace DRAMSys::util

NLOHMANN_JSON_NAMESPACE_BEGIN

template <typename... Ts> struct adl_serializer<std::variant<Ts...>>
{
    static void to_json(nlohmann::json& j, const std::variant<Ts...>& data)
    {
        std::visit([&j](const auto& v) { j = v; }, data);
    }

    static void from_json(const nlohmann::json& j, std::variant<Ts...>& data)
    {
        // Call variant_from_json for all types, only one will succeed
        (DRAMSys::util::variant_from_json<Ts>(j, data), ...);
    }
};

template <typename T> struct adl_serializer<std::optional<T>>
{
    static void to_json(json_t& j, const std::optional<T>& opt)
    {
        if (opt == std::nullopt)
        {
            j = nullptr;
        }
        else
        {
            j = *opt;
        }
    }

    static void from_json(const json_t& j, std::optional<T>& opt)
    {
        if (j.is_null())
        {
            opt = std::nullopt;
        }
        else
        {
            opt = j.get<T>();
        }
    }
};

NLOHMANN_JSON_NAMESPACE_END

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define EXTEND_JSON_TO(v1)                                                                         \
    DRAMSys::util::extended_to_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);
#define EXTEND_JSON_FROM(v1)                                                                       \
    DRAMSys::util::extended_from_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);

#define NLOHMANN_JSONIFY_ALL_THINGS(Type, ...)                                                     \
    inline void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t)              \
    {                                                                                              \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_TO, __VA_ARGS__))                     \
    }                                                                                              \
    inline void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t)            \
    {                                                                                              \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_FROM, __VA_ARGS__))                   \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // DRAMSYS_UTIL_JSON_H
