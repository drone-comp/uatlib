//! \file permit.hpp
//! \brief Defines region utilities and the permit class.

#ifndef UAT_PERMIT_HPP
#define UAT_PERMIT_HPP

#include <uat/type.hpp>

#include <memory>

#include <boost/functional/hash.hpp>

namespace uat
{

//! \brief A non-owning wrapper to an atomic region in the airspace.
//!
//! Any object that satisfies \ref `region_compatible` can be used to construct a \ref region_view.
//! No modifications are allowed to the region through the view (const semantics).
class region_view
{
public:
  //! Constructs a non-owning wrapper to a region that satisfies the \ref `region_compatible` concept.
  template <region_compatible R> region_view(const R& a) : region_(std::addressof(a)) {}

  region_view() noexcept = delete;

  region_view(const region_view&) = default;
  region_view(region_view&&) noexcept = default;

  auto operator=(const region_view&) -> region_view& = default;
  auto operator=(region_view&&) noexcept -> region_view& = default;

  //! Downcast the region view to its original type.
  //!
  //! Type R must be the original type used to construct the region.
  //! Otherwise, behavior is undefined.
  template <region_compatible R> auto downcast() const -> const R& { return *reinterpret_cast<const R*>(region_); }

private:
  const void* region_;
};

//! \brief A tuple containing a region and a time step.
//!
//! A permit refers to the assets agents can trade.
//! This class is just a convenient way to group a region and a time step.
//! Structured bindings and hashing are supported.
template <region_compatible Region> class permit
{
public:
  permit() noexcept = delete;
  permit(Region s, uint_t time) noexcept : region_(std::move(s)), time_(time) {}

  auto time() const noexcept -> uint_t { return time_; }
  auto location() const noexcept -> const Region& { return region_; }
  auto location() noexcept -> Region& { return region_; }

  auto operator==(const permit& other) const -> bool { return region_ == other.region_ && time_ == other.time_; }
  auto operator!=(const permit& other) const -> bool { return region_ != other.region_ || time_ != other.time_; }

private:
  Region region_;
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

namespace std
{

//! \private
template <typename Region> struct hash<uat::permit<Region>>
{
  //! \private
  auto operator()(const uat::permit<Region>& p) const noexcept -> size_t
  {
    size_t seed = 0;
    boost::hash_combine(seed, std::hash<Region>{}(p.location()));
    boost::hash_combine(seed, std::hash<std::size_t>{}(p.time()));
    return seed;
  }
};

//! \private
template <typename R> struct tuple_size<uat::permit<R>> : public integral_constant<size_t, 2>
{};

//! \private
template <typename R> struct tuple_element<0, uat::permit<R>>
{
  using type = R&;
};

//! \private
template <typename R> struct tuple_element<0, const uat::permit<R>>
{
  using type = const R&;
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
