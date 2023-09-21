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

#include "../src/sweeper.h"

#include <vector>

#include "../src/minimalloc.h"
#include "gtest/gtest.h"

namespace minimalloc {
namespace {

////////////// NoOverlap ////////////////
//                                     //
//            t=0    t=1    t=2    t=3 //
//             |======|======|======|  //
//    offset=1 |      |XXXXXX|XXXXXX|  //
//             |  b0  |------|------|  //
//    offset=0 |      |  b1  |  b2  |  //
//             |======|======|======|  //
//                                     //
//             |======|======|======|  //
// partitions: |  p0  |  p1  |  p2  |  //
//             |------|------|------|  //
//   sections: | sec0 | sec1 | sec2 |  //
//             |======|======|======|  //
//                                     //
/////////////////////////////////////////

TEST(SweeperTest, NoOverlap) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {2, 3}, .size = 1},
      }
  };
  EXPECT_EQ(
      Sweep(problem),
      (SweepResult{
          .sections = {{0}, {1}, {2}},
          .partitions = {
              {.buffer_idxs = {0}, .section_range = {0, 1}},
              {.buffer_idxs = {1}, .section_range = {1, 2}},
              {.buffer_idxs = {2}, .section_range = {2, 3}},
          },
          .buffer_data = {
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}}},
              {.section_spans = {{.section_range = {2, 3}, .window = {0, 1}}}},
          },
      }));
}

TEST(CalculateCutsTest, NoOverlap) {
  const SweepResult sweep_result = {
     .sections = {{0}, {1}, {2}},
     .buffer_data = {
         {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
         {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}}},
         {.section_spans = {{.section_range = {2, 3}, .window = {0, 1}}}},
     },
  };
  EXPECT_EQ(sweep_result.CalculateCuts(), std::vector<CutCount>({0, 0}));
}

//////////////// WithOverlap ///////////////////
//                                            //
//            t=0    t=1    t=2    t=3    t=4 //
//             |======|======|======|======|  //
//    offset=1 |      |      b1     |XXXXXX|  //
//             |  b0  |------|------|------|  //
//    offset=0 |      |XXXXXX|      b2     |  //
//             |======|======|======|======|  //
//                                            //
//             |======|======|======|======|  //
// partitions: |  p0  |         p1         |  //
//             |------|------|------|------|  //
//   sections: | sec0 |     sec1    | sec2 |  //
//             |======|======|======|======|  //
//                                            //
////////////////////////////////////////////////

TEST(SweeperTest, WithOverlap) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 3}, .size = 1},
          {.lifespan = {2, 4}, .size = 1},
      }
  };
  EXPECT_EQ(
      Sweep(problem),
      (SweepResult{
          .sections = {{0}, {1, 2}, {2}},
          .partitions = {
              {.buffer_idxs = {0}, .section_range = {0, 1}},
              {.buffer_idxs = {1, 2}, .section_range = {1, 3}},
          },
          .buffer_data = {
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
               .overlaps = {{2, 1}}},
              {.section_spans = {{.section_range = {1, 3}, .window = {0, 1}}},
               .overlaps = {{1, 1}}},
          },
      }));
}

TEST(CalculateCutsTest, WithOverlap) {
  const SweepResult sweep_result = {
      .sections = {{0}, {1, 2}, {2}},
      .buffer_data = {
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
          {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
           .overlaps = {{2, 1}}},
          {.section_spans = {{.section_range = {1, 3}, .window = {0, 1}}},
           .overlaps = {{1, 1}}},
      },
  };
  EXPECT_EQ(sweep_result.CalculateCuts(), std::vector<CutCount>({0, 1}));
}

//////// TwoBuffersEndAtSameTime ////////
//                                     //
//            t=0    t=1    t=2    t=3 //
//             |======|======|======|  //
//    offset=1 |      |      b1     |  //
//             |  b0  |------|------|  //
//    offset=0 |      |XXXXXX|  b2  |  //
//             |======|======|======|  //
//                                     //
//             |======|======|======|  //
// partitions: |  p0  |      p1     |  //
//             |------|------|------|  //
//   sections: | sec0 |     sec1    |  //
//             |======|======|======|  //
//                                     //
/////////////////////////////////////////

TEST(SweeperTest, TwoBuffersEndAtSameTime) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 3}, .size = 1},
          {.lifespan = {2, 3}, .size = 1},
      }
  };
  EXPECT_EQ(
      Sweep(problem),
      (SweepResult{
          .sections = {{0}, {1, 2}},
          .partitions = {
              {.buffer_idxs = {0}, .section_range = {0, 1}},
              {.buffer_idxs = {1, 2}, .section_range = {1, 2}},
          },
          .buffer_data = {
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
               .overlaps = {{2, 1}}},
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
               .overlaps = {{1, 1}}},
          },
      }));
}

