//! \file airspace.hpp
//! \brief Defines the airspace class and related types.

#ifndef UAT_AIRSPACE_HPP
#define UAT_AIRSPACE_HPP

#include <uat/permit.hpp>
#include <uat/type.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace uat
{

//! A callback type to traverse the regions of the airspace.
using region_fn = std::function<bool(const region&)>;

//! \private
template <class T> using mb_random_mission_t = decltype(std::declval<const T&>().random_mission(int{}));

//! \private
template <class T> using mb_iterate_t = decltype(std::declval<const T&>().iterate(std::declval<region_fn>()));

//! A mission is just a pair of regions.
struct mission_t
{
  region from, to;

  //! Returns the distance between the starting (from) and ending (to) regions.
  auto length() const { return from.distance(to); }
};

//! \brief A type-erased class that represents the airspace.
class airspace
{
  //! \private
  class airspace_interface
  {
  public:
    virtual ~airspace_interface() = default;
    virtual auto clone() const -> std::unique_ptr<airspace_interface> = 0;

    virtual auto random_mission(int) const -> mission_t = 0;
    virtual auto iterate(region_fn) const -> void = 0;
  };

  //! \private
  template <typename Airspace> class airspace_model : public airspace_interface
  {
  public:
    airspace_model(Airspace airspace) : airspace_(std::move(airspace)) {}
    virtual ~airspace_model() = default;

    auto clone() const -> std::unique_ptr<airspace_interface> override
    {
      return std::unique_ptr<airspace_interface>{new airspace_model(airspace_)};
    }

    auto random_mission(int seed) const -> mission_t override { return airspace_.random_mission(seed); }

    auto iterate(region_fn callback) const -> void override { return airspace_.iterate(std::move(callback)); }

  private:
    Airspace airspace_;
  };

public:
  //! Constructs a type-erased airspace from an object of type Airspace that satisfy at least:
  //!
  //! - `Airspace::random_mission(int) -> mission_t`
  //! - `Airspace::iterate(region_fn) -> void`
  //!
  //! The Airspace object should be copyable.
  template <typename Airspace> airspace(Airspace a) : interface_(new airspace_model<Airspace>(std::move(a)))
  {
    // XXX: simulation never uses this function, should we remove this requirement?
    static_assert(is_detected_convertible_v<mission_t, mb_random_mission_t, Airspace>,
                  "missing member function Airspace::random_mission() -> mission_t");
    // XXX: simulation never uses this function, should we remove this requirement?
    static_assert(is_detected_exact_v<void, mb_iterate_t, Airspace>, "missing member function Airspace::iterate(region_fn)");
  }

  airspace() = delete;

  airspace(const airspace&);
  airspace(airspace&&) noexcept = default;

  auto operator=(const airspace&) -> airspace&;
  auto operator=(airspace&&) noexcept -> airspace& = default;

  //! Returns a random mission.
  //!
  //! \param seed A random seed.
  auto random_mission(int seed) const -> mission_t;

  //! Iterates over the regions of the airspace.
  //!
  //! \param callback A callback function with signature `bool(const region&)` that will
  //!                 be called for each region in the airspace.
  //!                 Implementer should return false at the last region to stop the iteration.
  auto iterate(region_fn callback) const -> void;

private:
  std::unique_ptr<airspace_interface> interface_;
};

} // namespace uat

#endif // UAT_AIRSPACE_HPP
