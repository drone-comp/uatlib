#include <uat/simulation.hpp>

#include <random>
#include <cool/indices.hpp>
#include <jules/base/random.hpp>
#include <jules/array/array.hpp>
#include <variant>

using namespace uat;

class my_agent
{
public:
  explicit my_agent(mission_t mission) : mission_{std::move(mission)} {}

  auto act(uint_t t, bid_fn bid, permit_public_status_fn status, int seed)
  {
    using namespace permit_public_status;
    // it stops if it owns the end points
    if (std::holds_alternative<owned>(status(mission_.from, t)) &&
        std::holds_alternative<owned>(status(mission_.to, t + 1)))
      return false;

    // otherwise, it tries to buy in the next time
    if (std::holds_alternative<available>(status(mission_.from, t + 1)) &&
        std::holds_alternative<available>(status(mission_.to, t + 2)))
    {
      // note: in a real simulation there would exist intermediate regions
      std::mt19937 gen(seed);
      const auto price = 1.0 + jules::canon_sample(gen);
      bid(mission_.from, t + 1, price);
      bid(mission_.to, t + 2, price);
    }

    return true; // keeps active
  }

private:
  mission_t mission_;
};

// unidimensional space with 10 regions
class my_region
{
public:
  explicit my_region(std::size_t pos) : pos_{pos} {}

  auto hash() const -> std::size_t { return pos_; }

  auto adjacent_regions() const -> std::vector<region>
  {
    if (pos_ == 0)
      return {my_region{pos_ + 1}};
    if (pos_ == 9)
      return {my_region{pos_ - 1}};
    return {my_region{pos_ - 1}, my_region{pos_ + 1}};
  }

  auto operator==(const my_region& other) const { return pos_ == other.pos_; }

  auto distance(const my_region& other) const { return pos_ > other.pos_ ? pos_ - other.pos_ : other.pos_ - pos_; }

private:
  std::size_t pos_;
};

class my_airspace
{
public:
  auto random_mission(int seed) const -> mission_t
  {
    std::mt19937 gen(seed);
    const auto from = jules::uniform_index_sample(9u, gen);
    const auto to = from + 1;
    return {my_region{from}, my_region{to}};
  }
};

int main()
{
  static constexpr auto n = 100u;
  static constexpr auto lambda = 10u;

  auto factory = [](uint_t t, const airspace& airspace, int seed) -> std::vector<agent> {
    if (t >= n)
      return {};

    std::mt19937 gen(seed);

    std::vector<agent> result;
    result.reserve(lambda);
    for ([[maybe_unused]] const auto _ : cool::indices(lambda))
      result.push_back(my_agent(airspace.random_mission(gen())));

    return result;
  };

  jules::vector<value_t> cost(n * lambda);

  simulation_opts_t opts;
  opts.trade_callback = [&](trade_info_t info) {
    if (info.from != no_owner)
      cost[info.from] -= info.value;
    cost[info.to] += info.value;
  };

  simulate(factory, my_airspace{}, 17, opts);

  const auto [mean, sd] = jules::meansd(cost);
  fmt::print("Average cost: {} ± {}\n", mean, sd);
  fmt::print("Cost range: {}, {}\n", jules::min(cost), jules::max(cost));
}
