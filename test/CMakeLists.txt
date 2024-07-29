if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
  file(DOWNLOAD
    https://github.com/catchorg/Catch2/releases/download/v2.1.0/catch.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp
    STATUS status
    LOG log)

  list(GET status 0 status_code)
  list(GET status 1 status_string)

  if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "error downloading catch: ${status_string}" "${log}")
  endif()
endif()

add_executable(simple simple.cpp)
set_target_properties(simple PROPERTIES CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_compile_features(simple PRIVATE cxx_std_20)

option(UAT_TEST_COVERAGE "whether or not add coverage instrumentation" OFF)
if (UAT_TEST_COVERAGE)
  target_compile_options(simple PRIVATE "$<$<CONFIG:DEBUG>:-O0>")
  target_compile_options(simple PRIVATE "$<$<CONFIG:DEBUG>:--coverage>")
  set_target_properties(simple PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
else()
  target_compile_options(simple PRIVATE "$<$<CONFIG:DEBUG>:-O0>")
  target_compile_options(simple PRIVATE "$<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>")
  target_compile_options(simple PRIVATE "$<$<CONFIG:DEBUG>:-fsanitize=address,leak,undefined>")
  set_target_properties(simple PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address,leak,undefined -fuse-ld=gold")
endif()

target_link_libraries(simple PRIVATE uat jules fmt)
target_compile_definitions(simple PRIVATE -DJULES_DEBUG_THROWS)

if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    target_compile_options(simple PRIVATE "/W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(simple PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
