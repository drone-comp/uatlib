#include <uat/agent.hpp>

namespace uat
{

agent::agent(const agent& other) : interface_(other.interface_->clone()) {}

auto agent::operator=(const agent& other) -> agent&
{
  interface_ = other.interface_->clone();
  return *this;
}

auto agent::bid_phase(uint_t t, bid_fn b, permit_public_status_fn s, int seed) -> void
{
  interface_->bid_phase(t, std::move(b), std::move(s), seed);
}

auto agent::ask_phase(uint_t t, ask_fn a, permit_public_status_fn s, int seed) -> void
{
  interface_->ask_phase(t, std::move(a), std::move(s), seed);
}

auto agent::on_bought(region_view s, uint_t t, value_t v) -> void { interface_->on_bought(s, t, v); }

auto agent::on_sold(region_view s, uint_t t, value_t v) -> void { interface_->on_sold(s, t, v); }

auto agent::stop(uint_t t, int seed) -> bool { return interface_->stop(t, seed); }

} // namespace uat
