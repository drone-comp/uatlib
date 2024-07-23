#include <uat/py.hpp>

namespace uat
{

auto py_agent::bid_phase(uint_t t, bid_fn bid, permit_public_status_fn status, int seed) -> void {
  if (py::hasattr(agent, "bid_phase"))
    agent.attr("bid_phase")(t, bid, status, seed);
}

auto py_agent::ask_phase(uint_t t, ask_fn ask, permit_public_status_fn status, int seed) -> void {
  if (py::hasattr(agent, "ask_phase"))
    agent.attr("ask_phase")(t, ask, status, seed);
}

auto py_agent::on_bought(const region& region, uint_t t, value_t value) -> void {
  if (py::hasattr(agent, "on_bought"))
    agent.attr("on_bought")(region, t, value);
}

auto py_agent::on_sold(const region& region, uint_t t, value_t value) -> void {
  if (py::hasattr(agent, "on_sold"))
    agent.attr("on_sold")(region, t, value);
}

auto py_agent::stop(uint_t t, int seed) -> bool {
  if (py::hasattr(agent, "stop"))
    return agent.attr("stop")(t, seed).cast<bool>();

  return false;
}

} // namespace uat
