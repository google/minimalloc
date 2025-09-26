/*
Copyright 2023 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "../src/solver.h"

#include <functional>
#include <tuple>
#include <vector>

#include "../src/minimalloc.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"

namespace minimalloc {
namespace {

TEST(PreorderingComparator, ComparesCorrectly) {
  PreorderData data_a = {.area = 1, .total = 3, .width = 2, .buffer_idx = 0};
  PreorderData data_b = {.area = 0, .total = 4, .width = 1, .buffer_idx = 0};
  PreorderData data_c = {.area = 0, .total = 3, .width = 3, .buffer_idx = 0};
  PreorderData data_d = {.area = 2, .total = 3, .width = 2, .buffer_idx = 0};
  PreorderData data_e = {.area = 1, .total = 3, .width = 2, .buffer_idx = 1};
  PreorderingComparator preordering_comparator("TWA");
  EXPECT_TRUE(preordering_comparator(data_b, data_a));
  EXPECT_TRUE(preordering_comparator(data_c, data_a));
  EXPECT_TRUE(preordering_comparator(data_d, data_a));
  EXPECT_TRUE(preordering_comparator(data_a, data_e));
}

SolverParams getDisabledParams() {
  return {
    .canonical_only = false,
    .section_inference = false,
    .dynamic_ordering = false,
    .check_dominance = false,
    .unallocated_floor = false,
    .static_preordering = false,
    .dynamic_decomposition = false,
    .monotonic_floor = false,
    .hatless_pruning = false,
    .minimize_capacity = false,
    .preordering_heuristics = {"TWA"},
  };
}

// Checks that the solver produces the expected result for each possible
// combination of parameters.
class SolverTest
    : public ::testing::TestWithParam<std::tuple<
          CanonicalOnlyParam, SectionInferenceParam, DynamicOrderingParam,
          CheckDominanceParam, UnallocatedFloorParam, StaticPreorderingParam,
          DynamicDecompositionParam, MonotonicFloorParam, MinimizeCapacity>> {
 protected:
  void test_feasible(const Problem& problem) {
    Solver solver(getParams());
    const auto solution = solver.Solve(problem);
    EXPECT_EQ(solution.status().code(), absl::StatusCode::kOk);
  }

  void test_infeasible(const Problem& problem) {
    Solver solver(getParams());
    const auto solution = solver.Solve(problem);
    EXPECT_EQ(solution.status().code(), absl::StatusCode::kNotFound);
    EXPECT_GT(solver.get_backtracks(), 0);
  }

  SolverParams getParams(
      absl::Duration timeout = absl::InfiniteDuration()) {
    return SolverParams{
        .timeout = timeout,
        .canonical_only = std::get<0>(GetParam()),
        .section_inference = std::get<1>(GetParam()),
        .dynamic_ordering = std::get<2>(GetParam()),
        .check_dominance = std::get<3>(GetParam()),
        .unallocated_floor = std::get<4>(GetParam()),
        .static_preordering = std::get<5>(GetParam()),
        .dynamic_decomposition = std::get<6>(GetParam()),
        .monotonic_floor = std::get<7>(GetParam()),
        .hatless_pruning = false,
        .minimize_capacity = std::get<8>(GetParam()),
    };
  }
};

INSTANTIATE_TEST_SUITE_P(
    SolverTest, SolverTest,
    ::testing::Combine(::testing::Bool(), ::testing::Bool(), ::testing::Bool(),
                       ::testing::Bool(), ::testing::Bool(), ::testing::Bool(),
                       ::testing::Bool(), ::testing::Bool(), ::testing::Bool()));

TEST_P(SolverTest, InfeasibleBufferTooBig) {
  const Problem problem = {
    .buffers = {{.lifespan = {0, 2}, .size = 3}},
    .capacity = 2
  };
  test_infeasible(problem);
}

TEST_P(SolverTest, InfeasibleTrivial) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 2}, .size = 2},
        {.lifespan = {0, 2}, .size = 2},
    },
    .capacity = 3
  };
  test_infeasible(problem);
}

TEST_P(SolverTest, InfeasibleTricky) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 1}, .size = 3},
        {.lifespan = {0, 3}, .size = 1},
        {.lifespan = {4, 5}, .size = 3},
        {.lifespan = {2, 5}, .size = 1},
        {.lifespan = {1, 2}, .size = 2},
        {.lifespan = {3, 4}, .size = 2},
        {.lifespan = {1, 4}, .size = 1},
    },
    .capacity = 4
  };
  test_infeasible(problem);
}

TEST_P(SolverTest, EmptyProblem) {
  const Problem problem;
  test_feasible(problem);
}

TEST_P(SolverTest, SingleBuffer) {
  const Problem problem = {
    .buffers = {{.lifespan = {0, 2}, .size = 2}},
    .capacity = 2
  };
  test_feasible(problem);
}

TEST_P(SolverTest, TwoBuffers) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 2}, .size = 2},
        {.lifespan = {1, 3}, .size = 2},
    },
    .capacity = 4
  };
  test_feasible(problem);
}

TEST_P(SolverTest, FiveBuffers) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {1, 2}, .size = 1},
        {.lifespan = {0, 2}, .size = 1},
        {.lifespan = {2, 3}, .size = 2},
        {.lifespan = {1, 3}, .size = 1},
        {.lifespan = {0, 1}, .size = 2},
    },
    .capacity = 3
  };
  test_feasible(problem);
}

TEST_P(SolverTest, FixedBufferFeasible) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {1, 2}, .size = 1},
        {.lifespan = {0, 2}, .size = 1},
        {.lifespan = {2, 3}, .size = 2, .offset = 1},
        {.lifespan = {1, 3}, .size = 1},
        {.lifespan = {0, 1}, .size = 2},
    },
    .capacity = 3
  };
  Solver solver(getParams());
  const auto solution = solver.Solve(problem);
  EXPECT_EQ(solution->offsets[2], 1);
}

TEST_P(SolverTest, FixedBufferInfeasible) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {1, 2}, .size = 1, .offset = 0},
        {.lifespan = {0, 2}, .size = 1},
        {.lifespan = {2, 3}, .size = 2},
        {.lifespan = {1, 3}, .size = 1},
        {.lifespan = {0, 1}, .size = 2},
    },
    .capacity = 3
  };
  test_infeasible(problem);
}

TEST_P(SolverTest, TwoPartitions) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 2}, .size = 2},
        {.lifespan = {1, 3}, .size = 2},
        {.lifespan = {3, 5}, .size = 2},
        {.lifespan = {4, 6}, .size = 2},
     },
    .capacity = 4
  };
  test_feasible(problem);
}

TEST_P(SolverTest, EvenAlignment) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 2}, .size = 1, .alignment = 2},
        {.lifespan = {0, 2}, .size = 1, .alignment = 2},
    },
    .capacity = 4
  };
  test_feasible(problem);
}

TEST_P(SolverTest, BuffersWithGaps) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 4}, .size = 2, .gaps = {{.lifespan = {1, 3}}}},
        {.lifespan = {1, 3}, .size = 2},
    },
    .capacity = 2
  };
  test_feasible(problem);
}

TEST_P(SolverTest, Tetris) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {0, 5},
                                                   .window = {{0, 1}}}}},
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {5, 10},
                                                   .window = {{1, 2}}}}},
     },
    .capacity = 3
  };
  test_feasible(problem);
}

TEST_P(SolverTest, Stairs) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 108}, .size = 30, .gaps = {{.lifespan = {36, 72},
                                                     .window = {{10, 30}}},
                                                    {.lifespan = {72, 108},
                                                     .window = {{20, 30}}}}},
        {.lifespan = {36, 144}, .size = 50, .gaps = {{.lifespan = {36, 72},
                                                      .window = {{20, 30}}},
                                                     {.lifespan = {72, 108},
                                                      .window = {{10, 40}}}}},
        {.lifespan = {84, 144}, .size = 42, .gaps = {{.lifespan = {114, 129},
                                                      .window = {{0, 28}}},
                                                     {.lifespan = {129, 144},
                                                      .window = {{0, 14}}}}},
        {.lifespan = {84, 129}, .size = 42, .gaps = {{.lifespan = {99, 114},
                                                      .window = {{14, 42}}},
                                                     {.lifespan = {114, 129},
                                                      .window = {{28, 42}}}}},
        {.lifespan = {99, 144}, .size = 70, .gaps = {{.lifespan = {99, 114},
                                                      .window = {{28, 42}}},
                                                     {.lifespan = {114, 129},
                                                      .window = {{14, 56}}}}},
        {.lifespan = {0, 144}, .size = 30, .gaps = {{.lifespan = {72, 108},
                                                     .window = {{0, 20}}},
                                                    {.lifespan = {108, 144},
                                                     .window = {{0, 10}}}}},
     },
    .capacity = 144
  };
  test_feasible(problem);
}

TEST(SolverTest, CountsBacktracks) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 2}, .size = 2},
        {.lifespan = {0, 2}, .size = 2},
    },
    .capacity = 3
  };
  Solver solver(getDisabledParams());
  EXPECT_EQ(solver.Solve(problem).status().code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(solver.get_backtracks(), 3);
  // Now solve it again to see if it resets.
  EXPECT_EQ(solver.Solve(problem).status().code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(solver.get_backtracks(), 3);
}

using ReducesBacktracksTest =
    testing::TestWithParam<std::function<void(SolverParams&)>>;

INSTANTIATE_TEST_SUITE_P(
    ReducesBacktracksTests, ReducesBacktracksTest,
    testing::ValuesIn<std::function<void(SolverParams&)>>({
        [](auto& params) { params.canonical_only = true; },
        [](auto& params) { params.section_inference = true; },
        [](auto& params) { params.dynamic_ordering = true; },
        [](auto& params) { params.check_dominance = true; },
        [](auto& params) { params.static_preordering = true; },
        [](auto& params) { params.dynamic_decomposition = true; },
    }));

TEST_P(ReducesBacktracksTest, ReducesBacktracks) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {2, 3}, .size = 2},
        {.lifespan = {0, 1}, .size = 2},
        {.lifespan = {1, 2}, .size = 1},
        {.lifespan = {0, 2}, .size = 1},
        {.lifespan = {1, 3}, .size = 1},
    },
    .capacity = 3
  };

  SolverParams params = getDisabledParams();
  GetParam()(params);  // Update the appropriate parameter being tested.
  Solver solver(params);
  EXPECT_THAT(solver.Solve(problem).status().code(), absl::StatusCode::kOk);

  Solver disabled_solver(getDisabledParams());
  EXPECT_THAT(disabled_solver.Solve(problem).status().code(),
      absl::StatusCode::kOk);
  EXPECT_GT(disabled_solver.get_backtracks(), solver.get_backtracks());
}

TEST(SolverTest, ComputeIrreducibleInfeasibleSubset) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 2}, .size = 2},  // Not part of the IIS.
        {.lifespan = {0, 2}, .size = 2},  // Not part of the IIS.
        {.lifespan = {2, 5}, .size = 2},  // Part of the IIS.
        {.lifespan = {3, 6}, .size = 2},  // Part of the IIS.
        {.lifespan = {4, 7}, .size = 2},  // Part of the IIS.
    },
    .capacity = 4
  };
  Solver solver;
  auto subset = solver.ComputeIrreducibleInfeasibleSubset(problem);
  std::vector<minimalloc::BufferIdx> expected_subset = {2, 3, 4};
  EXPECT_THAT(*subset, expected_subset);
}

}  // namespace
}  // namespace minimalloc
