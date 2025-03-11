#include <uat/simulation.hpp>

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <variant>

namespace uat
{

auto agents_private_status_t::status(id_t id) const -> agent_private_status_t { throw std::runtime_error{"not implemented yet"}; }

auto agents_private_status_t::active_count() const -> uint_t { return active_.size(); }

auto agents_private_status_t::active() const -> std::span<const id_t> { return active_; }

void agents_private_status_t::insert(any_agent a)
{
  active_.push_back(first_id_ + agents_.size());
  agents_.push_back(std::move(a));
}

void agents_private_status_t::update_active(std::vector<id_t> new_agents)
{
  assert(std::is_sorted(new_agents.begin(), new_agents.end()));
  active_ = std::move(new_agents);
  if (active_.size() == 0)
    return;

  const auto first = active_.front();
  while (first_id_ < first) {
    ++first_id_;
    agents_.pop_front();
  }
}

auto agents_private_status_t::at(id_t id) -> any_agent&
{
  assert(id >= first_id_);
  assert(id - first_id_ < agents_.size());
  return agents_[id - first_id_];
}

} // namespace uat
