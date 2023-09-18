cmake_minimum_required(VERSION 3.25.1)
project(MiniMalloc)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(external/abseil-cpp)
add_subdirectory(external/googletest)
add_executable(minimalloc
  src/converter.cc
  src/main.cc
  src/minimalloc.cc
  src/solver.cc
  src/sweeper.cc
  src/validator.cc
)
target_link_libraries(minimalloc
  absl::flags_parse
  absl::statusor
)

enable_testing()

add_executable(converter_test
  tests/converter_test.cc
  src/converter.cc
  src/minimalloc.cc
)
target_link_libraries(converter_test
  GTest::gmock_main
  GTest::gtest_main
  absl::flags
  absl::statusor
)
add_test(NAME converter_test COMMAND converter_test)

add_executable(minimalloc_test
  tests/minimalloc_test.cc
  src/minimalloc.cc
)
target_link_libraries(minimalloc_test
  GTest::gmock_main
  GTest::gtest_main
  absl::statusor
)
add_test(NAME minimalloc_test COMMAND minimalloc_test)

add_executable(solver_test
  tests/solver_test.cc
  src/minimalloc.cc
  src/solver.cc
  src/sweeper.cc
)
target_link_libraries(solver_test
  GTest::gmock_main
  GTest::gtest_main
  absl::flags
  absl::statusor
)
add_test(NAME solver_test COMMAND solver_test)

add_executable(sweeper_test
  tests/sweeper_test.cc
  src/minimalloc.cc
  src/sweeper.cc
)
target_link_libraries(sweeper_test
  GTest::gtest_main
  absl::flags
  absl::statusor
)
add_test(NAME sweeper_test COMMAND sweeper_test)

add_executable(validator_test
  tests/validator_test.cc
  src/minimalloc.cc
  src/validator.cc
)
target_link_libraries(validator_test
  GTest::gtest_main
  absl::statusor
)
add_test(NAME validator_test COMMAND validator_test)

include(GoogleTest)
