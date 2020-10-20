#include <uat/simulation.hpp>

#include <deque>
#include <functional>
#include <limits>
#include <random>
#include <unordered_map>
#include <variant>

#include <cool/compose.hpp>
#include <fmt/format.h>

namespace uat
{

auto agents_private_status_fn::operator()(id_t id) const -> agent_private_status_t { return {}; }

auto agents_private_status_fn::active_count() const -> uint_t { return active_.size(); }

void agents_private_status_fn::insert(agent a)
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

struct agents_private_status_accessor
{
  auto active(const agents_private_status_fn& self) const -> const std::vector<id_t>& { return self.active_; }
  auto at(agents_private_status_fn& self, id_t id) const -> agent&
  {
    assert(id >= self.first_id_);
    assert(id - self.first_id_ < self.agents_.size());
    return self.agents_[id - self.first_id_];
  }
};

auto simulate(factory_fn factory, airspace space, int seed, const simulation_opts_t& opts) -> void
{
  std::mt19937 rnd(seed);

  agents_private_status_fn agents;
  constexpr agents_private_status_accessor accessor;
  std::vector<id_t> keep_active, to_finished;

  uint_t t0 = 0;

  std::deque<std::unordered_map<permit, permit_private_status_t>> data;

  permit_private_status_t ool = permit_private_status::out_of_limits{};
  auto book = [&t0, &data, &ool, &opts](const region& loc, uint_t t) mutable -> permit_private_status_t& {
    if (t < t0) // XXX agents can check the state at t0, however they should be prohibited to bid for.
      return ool;
    if (opts.time_window && t > t0 + 1 + *opts.time_window)
      return ool;
    while (t - t0 >= data.size())
      data.emplace_back();
    return data[t - t0][{loc, t}];
  };

  auto public_access = [&book](auto id) {
    return [id = id, &book](const region& s, uint_t t) -> permit_public_status_t {
      using namespace permit_private_status;
      using namespace permit_public_status;
      return std::visit(cool::compose{[](out_of_limits) -> permit_public_status_t { return unavailable{}; },
                                      [&](in_use status) -> permit_public_status_t {
                                        return status.owner == id ? permit_public_status_t{owned{}} : unavailable{};
                                      },
                                      [&](on_sale status) -> permit_public_status_t {
                                        return status.owner == id ? permit_public_status_t{unavailable{}}
                                                                  : available{status.min_value};
                                      }},
                        book(s, t));
    };
  };

  const auto stop = [&] {
    using namespace stop_criteria;
    return std::visit(cool::compose{
                        [&](no_agents_t) { return agents.active_count() == 0; },
                        [&](time_threshold_t th) { return t0 > th.t; },
                      },
                      opts.stop_criteria);
  };

  do {
    {
      auto new_agents = factory(t0, space, rnd());
      for (auto& agent : new_agents)
        agents.insert(std::move(agent));
    }

    keep_active.clear();
    keep_active.reserve(agents.active_count());

    to_finished.clear();
    to_finished.reserve(agents.active_count());

    if (opts.status_callback)
      opts.status_callback(t0, std::as_const(agents), space,
                           [&book](const region& loc, uint_t t) -> permit_private_status_t { return book(loc, t); });

    {
      std::vector<std::tuple<region, uint_t>> bids;
      for (const auto id : accessor.active(agents)) {
        const auto r = accessor.at(agents, id)
                         .act(
                           t0,
                           [&](const region& s, uint_t t, value_t v) -> bool {
                             if (t < t0)
                               return false;
                             using namespace permit_private_status;
                             return std::visit(cool::compose{[](out_of_limits) { return false; }, [](in_use) { return false; },
                                                             [&](on_sale& status) {
                                                               if (v > status.min_value && v > status.highest_bid) {
                                                                 if (status.highest_bidder == no_owner)
                                                                   bids.emplace_back(s, t);

                                                                 status.highest_bidder = id;
                                                                 status.highest_bid = v;
                                                               }
                                                               return true;
                                                             }},
                                               book(s, t));
                           },
                           public_access(id), rnd());

        if (r)
          keep_active.push_back(id);
        else
          to_finished.push_back(id);
      }

      for (const auto& [s, t] : bids) {
        const auto status = std::get<permit_private_status::on_sale>(book(s, t));
        if (opts.trade_callback)
          opts.trade_callback({t0, status.owner, status.highest_bidder, s, t, status.highest_bid});

        accessor.at(agents, status.highest_bidder).on_bought(s, t, status.highest_bid);
        if (status.owner != no_owner)
          accessor.at(agents, status.owner).on_sold(s, t, status.highest_bid);

        book(s, t) = permit_private_status::in_use{status.highest_bidder};
      }
    }

    {
      std::vector<std::tuple<region, uint_t, uint_t, value_t>> asks;
      for (const auto id : accessor.active(agents)) {
        accessor.at(agents, id)
          .after_trading(
            t0,
            [&](const region& s, uint_t t, value_t v) -> bool {
              if (t < t0)
                return false;
              using namespace permit_private_status;
              return std::visit(cool::compose{[](out_of_limits) { return false; }, [](on_sale) { return false; },
                                              [&](in_use& status) {
                                                if (status.owner != id)
                                                  return false;
                                                asks.emplace_back(s, t, id, v);
                                                return true;
                                              }},
                                book(s, t));
            },
            public_access(id), rnd());
      }

      for (const auto& [s, t, id, v] : asks)
        book(s, t) = permit_private_status::on_sale{.owner = id, .min_value = v};
    }

    for (const auto id : to_finished)
      accessor.at(agents, id).on_finished(id, t0);

    agents.update_active(std::move(keep_active));

    data.pop_front();
    ++t0;
  } while (!stop());
}

} // namespace uat
