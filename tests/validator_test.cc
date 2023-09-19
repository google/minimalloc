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

#include "../src/validator.h"

#include "../src/minimalloc.h"
#include "gtest/gtest.h"

namespace minimalloc {
namespace {

TEST(ValidatorTest, ValidatesGoodSolution) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 1}, .size = 2},
        {.lifespan = {1, 3}, .size = 1},
        {.lifespan = {2, 4}, .size = 1},
        {.lifespan = {3, 5}, .size = 1},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  const Solution solution = {.offsets = {0, 0, 1, 0}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, ValidatesGoodSolutionWithGaps) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {1, 9}}}},
        {.lifespan = {5, 15}, .size = 2, .gaps = {{.lifespan = {6, 14}}}},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  const Solution solution = {.offsets = {0, 0}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, ValidatesGoodSolutionWithGapsEdgeCase) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {1, 8}}}},
        {.lifespan = {5, 15}, .size = 2, .gaps = {{.lifespan = {8, 14}}}},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  const Solution solution = {.offsets = {0, 0}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, ValidatesTetris) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {0, 5},
                                                   .window = {{0, 1}}}}},
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {5, 10},
                                                   .window = {{1, 2}}}}},
     },
    .capacity = 3
  };
  // right # of offsets, in range, no overlaps
  const Solution solution = {.offsets = {0, 1}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, ValidatesStairs) {
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
  const Solution solution = {.offsets = {30, 10, 60, 102, 74, 0}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, InvalidatesBadSolution) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  const Solution solution = {.offsets = {0, 0}};  // wrong # of offsets
  EXPECT_EQ(Validate(problem, solution), kBadSolution);
}

TEST(ValidatorTest, InvalidatesFixedBuffer) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1, .offset = 0},
      },
      .capacity = 2
  };
  // last buffer needs offset @ 0
  const Solution solution = {.offsets = {0, 0, 1}};
  EXPECT_EQ(Validate(problem, solution), kBadFixed);
}

TEST(ValidatorTest, InvalidatesNegativeOffset) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  const Solution solution = {.offsets = {0, 0, -1}};  // offset is negative
  EXPECT_EQ(Validate(problem, solution), kBadOffset);
}

TEST(ValidatorTest, InvalidatesOutOfRangeOffset) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  // buffer lies outside range
  const Solution solution = {.offsets = {0, 0, 2}};
  EXPECT_EQ(Validate(problem, solution), kBadOffset);
}

TEST(ValidatorTest, InvalidatesOverlap) {
  const Problem problem = {
      .buffers = {
           {.lifespan = {0, 1}, .size = 2},
           {.lifespan = {1, 2}, .size = 1},
           {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  // final two buffers overlap
  const Solution solution = {.offsets = {0, 0, 0}};
  EXPECT_EQ(Validate(problem, solution), kBadOverlap);
}

TEST(ValidatorTest, InvalidatesMisalignment) {
  const Problem problem = {
      .buffers = {
           {.lifespan = {0, 1}, .size = 2},
           {.lifespan = {1, 2}, .size = 1, .alignment = 2},
      },
      .capacity = 2
  };
  // offset of 1 isn't a multiple of 2
  const Solution solution = {.offsets = {0, 1}};
  EXPECT_EQ(Validate(problem, solution), kBadAlignment);
}

TEST(ValidatorTest, InvalidatesGapOverlap) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {1, 7}}}},
        {.lifespan = {5, 15}, .size = 2, .gaps = {{.lifespan = {8, 14}}}},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  const Solution solution = {.offsets = {0, 0}};
  EXPECT_EQ(Validate(problem, solution), kBadOverlap);
}

}  // namespace
}  // namespace minimalloc
