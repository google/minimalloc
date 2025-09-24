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

# Python bindings (optional, only when building with Python)
if(DEFINED PYTHON_EXECUTABLE)
    # Find Python and pybind11
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    find_package(pybind11 REQUIRED)

    # Add the MiniMalloc core library
    add_library(minimalloc_core STATIC
        src/minimalloc.cc
        src/solver.cc
        src/sweeper.cc
        src/converter.cc
        src/validator.cc
    )
    target_include_directories(minimalloc_core PUBLIC src/)
    target_link_libraries(minimalloc_core
        absl::flags_parse
        absl::flags
        absl::statusor
        absl::status
        absl::strings
        absl::time
        absl::base
    )

    # Add the Python binding
    pybind11_add_module(_core python/minimalloc/_core.cpp)
    target_link_libraries(_core PRIVATE minimalloc_core)
    target_compile_definitions(_core PRIVATE VERSION_INFO="${PROJECT_VERSION}")

    # Set the output name and location
    set_target_properties(_core PROPERTIES
        OUTPUT_NAME "_core"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    )

    # Ensure proper RPATH/runpath for the shared library
    if(APPLE)
        set_target_properties(_core PROPERTIES INSTALL_RPATH "@loader_path")
    else()
        set_target_properties(_core PROPERTIES INSTALL_RPATH "$ORIGIN")
    endif()
endif()
