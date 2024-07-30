//! \file simulation.hpp
//! \brief Defines the simulation function and related types.

#ifndef UAT_SIMULATION_HPP
#define UAT_SIMULATION_HPP

#include <uat/agent.hpp>

#include <deque>
#include <optional>
#include <random>
#include <vector>

#include <cool/compose.hpp>

namespace uat
{

//! A function type that generates agents for each iteration.
using factory_fn = std::function<std::vector<any_agent>(uint_t, int)>;

//! Type to represent the information in a trade transaction.
template <region_compatible R> struct trade_info_t
{
  uint_t transaction_time;
  uint_t from;
  uint_t to;
  R location;
  uint_t time;
  value_t value;
};

//! Unique value to represent the absence of an owner.
constexpr auto no_owner = std::numeric_limits<uint_t>::max();

namespace permit_private_status
{

//! Represents the private status of a permit that is available for trading.
struct on_sale
{
  uint_t owner = no_owner;          //!< The owner of the permit.
  value_t min_value = 0.0;          //!< The minimum value (exclusive) that the owner is willing to sell the permit.
  uint_t highest_bidder = no_owner; //!< The current highest bidder.
  value_t highest_bid = 0.0;        //!< The current highest bid.
};

//! Represents the private status of a permit that is not available for trading.
struct in_use
{
  uint_t owner;
};

//! Represents the private status of a permit that is out of limits (past and future).
struct out_of_limits
{};

} // namespace permit_private_status

//! Represents the private status of a permit.
struct permit_private_status_t
{
  //! The current status of the permit.
  std::variant<permit_private_status::on_sale, permit_private_status::in_use, permit_private_status::out_of_limits> current;

  //! The history of trades involving the permit.
  std::vector<trade_value_t> history;
};

namespace agent_private_status
{

//! Represents the private status of an agent that is inactive in the simulation.
struct inactive
{
  id_t id;
};

//! Represents the private status of an agent that is active in the simulation.
struct active
{
  id_t id;
  any_agent data;
};

} // namespace agent_private_status

//! Variant that represents the private status of an agent.
using agent_private_status_t = std::variant<agent_private_status::inactive, agent_private_status::active>;

//! Functor type that allows the simulation to access the private status of an agent.
class agents_private_status_fn
{
  friend class agents_private_status_accessor;

public:
  auto operator()(id_t) const -> agent_private_status_t;
  auto active_count() const -> uint_t;
  auto active() const -> const std::vector<id_t>&;

private:
  void insert(any_agent);
  void update_active(std::vector<id_t>);

