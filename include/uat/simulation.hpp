#ifndef UAT_SIMULATION_HPP
#define UAT_SIMULATION_HPP

#include <uat/type.hpp>
#include <uat/agent.hpp>
#include <uat/airspace.hpp>
#include <uat/permit.hpp>

#include <functional>
#include <vector>

namespace uat
{

constexpr auto no_owner = std::numeric_limits<uint_t>::max();

using factory_t = std::function<std::vector<agent>(uint_t, const airspace&, int)>;

struct trade_info_t {
  uint_t transaction_time;
  uint_t from;
  uint_t to;
  region location;
  uint_t time;
  value_t value;
};

using trade_callback_t = std::function<void(trade_info_t)>;

struct no_agents_t {};
struct time_threshold_t {
  uint_t t;
};

using stop_criteria_t = std::variant<no_agents_t, time_threshold_t>;

struct onsale {
  uint_t owner = no_owner;
  value_t min_value = 1.0;

  uint_t highest_bidder = no_owner;
  value_t highest_bid = 0.0;
};

struct used {
  uint_t owner;
};

struct out_of_limits {
};

using private_status = std::variant<onsale, used, out_of_limits>;

using private_status_t = std::function<private_status(const region&, uint_t)>;
using status_callback_t = std::function<void(uint_t, const airspace&, private_status_t)>;

struct simulation_opts_t {
  std::optional<uint_t> time_window;
  stop_criteria_t stop_criteria;
  trade_callback_t trade_callback;
  status_callback_t status_callback;
};

// (First-price sealed-bid auction)
auto simulate(factory_t, airspace, int seed, const simulation_opts_t& = {}) -> void;

} // namespace uat

#endif // UAT_SIMULATION_HPP
