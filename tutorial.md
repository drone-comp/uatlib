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
