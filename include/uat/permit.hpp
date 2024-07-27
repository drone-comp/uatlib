//! \file permit.hpp
//! \brief Defines the region and permit classes and related types.

#ifndef UAT_PERMIT_HPP
#define UAT_PERMIT_HPP

#include <uat/type.hpp>

#include <functional>
#include <iterator>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace uat
{

//! \private
template <typename T> using mb_adjacent_regions_t = decltype(std::declval<const T&>().adjacent_regions());

//! \private
template <typename T> using mb_hash_t = decltype(std::declval<const T&>().hash());

//! \private
template <typename T> using mb_distance_t = decltype(std::declval<const T&>().distance(std::declval<const T&>()));

//! \private
template <typename T>
using mb_heuristic_distance_t = decltype(std::declval<const T&>().heuristic_distance(std::declval<const T&>()));

//! \private
template <typename T> using mb_shortest_path_t = decltype(std::declval<const T&>().shortest_path(std::declval<const T&>()));

//! \private
template <typename T>
using mb_print_t =
  decltype(std::declval<const T&>().print(std::declval<std::function<void(std::string_view, fmt::format_args)>>()));

//! \private
template <typename T> using mb_climb_t = decltype(std::declval<const T&>().climb(std::declval<const T&>()));

//! \private
template <typename T>
using mb_turn_t = decltype(std::declval<const T&>().turn(std::declval<const T&>(), std::declval<const T&>()));

//! \brief A type-erased class that represents an atomic region in the airspace.
//!
//! The region class is a type-erased class that represents an atomic region in the
//! airspace. It is used to represent the subspaces that a permit refers to.
class region
{
  //! \private
  class region_interface
  {
  public:
    virtual ~region_interface() = default;
    virtual auto clone() const -> std::unique_ptr<region_interface> = 0;

    virtual auto adjacent_regions() const -> std::vector<region> = 0;
    virtual auto hash() const -> std::size_t = 0;
    virtual auto equals(const region_interface&) const -> bool = 0;
    virtual auto distance(const region_interface&) const -> uint_t = 0;
    virtual auto heuristic_distance(const region_interface&) const -> value_t = 0;
    virtual auto shortest_path(const region_interface&, int) const -> std::vector<region> = 0;
    virtual auto print_to(std::function<void(std::string_view, fmt::format_args)>) const -> void = 0;
    virtual auto turn(const region_interface&, const region_interface&) const -> bool = 0;
    virtual auto climb(const region_interface&) const -> bool = 0;
  };

  //! \private
  template <typename Region> class region_model : public region_interface
  {
  public:
    region_model(Region region) : region_(std::move(region)) {}
    virtual ~region_model() = default;

    auto clone() const -> std::unique_ptr<region_interface> override
    {
      return std::unique_ptr<region_interface>{new region_model(region_)};
    }

    auto adjacent_regions() const -> std::vector<region> override
    {
      if constexpr (is_detected_convertible_v<std::vector<region>, mb_adjacent_regions_t, Region>) {
        return region_.adjacent_regions();
      } else {
        auto nei = region_.adjacent_regions();
        static_assert(std::is_convertible_v<region, typename decltype(nei)::value_type>,
                      "member function Region::adjacent_regions must return a container of Region");

        std::vector<region> converted;
        converted.reserve(nei.size());
        std::move(nei.begin(), nei.end(), std::back_inserter(converted));
        return converted;
      }
    }

    auto hash() const -> std::size_t override { return region_.hash(); }

    auto equals(const region_interface& other) const -> bool override
    {
      return region_ == dynamic_cast<const region_model&>(other).region_;
    }

    auto distance(const region_interface& other) const -> uint_t override
    {
      if constexpr (is_detected_convertible_v<uint_t, mb_distance_t, Region>)
        return region_.distance(dynamic_cast<const region_model&>(other).region_);
      else
        throw not_implemented("Region::distance(Region) -> uint_t");
    }

    auto heuristic_distance(const region_interface& other) const -> value_t override
    {
      if constexpr (is_detected_convertible_v<value_t, mb_heuristic_distance_t, Region>)
        return region_.heuristic_distance(dynamic_cast<const region_model&>(other).region_);
      else
        return region_.distance(dynamic_cast<const region_model&>(other).region_);
    }

    auto shortest_path(const region_interface& other, int seed) const -> std::vector<region> override
    {
      if constexpr (is_detected_exact_v<std::vector<region>, mb_shortest_path_t, Region>)
        return region_.shortest_path(dynamic_cast<const region_model&>(other).region_, seed);
      else // TODO: container conversion
        return {};
    }

    auto print_to(std::function<void(std::string_view, fmt::format_args)> f) const -> void override
    {
      if constexpr (is_detected_exact_v<void, mb_print_t, Region>)
        region_.print(std::move(f));
      else
        f("NA", {});
    }

    auto turn(const region_interface& before, const region_interface& to) const -> bool override
    {
      if constexpr (is_detected_convertible_v<bool, mb_turn_t, Region>)
        return region_.turn(dynamic_cast<const region_model&>(before).region_, dynamic_cast<const region_model&>(to).region_);
      else
        return false;
    }

    auto climb(const region_interface& to) const -> bool override
    {
      if constexpr (is_detected_convertible_v<bool, mb_climb_t, Region>)
        return region_.climb(dynamic_cast<const region_model&>(to).region_);
      else
        return false;
    }

    auto downcast() -> Region& { return region_; }
    auto downcast() const -> const Region& { return region_; }

  private:
    Region region_;
  };

public:
  //! Constructs a type-erased region from an object of type Region that satisfy at least:
  //!
  //! - `Region::adjacent_regions() -> Container<Region>`
  //! - `Region::hash() -> convertible_to<size_t>`
  //!
  //! Moreover, it should be equality comparable and copyable.
  //!
  //! Container is a type that satisfies the requirements of a container of Region,
  //! such as std::vector<Region>.
  template <typename Region> region(Region a) : interface_(new region_model<Region>(std::move(a)))
  {
    // XXX: simulation never uses this function, should we remove this requirement?
    static_assert(is_detected_v<mb_adjacent_regions_t, Region>,
                  "missing member function Region::adjacent_regions() -> Container<Region>");
    static_assert(is_detected_convertible_v<std::size_t, mb_hash_t, Region>,
                  "missing member function Region::hash() -> convertible_to<size_t>");
    static_assert(is_detected_convertible_v<bool, equality_t, Region>, "Region does not satisfy equality comparable");
  }

  region() noexcept = delete;

  region(const region&);
  region(region&&) noexcept = default;

  auto operator=(const region&) -> region&;
  auto operator=(region&&) noexcept -> region& = default;

  //! Returns a vector containing all adjacent regions.
  auto adjacent_regions() const -> std::vector<region>;

  //! \private
  auto hash() const -> std::size_t;

  auto operator==(const region&) const -> bool;
  auto operator!=(const region&) const -> bool;

  //! Returns the shortest distance (in steps) to another region.
  //!
  //! If the distance is not defined in the original region, not_implemented is
  //! thrown.
  auto distance(const region&) const -> uint_t;

  //! Returns the heuristic distance to another region.
  //!
  //! If the heuristic distance is not defined in the original region, the distance is
  //! returned.
  auto heuristic_distance(const region&) const -> value_t;

  //! Returns the shortest path to another region.
  //!
  //! If the shortest path is not defined in the original region, an empty vector is
  //! returned.
  //!
  //! @param to The destination region.
  //! @param seed The seed used to break ties.
  //!
  //! @return A vector containing the regions in the shortest path, including the
  //!         origin and destination regions.  It is also assumed that the origin
  //!         region is the first element of the vector and the destination region
  //!         is the last element.  All elements in between are the regions in the
  //!         path.
  auto shortest_path(const region& to, int seed) const -> std::vector<region>;

  //! Prints the region using fmt::format syntax.
  //!
  //! If the region does not define a print function, the string "NA" is printed
  //! effectively calling `f("NA", {})`
  //!
  //! @param f A function that takes a string_view and a format_args object.
  //!          The string_view is the format string and the format_args object
  //!          contains the arguments to be formatted inside `fmt::make_format_args(...)`.
  auto print_to(std::function<void(std::string_view, fmt::format_args)> f) const -> void;

  //! Returns true if the movement (\p before, `this`, \p to) is a turn.
  //!
  //! If the region does not define a turn function, false is returned.
  auto turn(const region& before, const region& to) const -> bool;

  //! Returns true if the movement (`this`, \p to) is a climb.
  //!
  //! If the region does not define a climb function, false is returned.
  auto climb(const region& to) const -> bool;

  //! Downcast the region to its original type.
  //!
  //! Type T must be the original type used to construct the region.
  //! Otherwise, behavior is undefined.
  template <typename T> auto downcast() -> T& { return dynamic_cast<region_model<T>&>(*interface_).downcast(); }

  //! Downcast the region to its original type.
  //!
  //! Type T must be the original type used to construct the region.
  //! Otherwise, behavior is undefined.
  template <typename T> auto downcast() const -> const T& { return dynamic_cast<region_model<T>&>(*interface_).downcast(); }

private:
  std::unique_ptr<region_interface> interface_;
};

//! \brief A permit is a class that represents a permission to fly in a region at a given time.
//!
//! The time interval is represented as a non-negative integer.  (Arbitrary time spans are
//! not supported.)
class permit
{
public:
  permit() noexcept = delete;
  permit(class region s, uint_t time) noexcept;

  auto time() const noexcept -> uint_t;
  auto location() const noexcept -> const class region&;
  auto location() noexcept -> class region&;

  auto operator==(const permit& other) const -> bool;
  auto operator!=(const permit& other) const -> bool;

private:
  region region_;
  uint_t time_;
};

//! \private
template <std::size_t I> decltype(auto) get(permit& ts)
{
  if constexpr (I == 0)
    return ts.location();
  else
    return ts.time();
}

//! \private
template <std::size_t I> decltype(auto) get(const permit& ts)
{
  if constexpr (I == 0)
    return ts.location();
  else
    return ts.time();
}

} // namespace uat


