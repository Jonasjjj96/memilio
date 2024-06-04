#pragma once

#include "memilio/io/io.h"
#include "memilio/utils/compiler_diagnostics.h"
#include "memilio/utils/metaprogramming.h"

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace mio
{

// template <class... Targets>
// using targets_t = std::tuple<Targets&...>;
using names_t = std::pair<std::string_view, const std::vector<std::string_view>>;

template <class T>
using get_serialization_targets_expr =
    details::tuple_size_value_t<decltype(std::declval<T>().get_serialization_targets())>;

template <class T>
using get_serialization_names_expr = decltype(std::declval<names_t>() == T::get_serialization_names());

template <class T>
constexpr bool is_auto_serializable_v = is_expression_valid<get_serialization_targets_expr, const T>::value &&
                                        is_expression_valid<get_serialization_names_expr, T>::value;

// do not accept rvalues. just repackage lvalue refs as const into a tuple. make sure this is not dangling in tests!
template <class... Targets>
inline std::tuple<Targets const&...> make_auto_serialization_targets(Targets&... member_variables)
{
    return std::tie(member_variables...);
}

inline names_t make_auto_serialization_names(std::string_view type_name, std::vector<std::string_view> target_names)
{
    return std::make_pair(type_name, target_names);
}

template <class IOContext, class... Targets>
void auto_serialize(IOContext& io, const names_t& serialization_names,
                    const std::tuple<Targets const&...>& serialization_targets)
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
                                            Tag<std::tuple<Targets const&...>> tag,
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

template <class T, class... S>
using list_initialization_expr = decltype(T{std::declval<S>()...});

template <class T, class... S>
using is_list_initializable = is_expression_valid<list_initialization_expr, T, S...>;

template <typename Struct, typename... T>
constexpr bool is_list_initializable_v = is_list_initializable<Struct, T...>::value;

template <class Type, class IOContext, class... Targets>
IOResult<Type> auto_deserialize(IOContext& io, const names_t& serialization_names,
                                Tag<std::tuple<Targets const&...>> tag)
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

/**
 * serialize an Eigen matrix expression.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam M the type of Eigen matrix expression to be deserialized.
 * @param io an IO context.
 * @param mat the matrix expression to be serialized.
 */
template <
    class IOContext, class AutoSerializable,
    std::enable_if_t<is_auto_serializable_v<AutoSerializable> && !has_serialize<IOContext, AutoSerializable>::value,
                     void*> = nullptr>
void serialize_internal(IOContext& io, const AutoSerializable& t)
{
    auto_serialize(io, t.get_serialization_names(), t.get_serialization_targets());
}

/**
 * deserialize a tuple-like object, e.g. std::tuple or std::pair.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam Tup the tuple-like type to be deserialized, i.e. anything that supports tuple_size and tuple_element.
 * @param io an IO context.
 * @param tag define the type of the object to be deserialized.
 * @return a restored tuple
 */
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
