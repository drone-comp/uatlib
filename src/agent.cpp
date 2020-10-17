#include <uat/agent.hpp>

namespace uat
{

agent::agent(const agent& other) : interface_(other.interface_->clone()) {}

auto agent::operator=(const agent& other) -> agent&
{
  interface_ = other.interface_->clone();
  return *this;
}

auto agent::act(uint_t t, bid_fn b, permit_public_status_fn s, int seed) -> bool
{
  return interface_->act(t, std::move(b), std::move(s), seed);
}

auto agent::after_trading(uint_t t, ask_fn a, permit_public_status_fn s, int seed) -> void
{
  interface_->after_trading(t, std::move(a), std::move(s), seed);
}

auto agent::on_bought(const region& s, uint_t t, value_t v) -> void { interface_->on_bought(s, t, v); }

auto agent::on_sold(const region& s, uint_t t, value_t v) -> void { interface_->on_sold(s, t, v); }

auto agent::on_finished(uint_t id, uint_t t) -> void { interface_->on_finished(id, t); }

} // namespace uat
