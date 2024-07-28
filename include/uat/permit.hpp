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
#include <boost/functional/hash.hpp>

namespace uat
{

//! \private
template <typename T> using mb_hash_t = decltype(std::declval<const T&>().hash());

//! \private
template <typename T>
using mb_print_t =
  decltype(std::declval<const T&>().print(std::declval<std::function<void(std::string_view, fmt::format_args)>>()));

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

    virtual auto hash() const -> std::size_t = 0;
    virtual auto equals(const region_interface&) const -> bool = 0;
    virtual auto print_to(std::function<void(std::string_view, fmt::format_args)>) const -> void = 0;
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

    auto hash() const -> std::size_t override { return region_.hash(); }

    auto equals(const region_interface& other) const -> bool override
    {
      return region_ == dynamic_cast<const region_model&>(other).region_;
    }

    auto print_to(std::function<void(std::string_view, fmt::format_args)> f) const -> void override
    {
      if constexpr (is_detected_exact_v<void, mb_print_t, Region>)
        region_.print(std::move(f));
      else
        f("NA", {});
    }

    auto downcast() -> Region& { return region_; }
    auto downcast() const -> const Region& { return region_; }

  private:
    Region region_;
  };

public:
  //! Constructs a type-erased region from an object of type Region that satisfy at least:
  //!
  //! - `Region::hash() -> convertible_to<size_t>`
  //!
  //! Moreover, it should be equality comparable and copyable.
  //!
  //! Container is a type that satisfies the requirements of a container of Region,
  //! such as std::vector<Region>.
  template <typename Region> region(Region a) : interface_(new region_model<Region>(std::move(a)))
  {
    static_assert(is_detected_convertible_v<std::size_t, mb_hash_t, Region>,
                  "missing member function Region::hash() -> convertible_to<size_t>");
    static_assert(is_detected_convertible_v<bool, equality_t, Region>, "Region does not satisfy equality comparable");
  }

  region() noexcept = delete;

  region(const region&);
  region(region&&) noexcept = default;

  auto operator=(const region&) -> region&;
  auto operator=(region&&) noexcept -> region& = default;

  //! \private
  auto hash() const -> std::size_t;

  auto operator==(const region&) const -> bool;
  auto operator!=(const region&) const -> bool;

  //! Prints the region using fmt::format syntax.
  //!
  //! If the region does not define a print function, the string "NA" is printed
  //! effectively calling `f("NA", {})`
  //!
  //! @param f A function that takes a string_view and a format_args object.
  //!          The string_view is the format string and the format_args object
  //!          contains the arguments to be formatted inside `fmt::make_format_args(...)`.
  auto print_to(std::function<void(std::string_view, fmt::format_args)> f) const -> void;

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

template <typename Region = void>
class permit
{
public:
  permit() noexcept = delete;
  permit(Region s, uint_t time) noexcept : region_(std::move(s)), time_(time) {}

  auto time() const noexcept -> uint_t { return time_; }
  auto location() const noexcept -> const Region& { return region_; }
  auto location() noexcept -> Region& { return region_; }

  auto operator==(const permit& other) const -> bool { return region_ == other.region_ && time_ == other.time_; }
  auto operator!=(const permit& other) const -> bool { return !(*this == other); }

private:
  Region region_;
  uint_t time_;
};

//! \brief A permit is a class that represents a permission to fly in a region at a given time.
//!
//! The time interval is represented as a non-negative integer.  (Arbitrary time spans are
//! not supported.)
template <>
class permit<void>
{
public:
  permit() noexcept = delete;
  permit(region s, uint_t time) noexcept;

  template <typename Region>
  permit(Region s, uint_t time) noexcept : region_(std::move(s)), time_(time) {}

  auto time() const noexcept -> uint_t;
  auto location() const noexcept -> const region&;
  auto location() noexcept -> region&;

  auto operator==(const permit& other) const -> bool;
  auto operator!=(const permit& other) const -> bool;

private:
  region region_;
  uint_t time_;
};

//! \private
template <std::size_t I, typename R> decltype(auto) get(permit<R>& ts)
{
  if constexpr (I == 0)
    return ts.location();
  else
    return ts.time();
}

//! \private
template <std::size_t I, typename R> decltype(auto) get(const permit<R>& ts)
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
template <typename Region> struct hash<uat::permit<Region>>
{
  //! \private
  auto operator()(const uat::permit<Region>& p) const noexcept -> size_t {
    size_t seed = 0;
    boost::hash_combine(seed, p.location().hash());
    boost::hash_combine(seed, std::hash<std::size_t>{}(p.time()));
    return seed;
  }
};

//! Class that enables the usage of structured bindings with the permit class.
template <typename R> struct tuple_size<uat::permit<R>> : public integral_constant<size_t, 2>
{};

//! \private
template <typename R> struct tuple_element<0, uat::permit<R>>
{
  using type = std::conditional_t<std::is_same_v<R, void>, uat::region, R>&;
};

//! \private
template <typename R> struct tuple_element<0, const uat::permit<R>>
{
  using type = const std::conditional_t<std::is_same_v<R, void>, uat::region, R>&;
};

//! \private
template <typename R> struct tuple_element<1, uat::permit<R>>
{
  using type = uat::uint_t;
};

//! \private
template <typename R> struct tuple_element<1, const uat::permit<R>>
{
  using type = uat::uint_t;
};
} // namespace std

#endif // UAT_PERMIT_HPP