TEST(CalculateCutsTest, TwoBuffersEndAtSameTime) {
  const SweepResult sweep_result = {
      .sections = {{0}, {1, 2}},
      .buffer_data = {
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
          {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
           .overlaps = {{2, 1}}},
          {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
           .overlaps = {{1, 1}}},
      },
  };
  EXPECT_EQ(sweep_result.CalculateCuts(), std::vector<CutCount>({0}));
}

////// SuperLongBufferPreventsPartitioning /////
//                                            //
//            t=0    t=1    t=2    t=3    t=4 //
//             |======|======|======|======|  //
//    offset=2 |      |      b1     |XXXXXX|  //
//             |  b0  |------|------|------|  //
//    offset=1 |      |XXXXXX|      b2     |  //
//             |------|------|------|------|  //
//    offset=0 |             b3            |  //
//             |======|======|======|======|  //
//                                            //
//             |======|======|======|======|  //
// partitions: |             p0            |  //
//             |------|------|------|------|  //
//   sections: | sec0 |     sec1    | sec2 |  //
//             |======|======|======|======|  //
//                                            //
////////////////////////////////////////////////

TEST(SweeperTest, SuperLongBufferPreventsPartitioning) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 3}, .size = 1},
          {.lifespan = {2, 4}, .size = 1},
          {.lifespan = {0, 4}, .size = 1},
      }
  };
  EXPECT_EQ(
      Sweep(problem),
      (SweepResult{
          .sections = {{0, 3}, {1, 3, 2}, {3, 2}},
          .partitions = {
              {.buffer_idxs = {0, 3, 1, 2}, .section_range = {0, 3}},
          },
          .buffer_data = {
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}},
               .overlaps = {{3, 2}}},
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
               .overlaps = {{2, 1}, {3, 1}}},
              {.section_spans = {{.section_range = {1, 3}, .window = {0, 1}}},
               .overlaps = {{1, 1}, {3, 1}}},
              {.section_spans = {{.section_range = {0, 3}, .window = {0, 1}}},
               .overlaps = {{0, 1}, {1, 1}, {2, 1}}},
          }
      }));
}

TEST(CalculateCutsTest, SuperLongBufferPreventsPartitioning) {
  const SweepResult sweep_result = {
      .sections = {{0, 3}, {1, 3, 2}, {3, 2}},
      .buffer_data = {
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}},
           .overlaps = {{3, 2}}},
          {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
           .overlaps = {{2, 1}, {3, 1}}},
          {.section_spans = {{.section_range = {1, 3}, .window = {0, 1}}},
           .overlaps = {{1, 1}, {3, 1}}},
          {.section_spans = {{.section_range = {0, 3}, .window = {0, 1}}},
           .overlaps = {{0, 1}, {1, 1}, {2, 1}}},
      }
  };
  EXPECT_EQ(sweep_result.CalculateCuts(), std::vector<CutCount>({1, 2}));
}

/////////// BuffersOutOfOrder ///////////
//                                     //
//            t=0    t=1    t=2    t=3 //
//             |======|======|======|  //
//    offset=1 |      |      b1     |  //
//             |  b2  |------|------|  //
//    offset=0 |      |XXXXXX|  b0  |  //
//             |======|======|======|  //
//                                     //
//             |======|======|======|  //
// partitions: |  p0  |      p1     |  //
//             |------|------|------|  //
//   sections: | sec0 |     sec1    |  //
//             |======|======|======|  //
//                                     //
/////////////////////////////////////////

TEST(SweeperTest, BuffersOutOfOrder) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {2, 3}, .size = 1},
          {.lifespan = {1, 3}, .size = 1},
          {.lifespan = {0, 1}, .size = 2},
      }
  };
  EXPECT_EQ(
      Sweep(problem),
      (SweepResult{
          .sections = {{2}, {1, 0}},
          .partitions = {
              {.buffer_idxs = {2}, .section_range = {0, 1}},
              {.buffer_idxs = {1, 0}, .section_range = {1, 2}},
          },
          .buffer_data = {
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
               .overlaps = {{1, 1}}},
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
               .overlaps = {{0, 1}}},
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
          }
      }));
}

TEST(CalculateCutsTest, BuffersOutOfOrder) {
  const SweepResult sweep_result = {
      .sections = {{2}, {1, 0}},
      .buffer_data = {
          {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
           .overlaps = {{1, 1}}},
          {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}}},
           .overlaps = {{0, 1}}},
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}}}},
      }
  };
  EXPECT_EQ(sweep_result.CalculateCuts(), std::vector<CutCount>({0}));
}

