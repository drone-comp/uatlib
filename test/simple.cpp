#include <fmt/core.h>
#include <jules/base/random.hpp>
#include <uat/simulation.hpp>
#include <unordered_set>

struct Point
{
  std::size_t x, y;
  auto operator==(const Point& other) const noexcept -> bool { return x == other.x and y == other.y; }
  auto operator!=(const Point& other) const noexcept -> bool { return !(*this == other); }
};

template <> struct std::hash<Point>
{
  auto operator()(const Point& p) const noexcept -> std::size_t
  {
    size_t seed = 0;
    boost::hash_combine(seed, p.x);
    boost::hash_combine(seed, p.y);
    return seed;
  }
};

class Agent : public uat::agent<Point>
{
public:
  Agent(int seed)
  {
    // Let's consider the airspace as a 3x3 grid.
    jules::random_engine rng(seed);

    while (goals_.size() < 3) {
      const auto x = rng.uniform_index_sample(3u);
      const auto y = rng.uniform_index_sample(3u);
      goals_.insert({x, y});
    }
  }

  auto stop(uat::uint_t, int) -> bool override { return goals_.size() == owned_.size(); }

  auto bid_phase(uat::uint_t time, uat::bid_fn bid, uat::permit_public_status_fn status, int seed) -> void override
  {
    // Check at which time all goals are available.
    uat::uint_t target_time = time + 1;
    jules::random_engine rng(seed);
    std::uniform_int_distribution<uat::uint_t> dist(1, 5);

    while (true) {
      bool all_goals_available = true;
      for (const auto& goal : goals_) {
        if (not std::holds_alternative<uat::permit_public_status::available>(status(goal, target_time))) {
          all_goals_available = false;
          break;
        }
      }
      if (all_goals_available)
        break;
      target_time += rng.sample(dist);
    }

    for (const auto& goal : goals_)
      bid(goal, target_time, rng.canon_sample());
  }

  auto ask_phase(uat::uint_t, uat::ask_fn ask, uat::permit_public_status_fn, int) -> void override
  {
    if (owned_.size() == goals_.size())
      return; // Do not sell permits if all goals are achieved.

    for (const auto& [location, time] : owned_)
      ask(location, time, 0);
    owned_.clear();
  }

  auto on_bought(const Point& location, uat::uint_t time, uat::value_t cost) -> void override
  {
    owned_.insert({location, time});
    cost_ += cost;
  }

  auto on_sold(const Point&, uat::uint_t, uat::value_t revenue) -> void override
  {
    // We have already cleared the owned_ set in the ask_phase method.
    // No need to remove the permit from the set here.
    cost_ -= revenue;
  }

private:
  std::unordered_set<Point> goals_;
  std::unordered_set<uat::permit<Point>> owned_;
  uat::value_t cost_ = 0;
};

int main()
{
  uat::simulate<Point>({.factory = [](uat::uint_t time, int seed) -> std::vector<uat::any_agent> {
                          // Create 10 agents at time 0.
                          if (time > 0)
                            return {};

                          std::vector<uat::any_agent> agents;
                          std::mt19937 rng(seed);

                          while (agents.size() < 10)
                            agents.push_back(Agent(rng()));

                          return agents;
                        },
                        .trade_callback = [](uat::trade_info_t<Point> trade) -> void {
                          fmt::print("@{}: agent {} bought permit at ({}, {}, {}) for {}", trade.transaction_time, trade.to,
                                     trade.location.x, trade.location.y, trade.time, trade.value);
                          if (trade.from == uat::no_owner)
                            fmt::print("\n");
                          else
                            fmt::print(" from agent {}\n", trade.from);
                        }});
}
