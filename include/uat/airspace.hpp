#ifndef UAT_AIRSPACE_HPP
#define UAT_AIRSPACE_HPP

#include <uat/type.hpp>
#include <uat/slot.hpp>

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace uat
{

template<class T>
using random_mission_t = decltype(std::declval<const T&>().random_mission(int{}));

struct mission_t
{
  slot from, to;
  auto distance() const { return from.distance(to); }
};

class airspace
{
  class airspace_interface
  {
  public:
    virtual ~airspace_interface() = default;
    virtual auto clone() const -> std::unique_ptr<airspace_interface> = 0;

    virtual auto random_mission(int) const -> mission_t = 0;
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

  private:
    Airspace airspace_;
  };

public:
  template <typename Airspace>
  airspace(Airspace a) : interface_(new airspace_model<Airspace>(std::move(a)))
  {
    static_assert(is_detected_convertible_v<mission_t, random_mission_t, Airspace>, "missing method 'Airspace::random_mission() -> mission_t'");

    assert(interface_);
  }

  airspace(const airspace&);
  airspace(airspace&&) noexcept = default;

  auto operator=(const airspace&) -> airspace&;
  auto operator=(airspace&&) noexcept -> airspace& = default;

  auto random_mission(int) const -> mission_t;

private:
  std::unique_ptr<airspace_interface> interface_;
};

} // namespace uat

#endif // UAT_AIRSPACE_HPP
