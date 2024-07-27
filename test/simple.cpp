#include <uat/simulation.hpp>

#include <cool/indices.hpp>
#include <jules/array/array.hpp>
#include <jules/base/random.hpp>

#include <limits>
#include <random>
#include <variant>

using namespace uat;

struct mission_t
{
  region from, to;
};

class my_agent
{
public:
  explicit my_agent(mission_t mission) : mission_{std::move(mission)} {}

  auto bid_phase(uint_t t, bid_fn bid, permit_public_status_fn status, int seed)
  {
    using namespace permit_public_status;

    // otherwise, it tries to buy in the next time
    if (std::holds_alternative<available>(status(mission_.from, t + 1)) &&
        std::holds_alternative<available>(status(mission_.to, t + 2))) {
      // note: in a real simulation there would exist intermediate regions
      std::mt19937 gen(seed);
      const auto price = 1.0 + jules::canon_sample(gen);
      bid(mission_.from, t + 1, price);
      bid(mission_.to, t + 2, price);
      remaining_ = 2;
    }
  }

  auto on_bought(const region&, uint_t, value_t) { --remaining_; }

  auto stop(uint_t, int) { return remaining_ == 0; }

private:
  mission_t mission_;
  uint_t remaining_ = std::numeric_limits<uint_t>::max();
};

// unidimensional space with 10 regions
class my_region
{
public:
  explicit my_region(std::size_t pos) : pos_{pos} {}

  auto hash() const -> std::size_t { return pos_; }

  auto operator==(const my_region& other) const { return pos_ == other.pos_; }

private:
  std::size_t pos_;
};

static auto random_mission(int seed) -> mission_t
{
  std::mt19937 gen(seed);
  const auto from = jules::uniform_index_sample(9u, gen);
  const auto to = from + 1;
  return {my_region{from}, my_region{to}};
}

using atraits = agent_traits<my_agent>;
static_assert(atraits::has_mb_bid_phase);
static_assert(!atraits::has_mb_ask_phase);
static_assert(atraits::has_mb_on_bought);
static_assert(!atraits::has_mb_on_sold);

int main()
{
  static constexpr auto n = 100u;
  static constexpr auto lambda = 10u;

  auto factory = [](uint_t t, int seed) -> std::vector<agent> {
    if (t >= n)
      return {};

    std::mt19937 gen(seed);

    std::vector<agent> result;
    result.reserve(lambda);
    for ([[maybe_unused]] const auto _ : cool::indices(lambda))
      result.push_back(my_agent(random_mission(gen())));

    return result;
  };

  jules::vector<value_t> cost(n * lambda);

  simulation_opts_t opts;
  opts.trade_callback = [&](trade_info_t info) {
    if (info.from != no_owner)
      cost[info.from] -= info.value;
    cost[info.to] += info.value;
  };

  simulate(factory, 17, opts);

  const auto [mean, sd] = jules::meansd(cost);
  fmt::print("Average cost: {} ± {}\n", mean, sd);
  fmt::print("Cost range: {}, {}\n", jules::min(cost), jules::max(cost));
}
