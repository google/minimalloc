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

#include "../src/converter.h"

#include "../src/minimalloc.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"

namespace minimalloc {
namespace {

TEST(ConverterTest, ToCsv) {
  EXPECT_EQ(
      ToCsv({
          .buffers = {
              {.id = "0", .lifespan = {5, 10}, .size = 15},
              {
                .id = "1",
                .lifespan = {6, 12},
                .size = 18,
                .alignment = 2,
                .gaps = {{.lifespan = {7, 8}}, {.lifespan = {9, 10}}}
              },
           },
          .capacity = 40
        }),
      "id,lower,upper,size,alignment,gaps\n"
      "0,5,10,15,1,\n1,6,12,18,2,7-8 9-10\n");
}

TEST(ConverterTest, ToCsvWithoutAlignment) {
  EXPECT_EQ(
      ToCsv({
          .buffers = {
              {.id = "0", .lifespan = {5, 10}, .size = 15},
              {
                .id = "1",
                .lifespan = {6, 12},
                .size = 18,
                .gaps = {{.lifespan = {7, 8}}, {.lifespan = {9, 10}}}
              },
           },
          .capacity = 40
        }),
      "id,lower,upper,size,gaps\n"
      "0,5,10,15,\n1,6,12,18,7-8 9-10\n");
}

TEST(ConverterTest, ToCsvWithoutGaps) {
  EXPECT_EQ(
      ToCsv({
          .buffers = {
              {.id = "0", .lifespan = {5, 10}, .size = 15},
              {
                .id = "1",
                .lifespan = {6, 12},
                .size = 18,
                .alignment = 2,
              },
           },
          .capacity = 40
        }),
      "id,lower,upper,size,alignment\n"
      "0,5,10,15,1\n1,6,12,18,2\n");
}

TEST(ConverterTest, ToCsvWithSolution) {
  Solution solution = {.offsets = {1, 21}};
  EXPECT_EQ(
      ToCsv({
          .buffers = {
              {.id = "0", .lifespan = {5, 10}, .size = 15},
              {
                .id = "1",
                .lifespan = {6, 12},
                .size = 18,
                .alignment = 2,
                .gaps = {{.lifespan = {7, 8}}, {.lifespan = {9, 10}}}
              },
          },
          .capacity = 40
        },
        &solution),
      "id,lower,upper,size,alignment,gaps,offset\n"
      "0,5,10,15,1,,1\n1,6,12,18,2,7-8 9-10,21\n");
}

TEST(ConverterTest, ToCsvWeirdIDs) {
  EXPECT_EQ(
      ToCsv({
          .buffers = {
              {.id = "10", .lifespan = {5, 10}, .size = 15},
              {
                .id = "20",
                .lifespan = {6, 12},
                .size = 18,
                .alignment = 2,
                .gaps = {{.lifespan = {7, 8}}, {.lifespan = {9, 10}}}
              },
           },
          .capacity = 40
        }),
      "id,lower,upper,size,alignment,gaps\n"
      "10,5,10,15,1,\n20,6,12,18,2,7-8 9-10\n");
}

TEST(ConverterTest, ToCsvStringIDs) {
  EXPECT_EQ(
      ToCsv({
          .buffers = {
              {.id = "Little", .lifespan = {5, 10}, .size = 15},
              {
                .id = "Big",
                .lifespan = {6, 12},
                .size = 18,
                .alignment = 2,
                .gaps = {{.lifespan = {7, 8}}, {.lifespan = {9, 10}}}
              },
           },
          .capacity = 40
        }),
      "id,lower,upper,size,alignment,gaps\n"
      "Little,5,10,15,1,\nBig,6,12,18,2,7-8 9-10\n");
}

TEST(ConverterTest, FromCsvProblemOnly) {
  EXPECT_EQ(
      *FromCsv("lower,size,id,upper\n6,18,1,12\n5,15,0,10\n"),
      (Problem{
        .buffers = {
            {.id = "1", .lifespan = {6, 12}, .size = 18},
            {.id = "0", .lifespan = {5, 10}, .size = 15},
        },
      }));
}

TEST(ConverterTest, FromCsvWithAlignment) {
  EXPECT_THAT(
      *FromCsv("begin,size,buffer,upper,alignment\n6,18,1,12,2\n5,15,0,10,1\n"),
      (Problem{
        .buffers = {
          {.id = "1", .lifespan = {6, 12}, .size = 18, .alignment = 2},
          {.id = "0", .lifespan = {5, 10}, .size = 15, .alignment = 1},
        },
      }));
}

TEST(ConverterTest, FromCsvWithEmptyGaps) {
  EXPECT_THAT(
      *FromCsv("start,size,buffer_id,upper,alignment,gaps\n"
              "6,18,1,12,2,\n5,15,0,10,1,\n"),
      (Problem{
        .buffers = {
          {.id = "1", .lifespan = {6, 12}, .size = 18, .alignment = 2},
          {.id = "0", .lifespan = {5, 10}, .size = 15, .alignment = 1},
        },
      }));
}

TEST(ConverterTest, FromCsvWithGaps) {
  EXPECT_THAT(
      *FromCsv("start,size,buffer,upper,alignment,gaps\n"
              "6,18,1,12,2,7-9 \n5,15,0,10,1,9-11 12-14\n"),
      (Problem{
        .buffers = {
          {
            .id = "1",
            .lifespan = {6, 12},
            .size = 18,
            .alignment = 2,
            .gaps = {{.lifespan = {7, 9}}}
          },
          {
            .id = "0",
            .lifespan = {5, 10},
            .size = 15,
            .alignment = 1,
            .gaps = {{.lifespan = {9, 11}}, {.lifespan = {12, 14}}}
          },
        },
      }));
}

TEST(ConverterTest, FromCsvWithEndColumn) {
  EXPECT_THAT(
      *FromCsv("start,size,buffer,end,alignment,gaps\n"
              "6,18,1,11,2,7-8 \n5,15,0,9,1,9-10 12-13\n"),
      (Problem{
        .buffers = {
          {
            .id = "1",
            .lifespan = {6, 12},
            .size = 18,
            .alignment = 2,
            .gaps = {{.lifespan = {7, 9}}}
          },
          {
            .id = "0",
            .lifespan = {5, 10},
            .size = 15,
            .alignment = 1,
            .gaps = {{.lifespan = {9, 11}}, {.lifespan = {12, 14}}}
          },
        },
      }));
}

TEST(ConverterTest, FromCsvWithSolution) {
  EXPECT_THAT(
      *FromCsv("start,size,offset,buffer,upper\n6,18,21,1,12\n5,15,1,0,10\n"),
      (Problem{
        .buffers = {
            {.id = "1", .lifespan = {6, 12}, .size = 18, .offset = 21},
            {.id = "0", .lifespan = {5, 10}, .size = 15, .offset = 1},
        },
      }));
}

TEST(ConverterTest, FromCsvBufferId) {
  EXPECT_THAT(
      *FromCsv("start,size,buffer_id,upper\n6,18,1,12\n5,15,0,10\n"),
      (Problem{
        .buffers = {
            {.id = "1", .lifespan = {6, 12}, .size = 18},
            {.id = "0", .lifespan = {5, 10}, .size = 15},
        },
      }));
}

TEST(ConverterTest, FromCsvWeirdIDs) {
  EXPECT_THAT(
      *FromCsv("start,size,buffer,upper\n6,18,20,12\n5,15,10,10\n"),
      (Problem{
        .buffers = {
            {.id = "20", .lifespan = {6, 12}, .size = 18},
            {.id = "10", .lifespan = {5, 10}, .size = 15},
        },
      }));
}

TEST(ConverterTest, FromCsvStringIDs) {
  EXPECT_THAT(
      *FromCsv("start,size,buffer,upper\n6,18,Big,12\n5,15,Little,10\n"),
      (Problem{
        .buffers = {
            {.id = "Big", .lifespan = {6, 12}, .size = 18},
            {.id = "Little", .lifespan = {5, 10}, .size = 15},
        },
      }));
}

TEST(ConverterTest, BogusInputs) {
  EXPECT_EQ(
      FromCsv("start,size,buffer,upper\n"
              "a,b,c,d\ne,f,g,h\n").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ConverterTest, BogusOffsets) {
  EXPECT_EQ(
      FromCsv("start,size,offset,buffer,upper\n"
              "6,18,a,1,12\n5,15,b,0,10\n").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ConverterTest, BogusGaps) {
  EXPECT_EQ(
      FromCsv("start,size,buffer,upper,gaps\n"
              "6,18,1,12,1-2-3\n5,15,0,10,\n").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ConverterTest, MoreBogusGaps) {
  EXPECT_EQ(
      FromCsv("start,size,buffer,upper,gaps\n"
              "6,18,1,12,A-B\n5,15,0,10,\n").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ConverterTest, MissingColumn) {
  EXPECT_EQ(
      FromCsv("start,size,upper\n"
              "6,18,1,12\n5,15,10\n").status().code(),
      absl::StatusCode::kNotFound);
}

TEST(ConverterTest, DuplicateColumn) {
  EXPECT_EQ(
      FromCsv("start,size,offset,buffer,upper,upper\n"
              "6,18,21,1,12\n5,15,1,0,10\n").status().code(),
      absl::StatusCode::kInvalidArgument);
}

TEST(ConverterTest, ExtraFields) {
  EXPECT_EQ(
      FromCsv("start,size,offset,buffer,upper\n"
              "6,18,21,1,12\n5,15,1,0,10,100\n").status().code(),
      absl::StatusCode::kInvalidArgument);
}

}  // namespace
}  // namespace minimalloc
