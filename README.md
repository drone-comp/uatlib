<img src="assets/logo.png"></img>

# uatlib: a C++ simulation library for Urban Airspace Tradable Permit Model

## Getting started

Create a `main.cpp` file with the following content:

```cpp
#include <uat/simulation.hpp>

int main() {
  // TODO
}
```

Then, clone this repository:

```bash
git clone https://github.com/drone-comp/uatlib.git
cd uatlib
git submodule update --init --recursive
cd ..
```

And include the library in your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)

project(my_project)

add_subdirectory(uatlib)

add_executable(my_project main.cpp)
target_link_libraries(my_project uat)
```

## Running the simulation

To run the simulation, first compile the project:

```bash
cmake -H. -Bbuild
cmake --build build
```

Then, run the executable:

```bash
./build/my_project
```
