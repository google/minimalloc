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

#include "../src/tiny_solver.h"

#include <vector>

#include "../src/minimalloc.h"
#include "../src/validator.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gtest/gtest.h"

namespace minimalloc {
namespace {

class TinySolverTest : public ::testing::Test {
 protected:
  void test_feasible(const Problem& problem) {
    TinySolver solver;
    const auto solution = solver.Solve(problem);
    ASSERT_EQ(solution.status().code(), absl::StatusCode::kOk);
    EXPECT_EQ(Validate(problem, *solution), kGood);
  }

  void test_infeasible(const Problem& problem) {
    TinySolver solver;
    const auto solution = solver.Solve(problem);
    EXPECT_EQ(solution.status().code(), absl::StatusCode::kNotFound);
  }
};

TEST_F(TinySolverTest, EmptyProblem) {
  const Problem problem;
  test_feasible(problem);
}

TEST_F(TinySolverTest, InfeasibleBufferTooBig) {
  const Problem problem = {
      .buffers = {{.lifespan = {0, 2}, .size = 3}},
      .capacity = 2,
  };
  test_infeasible(problem);
}

TEST_F(TinySolverTest, InfeasibleAlignedBufferTooBig) {
  const Problem problem = {
      .buffers = {{.lifespan = {0, 2}, .size = 3, .alignment = 2}},
      .capacity = 2,
  };
  test_infeasible(problem);
}

TEST_F(TinySolverTest, InfeasibleTrivial) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 2}, .size = 2},
              {.lifespan = {0, 2}, .size = 2},
          },
      .capacity = 3,
  };
  test_infeasible(problem);
}

TEST_F(TinySolverTest, InfeasibleTricky) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 1}, .size = 3},
              {.lifespan = {0, 3}, .size = 1},
              {.lifespan = {4, 5}, .size = 3},
              {.lifespan = {2, 5}, .size = 1},
              {.lifespan = {1, 2}, .size = 2},
              {.lifespan = {3, 4}, .size = 2},
              {.lifespan = {1, 4}, .size = 1},
          },
      .capacity = 4,
  };
  test_infeasible(problem);
}

TEST_F(TinySolverTest, SingleBuffer) {
  const Problem problem = {
      .buffers = {{.lifespan = {0, 2}, .size = 2}},
      .capacity = 2,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, TwoBuffers) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 2}, .size = 2},
              {.lifespan = {1, 3}, .size = 2},
          },
      .capacity = 4,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, FiveBuffers) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {1, 2}, .size = 1},
              {.lifespan = {0, 2}, .size = 1},
              {.lifespan = {2, 3}, .size = 2},
              {.lifespan = {1, 3}, .size = 1},
              {.lifespan = {0, 1}, .size = 2},
          },
      .capacity = 3,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, FixedBufferFeasible) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {1, 2}, .size = 1},
              {.lifespan = {0, 2}, .size = 1},
              {.lifespan = {2, 3}, .size = 2, .offset = 1},
              {.lifespan = {1, 3}, .size = 1},
              {.lifespan = {0, 1}, .size = 2},
          },
      .capacity = 3,
  };
  TinySolver solver;
  const auto solution = solver.Solve(problem);
  ASSERT_EQ(solution.status().code(), absl::StatusCode::kOk);
  EXPECT_EQ(solution->offsets[2], 1);
  EXPECT_EQ(Validate(problem, *solution), kGood);
}

TEST_F(TinySolverTest, FixedBufferInfeasible) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {1, 2}, .size = 1, .offset = 0},
              {.lifespan = {0, 2}, .size = 1},
              {.lifespan = {2, 3}, .size = 2},
              {.lifespan = {1, 3}, .size = 1},
              {.lifespan = {0, 1}, .size = 2},
          },
      .capacity = 3,
  };
  test_infeasible(problem);
}

TEST_F(TinySolverTest, FixedBufferMisalignedInfeasible) {
  const Problem problem = {
      .buffers = {{.lifespan = {0, 2}, .size = 2, .alignment = 2, .offset = 1}},
      .capacity = 4,
  };
  test_infeasible(problem);
}

TEST_F(TinySolverTest, TwoPartitions) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 2}, .size = 2},
              {.lifespan = {1, 3}, .size = 2},
              {.lifespan = {3, 5}, .size = 2},
              {.lifespan = {4, 6}, .size = 2},
          },
      .capacity = 4,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, EvenAlignment) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 2}, .size = 1, .alignment = 2},
              {.lifespan = {0, 2}, .size = 1, .alignment = 2},
          },
      .capacity = 4,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, BuffersWithGaps) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 4}, .size = 2, .gaps = {{.lifespan = {1, 3}}}},
              {.lifespan = {1, 3}, .size = 2},
          },
      .capacity = 2,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, Tetris) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 10},
               .size = 2,
               .gaps = {{.lifespan = {0, 5}, .window = {{0, 1}}}}},
              {.lifespan = {0, 10},
               .size = 2,
               .gaps = {{.lifespan = {5, 10}, .window = {{1, 2}}}}},
          },
      .capacity = 3,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, Stairs) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 108},
               .size = 30,
               .gaps = {{.lifespan = {36, 72}, .window = {{10, 30}}},
                        {.lifespan = {72, 108}, .window = {{20, 30}}}}},
              {.lifespan = {36, 144},
               .size = 50,
               .gaps = {{.lifespan = {36, 72}, .window = {{20, 30}}},
                        {.lifespan = {72, 108}, .window = {{10, 40}}}}},
              {.lifespan = {84, 144},
               .size = 42,
               .gaps = {{.lifespan = {114, 129}, .window = {{0, 28}}},
                        {.lifespan = {129, 144}, .window = {{0, 14}}}}},
              {.lifespan = {84, 129},
               .size = 42,
               .gaps = {{.lifespan = {99, 114}, .window = {{14, 42}}},
                        {.lifespan = {114, 129}, .window = {{28, 42}}}}},
              {.lifespan = {99, 144},
               .size = 70,
               .gaps = {{.lifespan = {99, 114}, .window = {{28, 42}}},
                        {.lifespan = {114, 129}, .window = {{14, 56}}}}},
              {.lifespan = {0, 144},
               .size = 30,
               .gaps = {{.lifespan = {72, 108}, .window = {{0, 20}}},
                        {.lifespan = {108, 144}, .window = {{0, 10}}}}},
          },
      .capacity = 144,
  };
  test_feasible(problem);
}

TEST_F(TinySolverTest, ComputeIrreducibleInfeasibleSubset) {
  const Problem problem = {
      .buffers =
          {
              {.lifespan = {0, 2}, .size = 2},  // Not part of the IIS.
              {.lifespan = {0, 2}, .size = 2},  // Not part of the IIS.
              {.lifespan = {2, 5}, .size = 2},  // Part of the IIS.
              {.lifespan = {3, 6}, .size = 2},  // Part of the IIS.
              {.lifespan = {4, 7}, .size = 2},  // Part of the IIS.
          },
      .capacity = 4,
  };
  TinySolver solver;
  auto subset = solver.ComputeIrreducibleInfeasibleSubset(problem);
  ASSERT_EQ(subset.status().code(), absl::StatusCode::kOk);
  std::vector<minimalloc::BufferIdx> expected_subset = {2, 3, 4};
  EXPECT_EQ(*subset, expected_subset);
}

}  // namespace
}  // namespace minimalloc
