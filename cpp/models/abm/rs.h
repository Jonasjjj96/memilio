#pragma once

#include "memilio/compartments/flow_model.h"
#include "memilio/io/io.h"
#include "memilio/utils/compiler_diagnostics.h"
#include "memilio/utils/metaprogramming.h"
#include "memilio/utils/type_list.h"
#include "memilio/utils/type_safe.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <libs/type_traits/include/boost/type_traits/declval.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace mio
{

// class RecursiveSerializer
// {
//     template <class T>
//     using is_iterable_expr = decltype(make_range(std::declval<T>().begin(), std::declval<T>().end()));

//     template <class T>
//     static constexpr bool is_iterable_v = is_expression_valid<is_iterable_expr, T>::value;

// public:
//     RecursiveSerializer(std::string name, const std::vector<std::string>& member_names)
//         : m_name(name)
//         , m_member_names(member_names)
//     {
//     }

// private:
//     std::string m_name;
//     std::vector<std::string> m_member_names;

//     // using Pairs = std::tuple<Pair<Members>...>;
//     // Pairs m_members;

//     //entry to recursively deserialize all parameters in the ParameterSet
//     //IOContext: serializer
//     //IOObject: object that stores the serialized ParameterSet
//     //Rs: IOResult<T> for each Parameter Tag that has already been deserialized
//     template <class Target, size_t Index = 0, class IOContext, class IOObject, class... Objects, class F, class Head,
//               class... Tail>
//     inline IOResult<Target> deserialize_impl(IOContext& io, IOObject& obj, Objects&&... objects, F&& f,
//                                              TypeList<Head, Tail...>)
//     {

//         const size_t index = m_member_names.size() - sizeof...(Tail) - 1;

//         IOResult<Head> object;
//         if constexpr (is_iterable_v<Head>) {
//             object = obj.expect_list(m_member_names[index], Tag<Head>{});
//         }
//         else {
//             object = obj.expect_object(m_member_names[index], Tag<Head>{});
//         }

//         if constexpr (sizeof...(Tail) > 0) {
//             return deserialize_impl(io, obj, std::forward<Objects>(objects)..., std::move(object), TypeList<Tail...>{});
//         }
//         else {
//             return mio::apply(io, f, std::forward<Objects>(objects)...);
//         }
//     }

//     template <class IOObject, class Head, class... Tail>
//     inline void serialize_impl(IOObject& obj, Head member, Tail... others) const
//     {
//         const size_t index = m_member_names.size() - sizeof...(Tail) - 1;

//         if constexpr (is_iterable_v<Head>) {
//             obj.add_list(m_member_names[index], member.begin(), member.end());
//         }
//         else {
//             obj.add_element(m_member_names[index], member);
//         }

//         if constexpr (sizeof...(Tail) > 0) {
//             serialize_impl(obj, others...);
//         }
//     }

// public:
//     /**
//      * serialize this.
//      * @see mio::serialize
//      */
//     template <class IOContext, class... Members>
//     void serialize(IOContext& io, const Members&... members) const
//     {
//         assert(m_member_names.size() == sizeof...(Members));
//         auto obj = io.create_object(m_name);
//         serialize_impl(obj, members...);
//     }

//     template <class IOContext, class... Members>
//     void serialize(IOContext& io, const std::tuple<std::reference_wrapper<Members>...>& members) const
//     {
//         assert(m_member_names.size() == sizeof...(Members));
//         auto obj = io.create_object(m_name);

//         auto add_elements = [&obj, this](const Members&... ms) {
//             size_t i = 0;
//             ((obj.add_element(m_member_names[i], ms), ++i), ...);
//         };

//         std::apply(add_elements, members);
//     }

//     /**
//      * deserialize an object of this class.
//      * @see mio::deserialize
//      */
//     template <class Target, class... Members, class IOContext>
//     IOResult<Target> deserialize(
//         IOContext& io, std::function<Target(Members&&...)>&& f = [](Members&&... ms) {
//             return Target{ms...};
//         })
//     {
//         assert(m_member_names.size() == sizeof...(Members));
//         auto obj = io.expect_object(m_name);
//         return deserialize_impl<Target>(io, obj, f, TypeList<Members...>{});
//     }
// };

template <class... Targets>
inline std::tuple<std::reference_wrapper<const Targets>...>
make_auto_serialization_targets(const Targets&... member_variables)
{
    return std::make_tuple(std::cref(member_variables)...);
}

inline std::pair<std::string_view, const std::vector<std::string_view>>
make_auto_serialization_names(std::string_view type_name, std::vector<std::string_view> target_names)
{
    return std::make_pair(type_name, target_names);
}

template <class IOContext, class... Targets>
void auto_serialize(IOContext& io,
                    const std::pair<std::string_view, const std::vector<std::string_view>>& serialization_names,
                    const std::tuple<std::reference_wrapper<const Targets>...>& serialization_targets)
{
    assert(serialization_names.second.size() == sizeof...(Targets));

    auto obj = io.create_object(std::string{serialization_names.first});

    auto add_elements = [&obj, &serialization_names](const Targets&... ts) {
        size_t i = 0;
        ((obj.add_element(std::string{serialization_names.second[i]}, ts), ++i), ...);
    };

    std::apply(add_elements, serialization_targets);
}

template <class Type, class IOContext, class... Targets, size_t... Indices>
IOResult<Type>
auto_deserialize(IOContext& io,
                 const std::pair<std::string_view, const std::vector<std::string_view>>& serialization_names,
                 Tag<std::tuple<std::reference_wrapper<const Targets>...>> tag,
                 std::index_sequence<Indices...> index_sequence = std::make_index_sequence<sizeof...(Targets)>{})
{
    mio::unused(tag, index_sequence);
    static_assert(sizeof...(Indices) == sizeof...(Targets), "Use the default value for the index_sequence.");
    assert(serialization_names.second.size() == sizeof...(Targets));

    auto obj = io.expect_object(serialization_names.first);

    // size_t i = 0;
    // auto targets = std::make_tuple(IOResult<Targets>{obj.expect_element(serialization_names.second[++i], Tag<Targets>{})}...);

    apply(
        io,
        [](Targets&&... ts) {
            return Type{ts...};
        },
        IOResult<Targets>{obj.expect_element(serialization_names.second[Indices], Tag<Targets>{})}...);
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

template <class T>
using get_serialization_targets_expr =
    details::tuple_size_value_t<decltype(std::declval<T>().get_serialization_targets())>;

template <class T>
using get_serialization_names_expr =
    decltype(std::declval<std::pair<std::string_view, const std::vector<std::string_view>>>() ==
             T::get_serialization_names());

template <class T>
constexpr bool is_auto_serializable_v = is_expression_valid<get_serialization_targets_expr, T>::value &&
                                        is_expression_valid<get_serialization_names_expr, T>::value;

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
