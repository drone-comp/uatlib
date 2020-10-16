#ifndef UAT_SLOT_HPP
#define UAT_SLOT_HPP

#include <uat/type.hpp>

#include <string_view>
#include <tuple>
#include <cassert>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <functional>

#include <fmt/format.h>

namespace uat
{

template <typename T>
using neighbors_t = decltype(std::declval<const T&>().neighbors());

template <typename T>
using hash_t = decltype(std::declval<const T&>().hash());

template <typename T>
using compare_t = decltype(std::declval<const T&>() == std::declval<const T&>());

template <typename T>
using distance_t = decltype(std::declval<const T&>().distance(std::declval<const T&>()));

template <typename T>
using heuristic_distance_t = decltype(std::declval<const T&>().heuristic_distance(std::declval<const T&>()));

template <typename T>
using shortest_path_t = decltype(std::declval<const T&>().shortest_path(std::declval<const T&>()));

template <typename T>
using print_t = decltype(std::declval<const T&>().print(std::declval<std::function<void(std::string_view, fmt::format_args)>>()));

template <typename T>
using climb_t = decltype(std::declval<const T&>().climb(std::declval<const T&>()));

template <typename T>
using turn_t = decltype(std::declval<const T&>().turn(std::declval<const T&>(), std::declval<const T&>()));

class slot
{
  class slot_interface
  {
  public:
    virtual ~slot_interface() = default;
    virtual auto clone() const -> std::unique_ptr<slot_interface> = 0;

    virtual auto neighbors() const -> std::vector<slot> = 0;
    virtual auto hash() const -> std::size_t = 0;
    virtual auto equals(const slot_interface&) const -> bool = 0;
    virtual auto distance(const slot_interface&) const -> uint_t = 0;
    virtual auto heuristic_distance(const slot_interface&) const -> value_t = 0;
    virtual auto shortest_path(const slot_interface&, int) const -> std::vector<slot> = 0;
    virtual auto print_to(std::function<void(std::string_view, fmt::format_args)>) const -> void = 0;
    virtual auto turn(const slot_interface&, const slot_interface&) const -> bool = 0;
    virtual auto climb(const slot_interface&) const -> bool = 0;
  };

  template <typename Slot>
  class slot_model : public slot_interface
  {
  public:
    slot_model(Slot slot) : slot_(std::move(slot)) {}
    virtual ~slot_model() = default;

    auto clone() const -> std::unique_ptr<slot_interface> override {
      return std::unique_ptr<slot_interface>{new slot_model(slot_)};
    }

    auto neighbors() const -> std::vector<slot> override
    {
      if constexpr (std::is_same_v<std::vector<slot>, decltype(slot_.neighbors())>)
      {
        return slot_.neighbors();
      }
      else
      {
        auto nei = slot_.neighbors();
        static_assert(std::is_convertible_v<slot, typename decltype(nei)::value_type>);

        std::vector<slot> converted;
        converted.reserve(nei.size());
        std::move(nei.begin(), nei.end(), std::back_inserter(converted));
        return converted;
      }
    }

    auto hash() const -> std::size_t override { return slot_.hash(); }

    auto equals(const slot_interface& other) const -> bool override
    {
      return slot_ == dynamic_cast<const slot_model&>(other).slot_;
    }

    auto distance(const slot_interface& other) const -> uint_t override
    {
      return slot_.distance(dynamic_cast<const slot_model&>(other).slot_);
    }

    auto heuristic_distance(const slot_interface& other) const -> value_t override
    {
      if constexpr (is_detected_convertible_v<value_t, heuristic_distance_t, Slot>)
        return slot_.heuristic_distance(dynamic_cast<const slot_model&>(other).slot_);
      else
        return slot_.distance(dynamic_cast<const slot_model&>(other).slot_);
    }

    auto shortest_path(const slot_interface& other, int seed) const -> std::vector<slot> override
    {
      if constexpr (is_detected_exact_v<std::vector<slot>, shortest_path_t, Slot>)
        return slot_.shortest_path(dynamic_cast<const slot_model&>(other).slot_, seed);
      else
        return {};
    }

    auto print_to(std::function<void(std::string_view, fmt::format_args)> f) const -> void override
    {
      if constexpr (is_detected_exact_v<void, print_t, Slot>)
        slot_.print(std::move(f));
      else
        f("NA", {});
    }

