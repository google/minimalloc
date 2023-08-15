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

#include "../src/minimalloc.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace minimalloc {
namespace {

TEST(ProblemTest, StripSolutionOk) {
  Problem problem = {
    .buffers = {
       {.lifespan = {0, 1}, .size = 2, .offset = 3},
       {.lifespan = {1, 2}, .size = 3, .offset = 4},
    },
    .capacity = 5
  };
  const auto solution = problem.strip_solution();
  EXPECT_EQ(solution.status().code(), absl::StatusCode::kOk);
  EXPECT_EQ(*solution, (Solution{.offsets = {3, 4}}));
}

TEST(ProblemTest, StripSolutionNotFound) {
  Problem problem = {
    .buffers = {
       {.lifespan = {0, 1}, .size = 2, .offset = 3},
       {.lifespan = {1, 2}, .size = 3},
    },
    .capacity = 5
  };
  const auto solution = problem.strip_solution();
  EXPECT_EQ(solution.status().code(), absl::StatusCode::kNotFound);
}

}  // namespace
}  // namespace minimalloc
