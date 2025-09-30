cmake_minimum_required(VERSION 3.25.1)
project(MiniMalloc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PIC for Python module compatibility
if(SKBUILD)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# External dependencies
add_subdirectory(external/abseil-cpp EXCLUDE_FROM_ALL)
add_subdirectory(external/googletest EXCLUDE_FROM_ALL)

# Core library
add_library(minimalloc_core STATIC
  src/converter.cc
  src/minimalloc.cc
  src/solver.cc
  src/sweeper.cc
  src/validator.cc
)
target_link_libraries(minimalloc_core PUBLIC
  absl::flags
  absl::statusor
)
target_include_directories(minimalloc_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# C++ executable
add_executable(minimalloc src/main.cc)
target_link_libraries(minimalloc PRIVATE
  minimalloc_core
  absl::flags_parse
  absl::log
  absl::log_initialize
)

# Python bindings (only when building via pip/scikit-build)
if(SKBUILD)
  find_package(pybind11 CONFIG REQUIRED)

  pybind11_add_module(_minimalloc MODULE python/minimalloc/_bindings.cpp)
  target_link_libraries(_minimalloc PRIVATE minimalloc_core)

  install(TARGETS _minimalloc LIBRARY DESTINATION minimalloc)
endif()

# C++ tests (skip when building Python module)
if(NOT SKBUILD)
  enable_testing()

  foreach(test_name converter minimalloc solver sweeper validator)
    add_executable(${test_name}_test tests/${test_name}_test.cc)
    target_link_libraries(${test_name}_test PRIVATE
      minimalloc_core
      GTest::gtest_main
      GTest::gmock_main
    )
    add_test(NAME ${test_name}_test COMMAND ${test_name}_test)
  endforeach()
endif()
