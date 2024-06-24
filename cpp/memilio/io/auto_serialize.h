/*
* Copyright (C) 2020-2024 MEmilio
*
* Authors: Rene Schmieding
*
* Contact: Martin J. Kuehn <Martin.Kuehn@DLR.de>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef MIO_IO_AUTO_SERIALIZE_H_
#define MIO_IO_AUTO_SERIALIZE_H_

#include "memilio/io/io.h"
#include "memilio/utils/metaprogramming.h"

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace mio
{

// TODO: check whether it's better to std::move the ~turtle~ tuple all the way down, or to use values/lvalue refs

template <class ValueType>
struct NVP {
    using Type = ValueType&;
    explicit NVP(const std::string_view n, Type v)
        : name(n)
        , value(v)
    {
    }
    const std::string_view name;
    Type value;

    NVP()                      = delete;
    NVP(const NVP&)            = delete;
    NVP(NVP&&)                 = default;
    NVP& operator=(const NVP&) = delete;
    NVP& operator=(NVP&&)      = delete;
};

// package names and member references together. always take return type by value!
template <class... Targets>
[[nodiscard]] inline auto make_auto_serialization(const std::string_view type_name, NVP<Targets>... member_variables)
{
    return std::make_pair(type_name, std::make_tuple(std::move(member_variables)...));
}

namespace details
{
template <class T>
using auto_serialize_expr = decltype(std::declval<T>().auto_serialize());

template <class IOObject, class Target>
void add_nvp(IOObject& obj, NVP<Target> const&& nvp)
{
    obj.add_element(std::string{nvp.name}, nvp.value);
}

template <class IOContext, class... Targets>
void auto_serialize_impl(IOContext& io, const std::string_view name, std::tuple<NVP<Targets>...> const&& targets)
{
    auto obj = io.create_object(std::string{name});

    std::apply(
        [&obj](NVP<Targets> const&&... nvps) {
            (add_nvp(obj, std::move(nvps)), ...);
        },
        std::move(targets));
}

template <class IOObject, class Target>
IOResult<Target> expect_nvp(IOObject& obj, NVP<Target>&& nvp)
{
    return obj.expect_element(std::string{nvp.name}, Tag<Target>{});
}

template <class IOContext, class AutoSerializable, class... Targets>
IOResult<AutoSerializable> auto_deserialize_impl(IOContext& io, AutoSerializable& a, std::string_view name,
                                                 std::tuple<NVP<Targets>...>&& targets)
{
    auto obj = io.expect_object(std::string{name});

    auto unpacked_apply = [&io, &a, &obj](NVP<Targets>... nvps) {
        return apply(
            io,
            [&a, &nvps...](const Targets&... values) {
                ((nvps.value = values), ...);
                return a;
            },
            expect_nvp(obj, std::move(nvps))...);
    };

    return std::apply(unpacked_apply, std::move(targets));
}

} // namespace details

template <class AutoSerializable>
struct AutoConstructor : public AutoSerializable {
    using AutoSerializable::AutoSerializable;
};

// whether T has a auto_serialize member
template <class T>
using has_auto_serialize = is_expression_valid<details::auto_serialize_expr, T>;

// disables itself if a deserialize member is present or if there is no auto_serialize member
// generates serialize method depending on NVPs given by auto_serialize
template <class IOContext, class AutoSerializable,
          std::enable_if_t<has_auto_serialize<AutoSerializable>::value &&
                               not has_serialize<IOContext, AutoSerializable>::value,
                           AutoSerializable*> = nullptr>
void serialize_internal(IOContext& io, const AutoSerializable& t)
{
    // Note that this cast is only safe if we do not modify targets.
    const auto targets = const_cast<AutoSerializable*>(&t)->auto_serialize();
    details::auto_serialize_impl(io, targets.first, std::move(targets.second));
}

// disables itself if a deserialize member is present or if there is no auto_serialize member
// generates deserialize method depending on NVPs given by auto_serialize
template <class IOContext, class AutoSerializable,
          std::enable_if_t<has_auto_serialize<AutoSerializable>::value &&
                               not has_deserialize<IOContext, AutoSerializable>::value,
                           AutoSerializable*> = nullptr>
IOResult<AutoSerializable> deserialize_internal(IOContext& io, Tag<AutoSerializable>)
{
    static_assert(std::is_default_constructible_v<AutoConstructor<AutoSerializable>>,
                  "Automatic deserialization requires a default constructor.");
    // the if constexpr is used to hide compilation errors that occur when the above static assertion fails,
    // hence the else block is effectively unreachable
    if constexpr (std::is_default_constructible_v<AutoConstructor<AutoSerializable>>) {
        AutoSerializable a = AutoConstructor<AutoSerializable>{};
        auto targets       = a.auto_serialize();
        return details::auto_deserialize_impl(io, a, targets.first, std::move(targets.second));
    }
    else {
        return mio::success();
    }
}

} // namespace mio

#endif // MIO_IO_AUTO_SERIALIZE_H_