/////////////////// WithGaps ///////////////////
//                                            //
//            t=4    t=5    t=6    t=7    t=8 //
//             |======|======|======|======|  //
//    offset=2 |  b0  |XXXXXX|  b0  |XXXXXX|  //
//             |------|------|------|------|  //
//    offset=1 |XXXXXX|  b1  |XXXXXX|  b1  |  //
//             |------|------|------|------|  //
//    offset=0 |  b2  |XXXXXX|XXXXXX|  b2  |  //
//             |======|======|======|======|  //
//                                            //
//             |======|======|======|======|  //
// partitions: |             p0            |  //
//             |------|------|------|------|  //
//   sections: | sec0 | sec1 | sec2 | sec3 |  //
//             |======|======|======|======|  //
//                                            //
////////////////////////////////////////////////

TEST(SweeperTest, WithGaps) {
  const Problem problem = {
      .buffers = {
          {.lifespan = {4, 7}, .size = 1, .gaps = {{.lifespan = {5, 6}}}},
          {.lifespan = {5, 8}, .size = 1, .gaps = {{.lifespan = {6, 7}}}},
          {.lifespan = {4, 8}, .size = 1, .gaps = {{.lifespan = {5, 7}}}},
      }
  };
  EXPECT_EQ(
      Sweep(problem),
      (SweepResult{
          .sections = {{0, 2}, {1}, {0}, {1, 2}},
          .partitions = {{.buffer_idxs = {0, 2, 1}, .section_range = {0, 4}}},
          .buffer_data = {
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 1}},
                                 {.section_range = {2, 3}, .window = {0, 1}}},
               .overlaps = {{2, 1}}},
              {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}},
                                 {.section_range = {3, 4}, .window = {0, 1}}},
               .overlaps = {{2, 1}}},
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 1}},
                                 {.section_range = {3, 4}, .window = {0, 1}}},
               .overlaps = {{0, 1}, {1, 1}}},
          },
      }));
}

TEST(CalculateCutsTest, WithGaps) {
  const SweepResult sweep_result = {
      .sections = {{0, 2}, {1}, {0}, {1, 2}},
      .partitions = {{.buffer_idxs = {0, 2, 1}, .section_range = {0, 4}}},
      .buffer_data = {
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 1}},
                             {.section_range = {2, 3}, .window = {0, 1}}},
           .overlaps = {{2, 1}}},
          {.section_spans = {{.section_range = {1, 2}, .window = {0, 1}},
                             {.section_range = {3, 4}, .window = {0, 1}}},
           .overlaps = {{2, 1}}},
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 1}},
                             {.section_range = {3, 4}, .window = {0, 1}}},
           .overlaps = {{0, 1}, {1, 1}}},
      },
  };
  EXPECT_EQ(sweep_result.CalculateCuts(), std::vector<CutCount>({2, 3, 2}));
}

//////////////////// Tetris ////////////////////
//                                            //
//            t=4    t=5    t=6    t=7    t=8 //
//             |======|======|======|======|  //
//    offset=2 |                           |  //
//             |      b1     |-------------|  //
//    offset=1 |             |             |  //
//             |-------------|      b0     |  //
//    offset=0 |                           |  //
//             |======|======|======|======|  //
//                                            //
//             |======|======|======|======|  //
// partitions: |             p0            |  //
//             |------|------|------|------|  //
//   sections: |     sec0    |     sec1    |  //
//             |======|======|======|======|  //
//                                            //
////////////////////////////////////////////////

TEST(SweeperTest, Tetris) {
  const Problem problem = {
    .buffers = {
        {.lifespan = {4, 8}, .size = 2, .gaps = {{.lifespan = {4, 6},
                                                  .window = {{0, 1}}}}},
        {.lifespan = {4, 8}, .size = 2, .gaps = {{.lifespan = {6, 8},
                                                  .window = {{1, 2}}}}},
     },
    .capacity = 3
  };
  EXPECT_EQ(
      Sweep(problem),
      (SweepResult{
          .sections = {{0, 1}, {0, 1}},
          .partitions = {{.buffer_idxs = {0, 1}, .section_range = {0, 2}}},
          .buffer_data = {
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 1}},
                                 {.section_range = {1, 2}, .window = {0, 2}}},
               .overlaps = {{1, 1}}},
              {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}},
                                 {.section_range = {1, 2}, .window = {1, 2}}},
               .overlaps = {{0, 2}}},
          },
      }));
}

TEST(CalculateCutsTest, Tetris) {
  const SweepResult sweep_result = {
      .sections = {{0, 1}, {0, 1}},
      .partitions = {{.buffer_idxs = {0, 1}, .section_range = {0, 2}}},
      .buffer_data = {
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 1}},
                             {.section_range = {1, 2}, .window = {0, 2}}},
           .overlaps = {{1, 1}}},
          {.section_spans = {{.section_range = {0, 1}, .window = {0, 2}},
                             {.section_range = {1, 2}, .window = {1, 2}}},
           .overlaps = {{0, 2}}},
      },
  };
  EXPECT_EQ(sweep_result.CalculateCuts(), std::vector<CutCount>({2}));
}

}  // namespace
}  // namespace minimalloc
