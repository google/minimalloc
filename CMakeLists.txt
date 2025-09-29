cmake_minimum_required(VERSION 3.25.1)
project(MiniMalloc)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure PIC for Python module compatibility
if(SKBUILD)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

add_subdirectory(external/abseil-cpp)
add_subdirectory(external/googletest)

# Create a library from your core sources (excluding main.cc)
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
target_link_libraries(minimalloc
  minimalloc_core
  absl::flags_parse
  absl::log
  absl::log_initialize
  absl::statusor
)

# Python bindings (only when building via pip/scikit-build)
if(SKBUILD)
  find_package(pybind11 CONFIG REQUIRED)
  
  pybind11_add_module(_minimalloc MODULE python/minimalloc/_bindings.cpp)
  target_link_libraries(_minimalloc PRIVATE minimalloc_core)
  
  install(TARGETS _minimalloc DESTINATION minimalloc)
endif()

# Tests (skip when building Python module)
if(NOT SKBUILD)
  enable_testing()
  
  add_executable(converter_test
    tests/converter_test.cc
  )
  target_link_libraries(converter_test
    minimalloc_core
    GTest::gmock_main
    GTest::gtest_main
  )
  add_test(NAME converter_test COMMAND converter_test)
  
  add_executable(minimalloc_test
    tests/minimalloc_test.cc
  )
  target_link_libraries(minimalloc_test
    minimalloc_core
    GTest::gmock_main
    GTest::gtest_main
  )
  add_test(NAME minimalloc_test COMMAND minimalloc_test)
  
  add_executable(solver_test
    tests/solver_test.cc
  )
  target_link_libraries(solver_test
    minimalloc_core
    GTest::gmock_main
    GTest::gtest_main
  )
  add_test(NAME solver_test COMMAND solver_test)
  
  add_executable(sweeper_test
    tests/sweeper_test.cc
  )
  target_link_libraries(sweeper_test
    minimalloc_core
    GTest::gtest_main
  )
  add_test(NAME sweeper_test COMMAND sweeper_test)
  
  add_executable(validator_test
    tests/validator_test.cc
  )
  target_link_libraries(validator_test
    minimalloc_core
    GTest::gtest_main
  )
  add_test(NAME validator_test COMMAND validator_test)
  
  include(GoogleTest)
endif()
