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

#include "sweeper.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "absl/container/btree_set.h"
#include "minimalloc.h"

namespace minimalloc {

namespace {

enum PointType { kRight, kLeft };

struct Point {
  BufferIdx buffer_idx;
  TimeValue time_value;
  PointType point_type;
  Window window;
  bool endpoint;
  bool operator<(const Point& x) const {
    // First, order by time, then direction (right vs. left), then buffer idx.
    if (time_value != x.time_value) return time_value < x.time_value;
    if (point_type != x.point_type) return point_type < x.point_type;
    return buffer_idx < x.buffer_idx;
  }
};

}  // namespace

bool SectionSpan::operator==(const SectionSpan& x) const {
  return section_range == x.section_range && window == x.window;
}

bool Partition::operator==(const Partition& x) const {
  return buffer_idxs == x.buffer_idxs &&
         section_range == x.section_range;
}

bool Overlap::operator==(const Overlap& x) const {
  return buffer_idx == x.buffer_idx && effective_size == x.effective_size;
}

bool Overlap::operator<(const Overlap& x) const {
  if (buffer_idx != x.buffer_idx) return buffer_idx < x.buffer_idx;
  return effective_size < x.effective_size;
}

bool BufferData::operator==(const BufferData& x) const {
  return section_spans == x.section_spans &&
         overlaps == x.overlaps;
}

bool SweepResult::operator==(const SweepResult& x) const {
  return sections == x.sections &&
         partitions == x.partitions &&
         buffer_data == x.buffer_data;
}

// For a given problem, places all buffer start times into a priority queue.
std::vector<Point> CreatePoints(const Problem& problem) {
  std::vector<Point> points;
  points.reserve(problem.buffers.size() * 2);  // Reserve 2 spots per buffer.
  for (auto buffer_idx = 0; buffer_idx < problem.buffers.size(); ++buffer_idx) {
    const Buffer& buffer = problem.buffers[buffer_idx];
    Window window = {0, buffer.size};
    std::optional<Point> point = {{.buffer_idx = buffer_idx,  // Point 'A'
                                   .time_value = buffer.lifespan.lower(),
                                   .point_type = kLeft,
                                   .window = window,
                                   .endpoint = true}};
    // There are six *potential* points of interest for a gap:
    //
    //   A        BC       DE        F
    //             |-------|
    //   |--------||  gap  ||--------|
    //             |-------|
    //
    // Point 'A' may not need to be created if it's co-occurrent with point 'B',
    // points 'C' and 'D' may not need to be created unless there's a window,
    // and so on.
    for (const Gap& gap : buffer.gaps) {
      if (gap.window) {  // This gap has a window
        bool endpoint = false;
        if (point) {
          if (gap.lifespan.lower() > point->time_value) {
            points.push_back(*point);
            points.push_back({.buffer_idx = buffer_idx,  // Point 'B'
                              .time_value = gap.lifespan.lower(),
                              .point_type = kRight,
                              .window = window,
                              .endpoint = false});
          } else {
            endpoint = true;  // Special case: we're at the very beginning
          }
          point.reset();
        }
        points.push_back({.buffer_idx = buffer_idx,  // Point 'C'
                          .time_value = gap.lifespan.lower(),
                          .point_type = kLeft,
                          .window = *gap.window,
                          .endpoint = endpoint});
        if (gap.lifespan.upper() == buffer.lifespan.upper()) {
          window = *gap.window;  // Special case: we've reached the very end
          continue;
        }
        points.push_back({.buffer_idx = buffer_idx,  // Point 'D'
                          .time_value = gap.lifespan.upper(),
                          .point_type = kRight,
                          .window = *gap.window,
                          .endpoint = false});
        points.push_back({.buffer_idx = buffer_idx,  // Point 'E'
                          .time_value = gap.lifespan.upper(),
                          .point_type = kLeft,
                          .window = window,
                          .endpoint = false});
      } else {  // This gap does not have a window
        points.push_back(*point);
        point.reset();
        points.push_back({.buffer_idx = buffer_idx,
                          .time_value = gap.lifespan.lower(),
                          .point_type = kRight,
                          .window = window,
                          .endpoint = false});
        points.push_back({.buffer_idx = buffer_idx,
                          .time_value = gap.lifespan.upper(),
                          .point_type = kLeft,
                          .window = window,
                          .endpoint = false});
      }
    }
    if (point) {
      points.push_back(*point);
      point.reset();
    }
    points.push_back({.buffer_idx = buffer_idx,  // Point 'F'
                      .time_value = buffer.lifespan.upper(),
                      .point_type = kRight,
                      .window = window,
                      .endpoint = true});
  }
  std::sort(points.begin(), points.end());
  return points;
}

// Maintains an "active" set of buffers to determine disjoint partitions.  For
// each partition, records the list of buffers + pairwise overlaps + unique
// cross sections.
SweepResult Sweep(const Problem& problem) {
  SweepResult result;
  const auto num_buffers = problem.buffers.size();
  const std::vector<Point> points = CreatePoints(problem);
  Section actives, alive;
  TimeValue last_section_time = -1;
  SectionIdx last_section_idx = 0;
  // Create a reverse index (from buffers to sections) for quick lookup.
  result.buffer_data.resize(num_buffers);
  std::vector<SectionIdx> buffer_idx_to_section_start(num_buffers, -1);
  for (const Point& point : points) {
    const BufferIdx buffer_idx = point.buffer_idx;
    const Buffer& buffer = problem.buffers[buffer_idx];
    if (last_section_time == -1) last_section_time = point.time_value;
    if (point.point_type == kRight) {
      // Create a new cross section of buffers if one doesn't yet exist.
      if (last_section_time < point.time_value) {
        last_section_time = point.time_value;
        result.sections.push_back(actives);
      }
      // If it's a right endpoint, remove it from the set of active buffers.
      actives.erase(buffer_idx);
      if (point.endpoint) alive.erase(buffer_idx);
      const SectionRange section_range =
          {buffer_idx_to_section_start[buffer_idx],
           (int)result.sections.size()};
      const SectionSpan section_span = {section_range, point.window};
      result.buffer_data[buffer_idx].section_spans.push_back(section_span);
      // If the alives are empty, the span of this partition is now known.
      if (alive.empty()) {
        result.partitions.back().section_range =
            {last_section_idx, (int)result.sections.size()};
        last_section_idx = result.sections.size();
      }
    }
    if (point.point_type == kLeft) {
      // If it's a left endpoint, check if a new partition should be established
      if (alive.empty()) result.partitions.push_back(Partition());
      // Record any overlaps, and then add this buffer to the set of actives.
      if (point.endpoint) {
        result.partitions.back().buffer_idxs.push_back(buffer_idx);
        for (auto alive_idx : alive) {
          const Buffer& alive = problem.buffers[alive_idx];
          auto alive_effective_size = alive.effective_size(buffer);
          if (alive_effective_size) {
            result.buffer_data[alive_idx].overlaps.insert(
                {buffer_idx, *alive_effective_size});
          }
          auto effective_size = buffer.effective_size(alive);
          if (effective_size) {
            result.buffer_data[buffer_idx].overlaps.insert({alive_idx,
                                                            *effective_size});
          }
        }
      }
      actives.insert(buffer_idx);
      // Mutants OK for following line; performance tweak to prevent reinsertion
      if (point.endpoint) alive.insert(buffer_idx);
      buffer_idx_to_section_start[buffer_idx] = result.sections.size();
    }
  }
  return result;
}

// Calculates the number of "cuts," i.e., buffers that are active between
// adjacent sections.  If there are zero cuts between sections i and i+1, it
// implies that the sections {..., i-2, i-1, i} and {i+1, i+2, ...} may be
// solved separately.
std::vector<CutCount> SweepResult::CalculateCuts() const {
  std::vector<CutCount> cuts(sections.size() - 1);
  for (const BufferData& buffer_data : buffer_data) {
    const std::vector<SectionSpan>& section_spans = buffer_data.section_spans;
    for (SectionIdx s_idx = section_spans.front().section_range.lower();
        s_idx + 1 < section_spans.back().section_range.upper(); ++s_idx) {
      ++cuts[s_idx];
    }
  }
  return cuts;
}

}  // namespace minimalloc