//! Class that enables the formatting (using fmtlib) of the region class.
template <> struct fmt::formatter<uat::region>
{
  //! \private
  constexpr auto parse(format_parse_context& ctx)
  {
    if (ctx.begin() != ctx.end() && *ctx.begin() != '}')
      throw format_error("invalid format");
    return ctx.begin();
  }

  //! \private
  template <typename FormatContext> auto format(const uat::region& p, FormatContext& ctx)
  {
    auto out = ctx.out();
    p.print_to([&ctx, &out](std::string_view str, format_args args) { out = vformat_to(ctx.out(), str, std::move(args)); });
    return out;
  }
};

namespace std
{

//! Class that enables the usage of std::unordered_* with the region class.
template <> struct hash<uat::region>
{
  //! \private
  auto operator()(const uat::region&) const noexcept -> size_t;
};

//! Class that enables the usage of std::unordered_* with the permit class.
template <> struct hash<uat::permit>
{
  //! \private
  auto operator()(const uat::permit&) const noexcept -> size_t;
};

//! Class that enables the usage of structured bindings with the permit class.
template <> struct tuple_size<uat::permit> : public integral_constant<size_t, 2>
{};

//! \private
template <> struct tuple_element<0, uat::permit>
{
  using type = uat::region&;
};

//! \private
template <> struct tuple_element<0, const uat::permit>
{
  using type = const uat::region&;
};

//! \private
template <> struct tuple_element<1, uat::permit>
{
  using type = uat::uint_t;
};

//! \private
template <> struct tuple_element<1, const uat::permit>
{
  using type = uat::uint_t;
};
} // namespace std

#endif // UAT_PERMIT_HPP
