#pragma once

#include "memilio/io/io.h"
#include "memilio/utils/compiler_diagnostics.h"
#include "memilio/utils/metaprogramming.h"

#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace mio
{

// template <class... Targets>
// using targets_t = std::tuple<Targets&...>;
using names_t = std::pair<std::string_view, const std::vector<std::string_view>>;

template <class T>
using auto_serialize_expr = decltype(std::declval<T>().auto_serialize());

template <class T>
constexpr bool has_auto_serialize_v = is_expression_valid<auto_serialize_expr, T>::value;

template <class T>
using get_serialization_targets_expr =
    details::tuple_size_value_t<decltype(std::declval<T>().get_serialization_targets())>;

template <class T>
using get_serialization_names_expr = decltype(std::declval<names_t>() == T::get_serialization_names());

template <class T>
constexpr bool is_auto_serializable_v =
    !has_auto_serialize_v<T> && is_expression_valid<get_serialization_targets_expr, T>::value &&
    is_expression_valid<get_serialization_names_expr, T>::value;

// template <class IOContext>
// class AutoSerializerObject : public decltype(std::declval<IOContext>().create_object(std::declval<std::string>()))
// {
//     // TODO: add a IOContext::ObjectType?
// public:
//     template <class TargetType>
//     using NVP = std::pair<std::string_view, TargetType const&>;

//     template <class TargetType>
//     AutoSerializerObject& operator&(NVP<TargetType> serialization_target)
//     {
//         this->add_element(std::string(serialization_target.first), serialization_target.second);
//     }

//     void finalize(){};
// };

// template <class IOContext>
// class AutoSerializer : public IOContext
// {
//     AutoSerializerObject<IOContext> create_object(const std::string& type)
//     {
//         return this->create_object(type);
//     }
// };

// template <class IOContext>
// class AutoDeserializerObject : decltype(std::declval<IOContext>().create_object(std::declval<std::string>()))
// {
// public:
//     template <class TargetType>
//     AutoDeserializerObject& operator<<(std::pair<std::string_view, TargetType&> serialization_target)
//     {
//         obj.add_element(std::string(serialization_target.first), serialization_target.second);
//     }

//     A

//         private : Type obj;
// };

// template <class IOContext>
// class AutoDeserializer
// {
// public:
//     AutoDeserializerObject<IOContext> create_object(const std::string& object_name)
//     {
//         return io.create_object(object_name);
//     }

// private:
//     IOContext& io;
// };

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
};

template <class... Targets>
inline auto make_auto_serialization(const std::string_view type_name, NVP<Targets>... member_variables)
{
    // return std::tie(type_name, member_variables...);
    return std::make_pair(type_name, std::make_tuple(member_variables...));
}

// do not accept rvalues. just repackage lvalue refs as const into a tuple. make sure this is not dangling in tests!
template <class... Targets>
inline std::tuple<Targets&...> make_auto_serialization_targets(Targets&... member_variables)
{
    return std::tie(member_variables...);
}

inline names_t make_auto_serialization_names(std::string_view type_name, std::vector<std::string_view> target_names)
{
    return std::make_pair(type_name, target_names);
}

template <class IOContext, class... Targets>
void auto_serialize(IOContext& io, const names_t& serialization_names,
                    const std::tuple<Targets&...>& serialization_targets)
{
    assert(serialization_names.second.size() == sizeof...(Targets));

    auto obj = io.create_object(std::string{serialization_names.first});

    auto add_elements = [&obj, &serialization_names](Targets const&... ts) {
        size_t i = 0;
        ((obj.add_element(std::string{serialization_names.second[i]}, ts), ++i), ...);
    };

    std::apply(add_elements, serialization_targets);
}

template <class Type, class IOContext, class... Targets, size_t... Indices>
IOResult<Type> inline auto_deserialize_impl(IOContext& io, const names_t& serialization_names,
                                            Tag<std::tuple<Targets&...>> tag,
                                            std::index_sequence<Indices...> index_sequence)
{
    mio::unused(tag, index_sequence);
    static_assert(sizeof...(Indices) == sizeof...(Targets), "Wrong Index set.");

    auto obj = io.expect_object(std::string{serialization_names.first});

    return apply(
        io,
        [](Targets const&... ts) {
            return Type{ts...};
        },
        IOResult<Targets>{obj.expect_element(std::string{serialization_names.second[Indices]}, Tag<Targets>{})}...);
}

template <class Type, class... Args>
using list_initialization_expr = decltype(Type{std::declval<Args>()...});

template <class Type, class... Args>
using is_list_initializable = is_expression_valid<list_initialization_expr, Type, Args...>;