  uint_t first_id_ = 0u;
  std::deque<any_agent> agents_;
  std::vector<id_t> active_;
};

// TODO: is it possible to use function_ref?

//! Function type that allows the simulation to access the private status of an agent.
using permit_private_status_fn = std::function<permit_private_status_t(region_view, uint_t)>;

//! Callback type that receives information about a trade transaction.
template <region_compatible R> using trade_info_fn = std::function<void(trade_info_t<R>)>;

//! Callback type that receives information about the status of the simulation.
using status_info_fn = std::function<void(uint_t, const agents_private_status_fn&, permit_private_status_fn)>;

namespace stop_criterion
{

//! Stop criterion that stops the simulation when there are no agents.
struct no_agents_t
{};

//! Stop criterion that stops the simulation when a time threshold is reached.
struct time_threshold_t
{
  uint_t t;
};
} // namespace stop_criterion

//! Variant that represents the possible stop criteria for the simulation.
using stop_criterion_t = std::variant<stop_criterion::no_agents_t, stop_criterion::time_threshold_t>;

//! Options to configure the simulation.
template <region_compatible R> struct simulation_opts_t
{
  factory_fn factory;                //!< Function that generates agents for each iteration.
  std::optional<uint_t> time_window; //!< Maximum time ahead a permit can be traded.
  stop_criterion_t stop_criterion;   //!< The criterion to stop the simulation.
  trade_info_fn<R> trade_callback;   //!< Callback to receive information about a trade transaction.
  status_info_fn status_callback;    //!< Callback to receive information about the status of the simulation.
  std::optional<uint_t> seed;        //!< Random seed.
};

//! \private
struct agents_private_status_accessor
{
  auto active(const agents_private_status_fn&) const -> const std::vector<id_t>&;
  auto at(agents_private_status_fn&, id_t) const -> any_agent&;
  void insert(agents_private_status_fn&, any_agent) const;
  void update_active(agents_private_status_fn&, std::vector<id_t>) const;
};

//! A simulation of a first-price sealed-bid auction.
//!
//! \param factory A function that generates agents for each iteration.
//! \param seed A random seed.
//! \param opts Options to configure the simulation.
template <region_compatible R> auto simulate(const simulation_opts_t<R>& opts = {}) -> void
{
  std::mt19937 rnd(opts.seed ? *opts.seed : std::random_device{}());

  agents_private_status_fn agents;
  constexpr agents_private_status_accessor accessor;
  std::vector<id_t> keep_active;

  uint_t t0 = 0;

  std::deque<std::unordered_map<permit<R>, permit_private_status_t>> data;

  permit_private_status_t ool = {permit_private_status::out_of_limits{}, {}};
  auto book = [&t0, &data, &ool, &opts](region_view loc, uint_t t) mutable -> permit_private_status_t& {
    if (t < t0) // XXX agents can check the state at t0, however they should be prohibited to bid for.
      return ool;
    if (opts.time_window && t > t0 + 1 + *opts.time_window)
      return ool;
    while (t - t0 >= data.size())
      data.emplace_back();
    return data[t - t0][{loc.downcast<R>(), t}];
  };

  const auto safe_book = [&book](region_view loc, uint_t t) -> permit_private_status_t { return book(loc, t); };

  auto public_access = [&book](auto id) {
    return [id = id, &book](region_view s, uint_t t) -> permit_public_status_t {
      using namespace permit_private_status;
      using namespace permit_public_status;
      const auto& pstatus = book(s, t);
      using history_t = std::add_const_t<decltype(pstatus.history)>&;
      return std::visit(cool::compose{[](out_of_limits) -> permit_public_status_t { return unavailable{}; },
                                      [&](in_use status) -> permit_public_status_t {
                                        return status.owner == id ? permit_public_status_t{owned{}} : unavailable{};
                                      },
                                      [&](on_sale status) -> permit_public_status_t {
                                        return status.owner == id
                                                 ? permit_public_status_t{unavailable{}}
                                                 : available{status.min_value, [&]() -> history_t { return pstatus.history; }};
                                      }},
                        pstatus.current);
    };
  };

  const auto stop = [&] {
    using namespace stop_criterion;
    return std::visit(cool::compose{
                        [&](no_agents_t) { return agents.active_count() == 0; },
                        [&](time_threshold_t th) { return t0 > th.t; },
                      },
                      opts.stop_criterion);
  };

  do {
    if (opts.status_callback)
      opts.status_callback(t0, std::as_const(agents), safe_book);

    // Generate new agents
    if (opts.factory) {
      auto new_agents = opts.factory(t0, rnd());
      for (auto& agent : new_agents)
        accessor.insert(agents, std::move(agent));
    }

    {
      // Bid phase
      std::vector<permit<R>> bids;
      for (const auto id : accessor.active(agents)) {
        auto bid = [&](region_view s, uint_t t, value_t v) -> bool {
          if (t < t0)
            return false;
          using namespace permit_private_status;
          const auto visitor = cool::compose{[](out_of_limits) { return false; }, [](in_use) { return false; },
                                             [&](on_sale& status) {
                                               if (v > status.min_value && v > status.highest_bid) {
                                                 if (status.highest_bidder == no_owner)
                                                   bids.emplace_back(s.downcast<R>(), t);
                                                 status.highest_bidder = id;
                                                 status.highest_bid = v;
                                               }
                                               return true;
                                             }};
          return std::visit(visitor, book(s, t).current);
        };

        accessor.at(agents, id).bid_phase(t0, std::move(bid), public_access(id), rnd());
      }

      // Trading
      if (bids.size() > 0) {
        const auto first_active = accessor.active(agents).front();
        for (const auto& [s, t] : bids) {
          const auto status = std::get<permit_private_status::on_sale>(book(s, t).current);
          if (opts.trade_callback)
            opts.trade_callback({t0, status.owner, status.highest_bidder, s, t, status.highest_bid});

          accessor.at(agents, status.highest_bidder).on_bought(s, t, status.highest_bid);
          if (status.owner != no_owner && status.owner >= first_active)
            accessor.at(agents, status.owner).on_sold(s, t, status.highest_bid);

          auto& pstatus = book(s, t);
          pstatus.current = permit_private_status::in_use{status.highest_bidder};
          pstatus.history.push_back({status.min_value, status.highest_bid});
        }
      }
    }

    // Ask phase
    {
      std::vector<std::tuple<R, uint_t, uint_t, value_t>> asks;
      for (const auto id : accessor.active(agents)) {
        auto ask = [&](region_view s, uint_t t, value_t v) -> bool {
          if (t < t0)
            return false;
          using namespace permit_private_status;
          const auto visitor = cool::compose{[](out_of_limits) { return false; },
                                             [&](on_sale status) {
                                               if (status.owner != id)
                                                 return false;
                                               asks.emplace_back(s.downcast<R>(), t, id, v);
                                               return true;
                                             },
                                             [&](in_use& status) {
                                               if (status.owner != id)
                                                 return false;
                                               asks.emplace_back(s.downcast<R>(), t, id, v);
                                               return true;
                                             }};
          return std::visit(visitor, book(s, t).current);
        };

        accessor.at(agents, id).ask_phase(t0, std::move(ask), public_access(id), rnd());
      }

      for (const auto& [s, t, id, v] : asks)
        book(s, t).current = permit_private_status::on_sale{.owner = id, .min_value = v};
    }

    // Stop condition
    keep_active.clear();
    keep_active.reserve(agents.active_count());
    for (const auto id : accessor.active(agents))
      if (!accessor.at(agents, id).stop(t0, rnd()))
        keep_active.push_back(id);
    accessor.update_active(agents, std::move(keep_active));

    if (data.size() > 0)
      data.pop_front();
    ++t0;
  } while (!stop());
}

} // namespace uat

#endif // UAT_SIMULATION_HPP