    auto turn(const slot_interface& before, const slot_interface& to) const -> bool override
    {
      if constexpr (is_detected_convertible_v<bool, turn_t, Slot>)
        return slot_.turn(
            dynamic_cast<const slot_model&>(before).slot_,
            dynamic_cast<const slot_model&>(to).slot_);
      else
        return false;
    }

    auto climb(const slot_interface& to) const -> bool override
    {
      if constexpr (is_detected_convertible_v<bool, climb_t, Slot>)
        return slot_.climb(dynamic_cast<const slot_model&>(to).slot_);
      else
        return false;
    }

  private:
    Slot slot_;
  };

public:
  template <typename Slot>
  slot(Slot a) : interface_(new slot_model<Slot>(std::move(a)))
  {
    static_assert(is_detected_convertible_v<std::vector<slot>, neighbors_t, Slot>, "missing method 'Slot::neighbors() -> vector<slot>'");
    static_assert(is_detected_convertible_v<std::size_t, hash_t, Slot>, "missing method 'Slot::hash() -> size_t'");
    static_assert(is_detected_convertible_v<bool, compare_t, Slot>, "missing method '{ Slot == Slot } -> bool'");
    static_assert(is_detected_convertible_v<uint_t, distance_t, Slot>, "missing method 'Slot::distance(Slot) -> uint_t'");

    assert(interface_);
  }

  slot() noexcept = default;

  slot(const slot&);
  slot(slot&&) noexcept = default;

  auto operator=(const slot&) -> slot&;
  auto operator=(slot&&) noexcept -> slot& = default;

  auto neighbors() const -> std::vector<slot>;
  auto hash() const -> std::size_t;

  auto operator==(const slot&) const -> bool;
  auto operator!=(const slot&) const -> bool;

  auto distance(const slot&) const -> uint_t; // real distance

  auto heuristic_distance(const slot&) const -> value_t; // heuristic for A*

  auto shortest_path(const slot&, int) const -> std::vector<slot>; // must be ordered

  auto print_to(std::function<void(std::string_view, fmt::format_args)>) const -> void;

  auto turn(const slot& before, const slot& to) const -> bool;

  auto climb(const slot& to) const -> bool;

private:
  std::unique_ptr<slot_interface> interface_;
};

// TODO: better naming (class name and methods)
class tslot
{
public:
  tslot() noexcept = default;
  tslot(class slot s, uint_t time) noexcept;

  auto time() const noexcept -> uint_t;
  auto slot() const noexcept -> const class slot&;
  auto slot() noexcept -> class slot&;

  auto operator==(const tslot& other) const -> bool;
  auto operator!=(const tslot& other) const -> bool;

private:
  class slot slot_;
  uint_t time_;
};

template <std::size_t I> decltype(auto) get(tslot& ts)
{
  if constexpr (I == 0)
    return ts.slot();
  else
    return ts.time();
}

template <std::size_t I> decltype(auto) get(const tslot& ts)
{
  if constexpr (I == 0)
    return ts.slot();
  else
    return ts.time();
}

} // namespace uat


template <>
struct fmt::formatter<uat::slot>
{
  constexpr auto parse(format_parse_context& ctx) {
    if (ctx.begin() != ctx.end() && *ctx.begin() != '}')
      throw format_error("invalid format");
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const uat::slot& p, FormatContext& ctx)
  {
    auto out = ctx.out();
    p.print_to([&ctx, &out](std::string_view str, format_args args) {
      out = vformat_to(ctx.out(), str, args);
    });
    return out;
  }
};

namespace std
{
template <> struct hash<uat::slot>
{
  auto operator()(const uat::slot&) const noexcept -> size_t;
};

template <> struct hash<uat::tslot>
{
  auto operator()(const uat::tslot&) const noexcept -> size_t;
};

template <> struct tuple_size<uat::tslot> : public integral_constant<size_t, 2> {};

template <> struct tuple_element<0, uat::tslot>
{
  using type = uat::slot&;
};

template <> struct tuple_element<0, const uat::tslot>
{
  using type = const uat::slot&;
};

template <> struct tuple_element<1, uat::tslot>
{
  using type = uat::uint_t;
};

template <> struct tuple_element<1, const uat::tslot>
{
  using type = uat::uint_t;
};
} // namespace std

#endif // UAT_SLOT_HPP
