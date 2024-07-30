# Tutorial

This tutorial is a step-by-step guide to creating a simple simulation of
a first-price sealed-bid auction using `uatlib`.  Each agent in the simulation
bids for permits to use a certain region of the airspace for a certain period
of time.

`uatlib` is a C++ library that enables users to describe the rules of the
airspace and the behavior of the agents in the simulation.  The library
provides a simulation engine that can be used to run the simulation and analyze
the results.

## Getting started

`uatlib` uses CMake to build the library and link it with the user's code.

To get started, create a new directory for the tutorial and initialize
a Git repository:

```bash
mkdir uatlib-tutorial
cd uatlib-tutorial
git init
```

Now, let's add `uatlib` as a submodule:

```bash
git submodule add https://github.com/drone-comp/uatlib.git
```

`uatlib` contains some third-party dependencies that are included as submodules
as well.  To initialize these submodules, run:

```bash
git submodule update --init --recursive
```

Now, we can create a minimal C++20 program that uses `uatlib`.  Create a new
file called `main.cpp` in the `uatlib-tutorial` directory with the following
contents:

```cpp
#include <uat/simulation.hpp>

struct Point {
  std::size_t x, y;
  auto operator==(const Point& other) const noexcept -> bool {
    return x == other.x and y == other.y;
  }
  auto operator!=(const Point& other) const noexcept -> bool {
    return !(*this == other);
  }
};

template <> struct std::hash<Point> {
  auto operator()(const Point& p) const noexcept -> std::size_t {
    size_t seed = 0;
    boost::hash_combine(seed, p.x);
    boost::hash_combine(seed, p.y);
    return seed;
  }
};

int main() {
  uat::simulate<Point>();
}
```

To describe our simulation, we first need to describe the type
that represents the locations in the airspace.  In the example above,
we use a simple `Point` structure with `x` and `y` coordinates.

The library requires that your custom type satisfies `uat::region_compatible`,
which is a concept that requires the type to be hashable, equality comparable,
and copyable.

The `std::hash` specialization and the operators `==` and `!=` are required to
use `Point` as a key in a `std::unordered_map` implicitly created by the
library.

The `main` function calls the `simulate` function with `Point` as the template
argument.  This function runs the simulation with the given type representing
the locations in the airspace.

As you should have noticed, this simulation does not do anything yet.  In the
next sections, we will describe how to create agents to participate in the
auction.

Before we continue, let's create a `CMakeLists.txt` file in the `uatlib-tutorial`
directory:

```cmake
cmake_minimum_required(VERSION 3.10)

project(uatlib-tutorial)

add_subdirectory(uatlib)

add_executable(uatlib-tutorial main.cpp)
target_link_libraries(uatlib-tutorial PRIVATE uatlib)
target_compile_features(uatlib-tutorial PRIVATE cxx_std_20)
```

This file tells CMake to build the `uatlib-tutorial` executable from the
`main.cpp` file and link it with the `uatlib` library.  The library
requires C++20 features, so we set the compile features accordingly.

Now, we can build the project:

```bash
cmake -H. -Bbuild
cmake --build build
```

If everything goes well, you should see the `uatlib-tutorial` executable in the
`build` directory.  Running this executable should not produce any output yet.

Tip: Add the following line to your `.gitignore` file to ignore the `build`
directory:

```
build
```

Do not forget to add and commit the changes to the repository:

```bash
git add main.cpp CMakeLists.txt .gitignore
git commit -m "First commit."
```

## Creating agents

In this section, we will create agents that participate in the auction.  Agent
implementations must inherit from the `uat::agent` class template, satisfy
`std::movable` and implement a few methods.

Let's create a simple agent that have a few goals in the airspace and bid for
permits to use them.  Add the following code to the `main.cpp` file before the
`main` function:

```cpp
class Agent : public uat::agent<Point> {
public:
  Agent(int seed) {
    // ...
  }

  auto stop(uat::uint_t time, int seed) -> void override {
    // ...
  }

private:
  std::unordered_set<Point> goals_;
  std::unordered_set<uat::permit<Point>> owned_;
  uat::value_t cost_ = 0;
};
```

So, the `Agent` class inherits from `uat::agent<Point>`. The only required
method to implement is the `stop` method, which is called every iteration
of the simulation.  An agent that returns true from the `stop` method is
removed from the simulation.

Our `Agent` class has three private members: `goals_`, `owned_`, and `cost_`.
The `goals_` member is a set of points that represent the goals of the agent.
The `owned_` member is a set of permits that the agent has successfully
acquired.  The `cost_` member is the total cost of the permits that the agent
has acquired minus the earnings from the permits that the agent has sold.

Note that `goals_` contains only regions of the airspace that the agent is
interested in.  The agent will bid for permits to use these regions for
different periods of time, depending when the agent enters the auction.

On the other hand, `owned_` contains elements of type `uat::permit`,
which is just a structure that holds the region and the time period that the
agent has acquired.

Do not forget to include the necessary headers:

```cpp
#include <unordered_set>
#include <jules/base/random.hpp>
```