template <class Type, class... Args>
constexpr bool is_list_initializable_v = is_list_initializable<Type, Args...>::value;

template <class Type, class IOContext, class... Targets>
IOResult<Type> auto_deserialize(IOContext& io, const names_t& serialization_names, Tag<std::tuple<Targets&...>> tag)
{
    static_assert(std::is_constructible_v<Type, Targets const&...> || is_list_initializable_v<Type, Targets const&...>,
                  "Need a constructor using all serialization targets in order for auto_deserialize.");
    assert(serialization_names.second.size() == sizeof...(Targets));
    return auto_deserialize_impl<Type>(io, serialization_names, tag, std::make_index_sequence<sizeof...(Targets)>{});
}

// template <class... Targets>
// struct AutoSerializerTuple : TypeSafe<std::tuple<std::reference_wrapper<std::pair<Targets, std::string>>...>,
//                                       AutoSerializerTuple<Targets...>> {
//     using Base = TypeSafe<std::tuple<std::reference_wrapper<std::pair<Targets, std::string>>...>,
//                           AutoSerializerTuple<Targets...>>;
//     AutoSerializerTuple(const typename Base::ValueType& v)
//         : Base(v)
//     {
//     }
// };

// template <class... Targets>
// std::tuple<std::reference_wrapper<std::pair<Targets, std::string>>...>
// make_serialization_tuple(std::pair<const Targets&, const std::string&>... targets)
// {
//     return std::make_tuple(std::cref(targets)...);
// }

template <class F, class... Targets>
void apply_each(F&& f, const std::tuple<Targets...>& targets)
{
    std::apply(
        [&](const Targets&... ts) {
            (f(ts), ...);
        },
        targets);
}

template <class IOContext, class AutoSerializable,
          std::enable_if_t<has_auto_serialize_v<AutoSerializable> && !has_serialize<IOContext, AutoSerializable>::value,
                           void*> = nullptr>
void serialize_internal(IOContext& io, const AutoSerializable& t)
{
    // TODO: removing const here is a kind of a hack, but the correct way would be to change how serialization works...
    const auto& targets = const_cast<AutoSerializable*>(&t)->auto_serialize();

    auto obj = io.create_object(std::string{targets.first});

    apply_each(
        [&obj](auto&& nvp) {
            obj.add_element(std::string{nvp.name}, nvp.value);
        },
        targets.second);
}

template <class IOContext, class AutoSerializable, class... Targets>
auto auto_deserialize_impl(IOContext& io, AutoSerializable& a, std::string_view name,
                           std::tuple<NVP<Targets>...> targets)
{
    auto obj = io.expect_object(std::string{name});

    auto unpacked_apply = [&io, &a, &obj](const NVP<Targets>&... nvps) {
        return apply(
            io,
            [&a, &nvps...](const Targets&... values) {
                ((nvps.value = values), ...);
                return a;
            },
            IOResult<Targets>{obj.expect_element(std::string{nvps.name}, Tag<Targets>{})}...);
    };

    return std::apply(unpacked_apply, targets);
}

template <
    class IOContext, class AutoSerializable,
    std::enable_if_t<has_auto_serialize_v<AutoSerializable> && !has_deserialize<IOContext, AutoSerializable>::value,
                     void*> = nullptr>
IOResult<AutoSerializable> deserialize_internal(IOContext& io, Tag<AutoSerializable>)
{
    AutoSerializable a;
    auto targets = a.auto_serialize();
    return auto_deserialize_impl(io, a, targets.first, targets.second);
}

template <
    class IOContext, class AutoSerializable,
    std::enable_if_t<is_auto_serializable_v<AutoSerializable> && !has_serialize<IOContext, AutoSerializable>::value,
                     void*> = nullptr>
void serialize_internal(IOContext& io, const AutoSerializable& t)
{
    // TODO: removing const here is a HACK, but the correct way would be to change how serialization works...
    auto_serialize(io, t.get_serialization_names(), (const_cast<AutoSerializable*>(&t))->get_serialization_targets());
}

template <
    class IOContext, class AutoSerializable,
    std::enable_if_t<is_auto_serializable_v<AutoSerializable> && !has_deserialize<IOContext, AutoSerializable>::value,
                     void*> = nullptr>
IOResult<AutoSerializable> deserialize_internal(IOContext& io, Tag<AutoSerializable>)
{
    using Targets = decltype(std::declval<AutoSerializable>().get_serialization_targets());
    return auto_deserialize<AutoSerializable>(io, AutoSerializable::get_serialization_names(), Tag<Targets>{});
}

} // namespace mio
