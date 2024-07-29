//! \file type.hpp
//! \brief Basic types and utilities.

#ifndef UAT_TYPE_HPP
#define UAT_TYPE_HPP

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <functional>

namespace uat
{

//! Concept for types that are hashable.
template <typename T>
concept hashable = requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

//! Concept for types that are compatible as regions.
template <typename R>
concept region_compatible = hashable<R> && std::equality_comparable<R> && std::copyable<R>;

//! Default unsigned integer type
using uint_t = std::size_t;

//! Default type for identifier
using id_t = std::size_t;

//! Default type for price
using value_t = double;

} // namespace uat

#endif // UAT_TYPE_HPP
