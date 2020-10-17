#ifndef UAT_AIRSPACE_HPP
#define UAT_AIRSPACE_HPP

#include <uat/type.hpp>
#include <uat/permit.hpp>

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace uat
{

using region_fn = std::function<bool(const region&)>;

template<class T>
using mb_random_mission_t = decltype(std::declval<const T&>().random_mission(int{}));

template<class T>
using mb_iterate_t = decltype(std::declval<const T&>().iterate(std::declval<region_fn>()));

struct mission_t
{
  region from, to;
  auto length() const { return from.distance(to); }
};

class airspace
{
  class airspace_interface
  {
  public:
    virtual ~airspace_interface() = default;
    virtual auto clone() const -> std::unique_ptr<airspace_interface> = 0;

    virtual auto random_mission(int) const -> mission_t = 0;
    virtual auto iterate(region_fn) const -> void = 0;
  };

  template <typename Airspace>
  class airspace_model : public airspace_interface
  {
  public:
    airspace_model(Airspace airspace) : airspace_(std::move(airspace)) {}
    virtual ~airspace_model() = default;

    auto clone() const -> std::unique_ptr<airspace_interface> override {
      return std::unique_ptr<airspace_interface>{new airspace_model(airspace_)};
    }

    auto random_mission(int seed) const -> mission_t override { return airspace_.random_mission(seed); }

    auto iterate(region_fn callback) const -> void override { return airspace_.iterate(std::move(callback)); }

  private:
    Airspace airspace_;
  };

public:
  template <typename Airspace>
  airspace(Airspace a) : interface_(new airspace_model<Airspace>(std::move(a)))
  {
    static_assert(is_detected_convertible_v<mission_t, mb_random_mission_t, Airspace>, "missing member function Airspace::random_mission() -> mission_t");
    static_assert(is_detected_exact_v<void, mb_iterate_t, Airspace>, "missing member function Airspace::iterate(region_fn)");

    assert(interface_);
  }

  airspace(const airspace&);
  airspace(airspace&&) noexcept = default;

  auto operator=(const airspace&) -> airspace&;
  auto operator=(airspace&&) noexcept -> airspace& = default;

  auto random_mission(int) const -> mission_t;

  auto iterate(region_fn) const -> void;

private:
  std::unique_ptr<airspace_interface> interface_;
};

} // namespace uat

#endif // UAT_AIRSPACE_HPP
