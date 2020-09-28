#ifndef UAT_SIMULATION_HPP
#define UAT_SIMULATION_HPP

#include <uat/type.hpp>
#include <uat/agent.hpp>
#include <uat/airspace.hpp>
#include <uat/slot.hpp>

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
  slot s;
  uint_t t;
  value_t value;
};

using trade_callback_t = std::function<void(trade_info_t)>;

// (First-price sealed-bid auction)
auto simulate(factory_t, airspace, int seed, trade_callback_t) -> void;

} // namespace uat

#endif // UAT_SIMULATION_HPP
