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

#include <optional>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace minimalloc {
namespace {

TEST(BufferTest, EffectiveSizeWithOverlap) {
  const Buffer bufferA = {.lifespan = {0, 2}, .size = 4};
  const Buffer bufferB = {.lifespan = {1, 3}, .size = 5};
  EXPECT_EQ(bufferA.effective_size(bufferB), 4);
  EXPECT_EQ(bufferB.effective_size(bufferA), 5);
}

TEST(BufferTest, EffectiveSizeWithoutOverlap) {
  const Buffer bufferA = {.lifespan = {0, 2}, .size = 4};
  const Buffer bufferB = {.lifespan = {3, 5}, .size = 5};
  EXPECT_EQ(bufferA.effective_size(bufferB), std::nullopt);
  EXPECT_EQ(bufferB.effective_size(bufferA), std::nullopt);
}

TEST(BufferTest, EffectiveSizeWithoutOverlapEdgeCase) {
  const Buffer bufferA = {.lifespan = {0, 2}, .size = 4};
  const Buffer bufferB = {.lifespan = {2, 4}, .size = 5};
  EXPECT_EQ(bufferA.effective_size(bufferB), std::nullopt);
  EXPECT_EQ(bufferB.effective_size(bufferA), std::nullopt);
}

TEST(BufferTest, EffectiveSizeGapsWithOverlap) {
  const Buffer bufferA = {.lifespan = {0, 10},
                          .size = 4,
                          .gaps = {{.lifespan = {1, 4}},
                                   {.lifespan = {6, 9}}}};
  const Buffer bufferB = {.lifespan = {5, 15},
                          .size = 5,
                          .gaps = {{.lifespan = {6, 9}},
                                   {.lifespan = {11, 14}}}};
  EXPECT_EQ(bufferA.effective_size(bufferB), 4);
  EXPECT_EQ(bufferB.effective_size(bufferA), 5);
}

TEST(BufferTest, EffectiveSizeGapsWithoutOverlap) {
  const Buffer bufferA = {.lifespan = {0, 10},
                          .size = 4,
                          .gaps = {{.lifespan = {1, 9}}}};
  const Buffer bufferB = {.lifespan = {5, 15},
                          .size = 5,
                          .gaps = {{.lifespan = {6, 14}}}};
  EXPECT_EQ(bufferA.effective_size(bufferB), std::nullopt);
  EXPECT_EQ(bufferB.effective_size(bufferA), std::nullopt);
}

TEST(BufferTest, EffectiveSizeGapsWithoutOverlapEdgeCaseFirst) {
  const Buffer bufferA = {.lifespan = {0, 10}, .size = 4};
  const Buffer bufferB = {.lifespan = {5, 15},
                          .size = 5,
                          .gaps = {{.lifespan = {5, 10}}}};
  EXPECT_EQ(bufferA.effective_size(bufferB), std::nullopt);
  EXPECT_EQ(bufferB.effective_size(bufferA), std::nullopt);
}

TEST(BufferTest, EffectiveSizeGapsWithoutOverlapEdgeCaseSecond) {
  const Buffer bufferA = {.lifespan = {0, 10},
                          .size = 4,
                          .gaps = {{.lifespan = {5, 10}}}};
  const Buffer bufferB = {.lifespan = {5, 15}, .size = 5};
  EXPECT_EQ(bufferA.effective_size(bufferB), std::nullopt);
  EXPECT_EQ(bufferB.effective_size(bufferA), std::nullopt);
}

TEST(BufferTest, EffectiveSizeTetris) {
  const Buffer bufferA = {.lifespan = {0, 10},
                          .size = 2,
                          .gaps = {{.lifespan = {0, 5}, .window = {{0, 1}}}}};
  const Buffer bufferB = {.lifespan = {0, 10},
                          .size = 2,
                          .gaps = {{.lifespan = {5, 10}, .window = {{1, 2}}}}};
  EXPECT_EQ(bufferA.effective_size(bufferB), 1);
}

TEST(BufferTest, EffectiveSizeStairs) {
  const Buffer bufferA = {.lifespan = {0, 15},
                          .size = 3,
                          .gaps = {{.lifespan = {0, 5}, .window = {{0, 1}}},
                                   {.lifespan = {5, 10}, .window = {{0, 2}}}}};
  const Buffer bufferB = {.lifespan = {0, 15},
                          .size = 3,
                          .gaps = {{.lifespan = {5, 10}, .window = {{1, 3}}},
                                   {.lifespan = {10, 15}, .window = {{2, 3}}}}};
  EXPECT_EQ(bufferA.effective_size(bufferB), 1);
}

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
