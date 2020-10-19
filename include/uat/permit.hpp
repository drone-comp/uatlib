#ifndef UAT_PERMIT_HPP
#define UAT_PERMIT_HPP

#include <uat/type.hpp>

#include <cassert>
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

template <typename T> using mb_adjacent_regions_t = decltype(std::declval<const T&>().adjacent_regions());

template <typename T> using mb_hash_t = decltype(std::declval<const T&>().hash());

template <typename T> using mb_distance_t = decltype(std::declval<const T&>().distance(std::declval<const T&>()));

template <typename T>
using mb_heuristic_distance_t = decltype(std::declval<const T&>().heuristic_distance(std::declval<const T&>()));

template <typename T> using mb_shortest_path_t = decltype(std::declval<const T&>().shortest_path(std::declval<const T&>()));

template <typename T>
using mb_print_t =
  decltype(std::declval<const T&>().print(std::declval<std::function<void(std::string_view, fmt::format_args)>>()));

template <typename T> using mb_climb_t = decltype(std::declval<const T&>().climb(std::declval<const T&>()));

template <typename T>
using mb_turn_t = decltype(std::declval<const T&>().turn(std::declval<const T&>(), std::declval<const T&>()));

class region
{
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
      return region_.distance(dynamic_cast<const region_model&>(other).region_);
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
  template <typename Region> region(Region a) : interface_(new region_model<Region>(std::move(a)))
  {
    static_assert(is_detected_v<mb_adjacent_regions_t, Region>,
                  "missing member function Region::adjacent_regions() -> Container<Region>");
    static_assert(is_detected_convertible_v<std::size_t, mb_hash_t, Region>,
                  "missing member function Region::hash() -> convertible_to<size_t>");
    static_assert(is_detected_convertible_v<bool, equality_t, Region>, "Region does not satisfy equality comparable");
    static_assert(is_detected_convertible_v<uint_t, mb_distance_t, Region>,
                  "missing member function Region::distance(Region) -> convertible_to<uint_t>");

    assert(interface_);
  }

  region() noexcept = default;

  region(const region&);
  region(region&&) noexcept = default;

  auto operator=(const region&) -> region&;
  auto operator=(region&&) noexcept -> region& = default;

  auto adjacent_regions() const -> std::vector<region>;
  auto hash() const -> std::size_t;

  auto operator==(const region&) const -> bool;
  auto operator!=(const region&) const -> bool;

  auto distance(const region&) const -> uint_t; // real distance

  auto heuristic_distance(const region&) const -> value_t; // heuristic for A*

  auto shortest_path(const region&, int) const -> std::vector<region>; // must be ordered

  auto print_to(std::function<void(std::string_view, fmt::format_args)>) const -> void;

  auto turn(const region& before, const region& to) const -> bool;

  auto climb(const region& to) const -> bool;

  template <typename T> auto downcast() -> T& { return dynamic_cast<region_model<T>&>(interface_).downcast(); }
  template <typename T> auto downcast() const -> const T& { return dynamic_cast<region_model<T>&>(interface_).downcast(); }

private:
  std::unique_ptr<region_interface> interface_;
};

class permit
{
public:
  permit() noexcept = default;
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

template <std::size_t I> decltype(auto) get(permit& ts)
{
  if constexpr (I == 0)
    return ts.location();
  else
    return ts.time();
}

template <std::size_t I> decltype(auto) get(const permit& ts)
{
  if constexpr (I == 0)
    return ts.location();
  else
    return ts.time();
}

} // namespace uat

template <> struct fmt::formatter<uat::region>
{
  constexpr auto parse(format_parse_context& ctx)
  {
    if (ctx.begin() != ctx.end() && *ctx.begin() != '}')
      throw format_error("invalid format");
    return ctx.begin();
  }

  template <typename FormatContext> auto format(const uat::region& p, FormatContext& ctx)
  {
    auto out = ctx.out();
    p.print_to([&ctx, &out](std::string_view str, format_args args) { out = vformat_to(ctx.out(), str, std::move(args)); });
    return out;
  }
};

namespace std
{
template <> struct hash<uat::region>
{
  auto operator()(const uat::region&) const noexcept -> size_t;
};

template <> struct hash<uat::permit>
{
  auto operator()(const uat::permit&) const noexcept -> size_t;
};

template <> struct tuple_size<uat::permit> : public integral_constant<size_t, 2>
{};

template <> struct tuple_element<0, uat::permit>
{
  using type = uat::region&;
};

template <> struct tuple_element<0, const uat::permit>
{
  using type = const uat::region&;
};

template <> struct tuple_element<1, uat::permit>
{
  using type = uat::uint_t;
};

template <> struct tuple_element<1, const uat::permit>
{
  using type = uat::uint_t;
};
} // namespace std

#endif // UAT_PERMIT_HPP
