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
    return x == other.x && y == other.y;
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
