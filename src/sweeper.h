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

#ifndef MINIMALLOC_SRC_SWEEPER_H_
#define MINIMALLOC_SRC_SWEEPER_H_

#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "minimalloc.h"

namespace minimalloc {

// An index into a list of schedule "cross sections."
using SectionIdx = int64_t;

// A count of buffers crossing between adjacent sections.
using CutCount = int64_t;

// An interval that defines a subset of ranges (e.g., where a buffer is
// continuously active).
using SectionRange = Interval<SectionIdx>;

// Defines a range that a buffer is active and its window during this interval.
struct SectionSpan {
  SectionRange section_range;
  Window window;
  bool operator==(const SectionSpan& x) const;
};

// Sections store subsets of buffers that interact with one another at some
// point in time.  As an example, consider the following problem:
//
//     Buffer buffer_0 = {.lifespan = [1, 2)};
//     Buffer buffer_1 = {.lifespan = [2, 4)};
//     Buffer buffer_2 = {.lifespan = [0, 5)};
//     Buffer buffer_3 = {.lifespan = [5, 9)};
//     Problem problem = {.buffers = {buffer_0, buffer_1, buffer_2, buffer_3}};
//
// Here is a visualization of these four buffers:
//
//            t=0    t=1    t=2    t=3    t=4    t=5    t=6    t=7    t=8    t=9
//             |======|======|======|======|======|======|======|======|======|
//    offset=3 |XXXXXX|  b0  |XXXXXX|XXXXXX|XXXXXX|XXXXXX|XXXXXX|XXXXXX|XXXXXX|
//             |------|------|------|------|------|------|------|------|------|
//    offset=2 |XXXXXX|XXXXXX|      b1     |XXXXXX|XXXXXX|XXXXXX|XXXXXX|XXXXXX|
//             |------|------|------|------|------|------|------|------|------|
//    offset=1 |                b2                |XXXXXX|XXXXXX|XXXXXX|XXXXXX|
//             |------|------|------|------|------|------|------|------|------|
//    offset=0 |XXXXXX|XXXXXX|XXXXXX|XXXXXX|XXXXXX|             b3            |
//             |======|======|======|======|======|======|======|======|======|
//
// Buffers 0 and 2 interact with one another from t=0 to t=2, forming our first
// section.  From t=2 to t=4, buffers 1 and 2 are both active.  A total of four
// sections result from these interactions:
//
//             |======|======|======|======|======|======|======|======|======|
//   sections: |     sec0    |     sec1    | sec2 |            sec3           |
//             |======|======|======|======|======|======|======|======|======|

using Section = absl::flat_hash_set<BufferIdx>;

// Partitions store various preprocessed attributes for a subset of a Problem's
// buffers.  Partitions are mutually exclusive -- that is, any buffer belongs to
// exactly one partition -- and they are guaranteed not to overlap in time (so
// may be solved independently).  In our example, buffer 3 never interacts with
// the others, and so a total of two partitions are created:
//
//             |======|======|======|======|======|======|======|======|======|
// partitions: |                p0                |             p1            |
//             |======|======|======|======|======|======|======|======|======|
//
// Here is how these relationships are expressed in the Partition objects:
//
//     Partition partition_a = {
//       .buffer_idxs = {0, 1, 2},
//       .section_range = {0, 3},  // <---- up to (but not including) section 3
//     };
//     Partition partition_b = {
//       .buffer_idxs = {3},
//       .section_range = {3, 4},  // <---- up to (but not including) section 4
//     };

struct Partition {
  // The buffers participating in this partition (indexes into problem.buffers).
  std::vector<BufferIdx> buffer_idxs;

  // A half-open interval specifying which sections this partition spans.
  SectionRange section_range;

  bool operator==(const Partition& x) const;
  friend std::ostream& operator<<(std::ostream& os, const Partition& p);
};

// Details regarding a buffer that overlaps with another.
struct Overlap {
  BufferIdx buffer_idx = -1;
  int64_t effective_size = 0;

  bool operator==(const Overlap& x) const;
  bool operator<(const Overlap& x) const;
};

// The BufferData object stores various preprocessed attributes of an individual
// buffer (i.e., its relationships with sections and any overlapping buffers).
// Given the above example, buffer 2 is active from section 0 up to (but not
// including) section 3.  Likewise, it overlaps in time with two other buffers
// (0 and 1).  Hence, its buffer data would be as follows:
//
//     BufferData buffer_data_0 = {
//       .section_spans = {{.section_range = {0, 3}, .size = 1},
//       .overlaps = {0, 1}
//     };

struct BufferData {
  // Half-open intervals specifying an exhaustive list of sections that this
  // buffer participates in.  Note: just because a buffer is live during a
  // given section does not necessarily mean it is live for the full duration.
  std::vector<SectionSpan> section_spans;

  // Contains a set of buffers that overlap at some point in time with this one.
  absl::btree_set<Overlap> overlaps;

  bool operator==(const BufferData& x) const;
};

// The SweepResult object encapsulates all of the data above.  Its primary
// client is the Solver, which uses these indices when propagating offset
// updates during its recursive depth-first search.

struct SweepResult {
  // Cross sections of buffers that are "active" at particular moments in the
  // schedule.
  std::vector<Section> sections;

  // The list of (mutually-exclusive) partitions over a problem's buffers.
  std::vector<Partition> partitions;

  // Maps each buffer to various properties (e.g, sections & pairwise overlaps).
  std::vector<BufferData> buffer_data;

  // Returns a vector of length sections.size() - 1 where the ith element is the
  // number of buffers that are active in both section i and section i + 1.
  std::vector<CutCount> CalculateCuts() const;

  bool operator==(const SweepResult& x) const;
};

enum SweepPointType { kRight, kLeft };

struct SweepPoint {
  BufferIdx buffer_idx;
  TimeValue time_value;
  SweepPointType point_type;
  Window window;
  bool endpoint = false;
  bool operator==(const SweepPoint& x) const;
  bool operator<(const SweepPoint& x) const;
};

// For a given problem, places all start & end times into a list sorted by time
// value, then point type, then buffer index.
std::vector<SweepPoint> CreatePoints(const Problem& problem);

// Maintains an "active" set of buffers to determine disjoint partitions.  For
// each partition, records the list of buffers + pairwise overlaps + unique
// cross sections.
SweepResult Sweep(const Problem& problem);

}  // namespace minimalloc

#endif  // MINIMALLOC_SRC_SWEEPER_H_
