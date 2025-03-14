cmake_minimum_required(VERSION 3.10)
project(UAT VERSION 0.5.0)

add_subdirectory(external/fmt)
add_subdirectory(external/cool)
add_subdirectory(external/jules)
add_subdirectory(external/type_safe)

add_library(uat
  src/agent.cpp
  src/simulation.cpp
  include/uat/agent.hpp
  include/uat/simulation.hpp
  include/uat/permit.hpp
  include/uat/type.hpp)

target_compile_features(uat PRIVATE cxx_std_20)

target_include_directories(uat PUBLIC include)

find_package(Boost CONFIG)
target_link_libraries(uat PUBLIC cool type_safe)
target_include_directories(uat PUBLIC Boost::headers)
# target_link_libraries(uat PUBLIC jules fmt type_safe)

option(UAT_BUILD_TEST "whether or not to build the test" OFF)
if(UAT_BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