To implement the constructor and the `stop` method, we are going to
use [jules](https://github.com/verri/jules), a header-only library that
provides a random number generator and a few other utilities.
It is already included as a submodule in `uatlib`.

Change the `CMakelists.txt` file to include the `jules` library:

```cmake
target_link_libraries(uatlib-tutorial PRIVATE uat jules)
```

Let's implement the constructor and the `agent::stop` method:

```cpp
Agent(int seed) {
  // Let's consider the airspace as a 3x3 grid.
  jules::random_device rng(seed);

  while (goals_.size() < 3) {
    const auto x = rng.uniform_index_sample(3u);
    const auto y = rng.uniform_index_sample(3u);
    goals_.insert({x, y});
  }
}

auto stop(uat::uint_t time, int seed) -> void override {
  return goals_.size() == owned_.size();
}
```

Our simple agent is created with three random goals in a 3x3 grid.  The agent
stops when it has acquired permits for all its goals.

But how an agent can acquire permits?  The library provides a method two
methods to control the actions of the agents: `agent::bid_phase` and
`agent::ask_phase`.  These methods are optional.  If the user does not
implement them, the agent won't do anything at that phase.

The `bid_phase` method is called in the beginning of each time step.  In this
method, the agent can bid for permits to use the regions of the airspace at
certain time that it is interested in.  All bids are processed using a
first-price sealed-bid auction.  Then, the agent can decide to sell permits
that it owns using the `ask_phase` method.

Let's implement the `bid_phase` method:

```cpp
auto bid_phase(uat::uint_t time, uat::bid_fn bid, uat::permit_public_status_fn status, int seed) -> void override {
  // Check at which time all goals are available.
  uat::uint_t target_time = time + 1;
  while (true) {
    for (const auto& goal : goals_) {
      bool = all_goals_available = true;
      if (not std::holds_alternative<uat::permit_public_status::available>(status(goal, target_time))) {
        all_goals_available = false;
        break;
      }
    }
    if (all_goals_available)
      break;
    ++target_time;
  }

  jules::random_device rng(seed);
  for (const auto& goal : goals_)
    bid(goal, target_time, rng.canon_sample());
}
```

In this function, we first find the time at which all goals are available.
Then, we bid for permits to use the regions of the airspace at that time.  The
bid amount is a random number between 0 and 1 generated by the
`jules::random_device` object.

The `bid` function (with type `uat::bid_fn`) is called with the region, the
time, and the bid amount.  The `status` function (with type `uat::permit_public_status_fn`)
return the status of the permit at the given region and time (check the
`permit_public_status_t` variant for the possible values).

Now, let's implement the `ask_phase` method:

```cpp
auto ask_phase(uat::uint_t time, uat::ask_fn ask, uat::permit_public_status_fn status, int) -> void override {
  if (owned_.size() == goals_.size())
    return; // Do not sell permits if all goals are achieved.

  for (const auto& [location, time] : owned_)
    ask(location, time, 0);
  owned_.clear();
}
```

In this function, we sell permits that we own.  We do not sell permits if all
goals are achieved.  Otherwise, we put the permits on sale with a minimum price
of 0.

Note that we clear the `owned_` set after selling the permits.  We do not actually
stop owning the permits, but we have not any interest in them anymore.  The
reason for this is that once we reach the ask phase and we have not achieved
all goals, it means that our bids were not successful.

How do we know if the agent has acquired a permit?  The library provides two
callbacks that are called when a permit is acquired or sold: `agent::on_bought`
and `agent::on_sold`.  These methods are optional.  If the user does not
implement them, the agent react when a permit is acquired or sold but the book
in the simulation will still consider the permits as owned by the agent.

Let's implement them:

```cpp
auto on_bought(const Point& location, uat::uint_t time, uat::value_t cost) -> void override {
  owned_.insert({location, time});
  cost_ += cost;
}

auto on_sold(const Point& location, uat::uint_t time, uat::value_t revenue) -> void override {
  // We have already cleared the owned_ set in the ask_phase method.
  // No need to remove the permit from the set here.
  cost_ -= revenue;
}
```

Now, we can build the project again:

```bash
cmake --build build
```

## Running the simulation

Well... now we have a simple agent, but we need to add it to the simulation.

Let's change the `main` function to create a factory that creates instances of
our agent for each iteration of the simulation:

```cpp
int main() {
  uat::simulate<Point>({
    .factory = [](uint_t time, int seed) -> std::vector<uat::any_agent> {
      // Create 10 agents at time 0.
      if (time > 0)
        return {};

      std::vector<uat::any_agent> agents;
      std::mt19937 rng(seed);

      while (agents.size() < 10)
        agents.push_back(Agent(rng()));

      return agents;
    }
  });
}
```

The `simulate` function now receives a configuration object with a `factory`
field.  This field is a function that receives the current time and a seed and
returns a vector of `uat::any_agent`.  The function is called at the beginning
of each iteration of the simulation.  The type `uat::any_agent` is a type-erased
wrapper around derivations of `uat::agent`.

What about the stop condition?  By default, the simulation stops when all agents
return true from the `stop` method.  If you want to change this behavior,
take a look at the `uat::simulate` function documentation.

Now, build the project again and run the simulation:

```bash
cmake --build build
./build/uatlib-tutorial
```

## Tracking trades

The library provides a way to track the trades that occur during the simulation.

Let's change the `main` function to track the trades:

```cpp
#include <fmt/core.h>

int main() {
  uat::simulate<Point>({
    .factory = [](uint_t time, int seed) -> std::vector<uat::any_agent> {
      // ...
    },
    .trade_callback = [](uat::trade_info_t trade) -> void {
      if (trade.from == uat::on_owner)
        fmt::print("@{}: agent {} bought permit at ({}, {}, {}) for {}\n",
                   trade.transaction_time, trade.to,
                   trade.location.x, trade.location.y, trade.time, trade.value);
      else
        fmt::print("@{}: agent {} bought permit at ({}, {}, {}) for {} from agent {}\n",
                   trade.transaction_time, trade.to,
                   trade.location.x, trade.location.y, trade.time,
                   trade.value, trade.from);
    }
  });
}
```

We have used the `fmt` library to format the output (which also is a dependency
of `uatlib`).  You can add it to the `CMakeLists.txt` file:

```cmake
target_link_libraries(uatlib-tutorial PRIVATE fmt)
```
