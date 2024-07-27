#ifndef UAT_SIMULATION_HPP
#define UAT_SIMULATION_HPP

#include <uat/agent.hpp>
#include <uat/airspace.hpp>
#include <uat/permit.hpp>
#include <uat/type.hpp>

#include <deque>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace uat
{

using factory_fn = std::function<std::vector<agent>(uint_t, const airspace&, int)>;

struct trade_info_t
{
  uint_t transaction_time;
  uint_t from;
  uint_t to;
  region location;
  uint_t time;
  value_t value;
};

constexpr auto no_owner = std::numeric_limits<uint_t>::max();
namespace permit_private_status
{
struct on_sale
{
  uint_t owner = no_owner;
  value_t min_value = 1.0;
  uint_t highest_bidder = no_owner;
  value_t highest_bid = 0.0;
};
struct in_use
{
  uint_t owner;
};
struct out_of_limits
{};
} // namespace permit_private_status

struct permit_private_status_t
{
  std::variant<permit_private_status::on_sale, permit_private_status::in_use, permit_private_status::out_of_limits> current;
  std::vector<trade_value_t> history;
};

namespace agent_private_status
{
struct inactive
{
  id_t id;
};
struct active
{
  id_t id;
  agent data;
};
struct out_of_limits
{};
} // namespace agent_private_status
using agent_private_status_t =
  std::variant<agent_private_status::out_of_limits, agent_private_status::inactive, agent_private_status::active>;

class agents_private_status_fn
{
  friend class agents_private_status_accessor;

public:
  auto operator()(id_t) const -> agent_private_status_t;
  auto active_count() const -> uint_t;

  void insert(agent);
  void update_active(std::vector<id_t>);

private:
  uint_t first_id_ = 0u;
  std::deque<agent> agents_;
  std::vector<id_t> active_;
};

// TODO: is it possible to use function_ref?
using permit_private_status_fn = std::function<permit_private_status_t(const region&, uint_t)>;
using trade_info_fn = std::function<void(trade_info_t)>;
using status_info_fn = std::function<void(uint_t, const agents_private_status_fn&, const airspace&, permit_private_status_fn)>;

namespace stop_criteria
{
struct no_agents_t
{};
struct time_threshold_t
{
  uint_t t;
};
} // namespace stop_criteria

using stop_criteria_t = std::variant<stop_criteria::no_agents_t, stop_criteria::time_threshold_t>;

struct simulation_opts_t
{
  std::optional<uint_t> time_window;
  stop_criteria_t stop_criteria;
  trade_info_fn trade_callback;
  status_info_fn status_callback;
};

// (First-price sealed-bid auction)
auto simulate(factory_fn, airspace, int seed, const simulation_opts_t& = {}) -> void;

} // namespace uat

#endif // UAT_SIMULATION_HPP
