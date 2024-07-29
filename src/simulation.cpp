#include <uat/simulation.hpp>

#include <cassert>
#include <deque>
#include <functional>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <variant>

namespace uat
{

auto agents_private_status_fn::operator()(id_t id) const -> agent_private_status_t
{
  throw std::runtime_error{"not implemented yet"};
}

auto agents_private_status_fn::active_count() const -> uint_t { return active_.size(); }

auto agents_private_status_fn::active() const -> const std::vector<id_t>& { return active_; }

void agents_private_status_fn::insert(any_agent a)
{
  active_.push_back(first_id_ + agents_.size());
  agents_.push_back(std::move(a));
}

void agents_private_status_fn::update_active(std::vector<id_t> new_agents)
{
  active_ = std::move(new_agents);
  // TODO: assert it is sorted.
  if (active_.size() == 0)
    return;

  const auto first = active_.front();
  while (first_id_ < first) {
    ++first_id_;
    agents_.pop_front();
  }
}

auto agents_private_status_accessor::active(const agents_private_status_fn& self) const -> const std::vector<id_t>&
{
  return self.active_;
}

auto agents_private_status_accessor::at(agents_private_status_fn& self, id_t id) const -> any_agent&
{
  assert(id >= self.first_id_);
  assert(id - self.first_id_ < self.agents_.size());
  return self.agents_[id - self.first_id_];
}

auto agents_private_status_accessor::insert(agents_private_status_fn& self, any_agent a) const -> void
{
  self.insert(std::move(a));
}

auto agents_private_status_accessor::update_active(agents_private_status_fn& self, std::vector<id_t> new_agents) const -> void
{
  self.update_active(std::move(new_agents));
}

} // namespace uat
