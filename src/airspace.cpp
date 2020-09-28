#include <uat/airspace.hpp>

namespace uat
{

airspace::airspace(const airspace& other) : interface_(other.interface_->clone()) {}

auto airspace::operator=(const airspace& other) -> airspace&
{
  interface_ = other.interface_->clone();
  return *this;
}

auto airspace::random_mission(int seed) const -> mission_t { return interface_->random_mission(seed); }

} // namespace uat
