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

#include "validator.h"

#include <optional>
#include <vector>

#include "minimalloc.h"

namespace minimalloc {

namespace {

enum PointType { kRight, kLeft };

using Point = std::pair<TimeValue, PointType>;

}  // namespace

bool Overlaps(const Buffer& b1, const Buffer& b2) {
  if (b1.lifespan.upper() <= b2.lifespan.lower()) return false;
  if (b2.lifespan.upper() <= b1.lifespan.lower()) return false;
  std::vector<Point> points = {
    {b1.lifespan.lower(), PointType::kLeft},
    {b1.lifespan.upper(), PointType::kRight},
    {b2.lifespan.lower(), PointType::kLeft},
    {b2.lifespan.upper(), PointType::kRight}};
  for (const Gap& gap : b1.gaps) {
    points.push_back({gap.lifespan.lower(), PointType::kRight});
    points.push_back({gap.lifespan.upper(), PointType::kLeft});
  }
  for (const Gap& gap : b2.gaps) {
    points.push_back({gap.lifespan.lower(), PointType::kRight});
    points.push_back({gap.lifespan.upper(), PointType::kLeft});
  }
  std::sort(points.begin(), points.end());
  int actives = 0;
  for (const Point& point : points) {
    actives += (point.second == PointType::kLeft) ? 1 : -1;
    if (actives > 1) return true;
  }
  return false;
}

ValidationResult Validate(const Problem& problem, const Solution& solution) {
  // Check that the number of buffers matches the number of offsets.
  if (problem.buffers.size() != solution.offsets.size()) return kBadSolution;
  // Check fixed buffers & check that offsets are within the allowable range.
  for (auto buffer_idx = 0; buffer_idx < problem.buffers.size(); ++buffer_idx) {
    const Buffer& buffer = problem.buffers[buffer_idx];
    const Offset offset = solution.offsets[buffer_idx];
    if (buffer.offset && *buffer.offset != offset) return kBadFixed;
    if (offset < 0) return kBadOffset;
    if (offset + buffer.size > problem.capacity) return kBadOffset;
    if (offset % buffer.alignment != 0) return kBadAlignment;
  }
  // Check that no two buffers overlap in both space and time, the O(n^2) way.
  for (BufferIdx i = 0; i < problem.buffers.size(); ++i) {
    const Buffer& buffer_i = problem.buffers[i];
    const Offset offset_i = solution.offsets[i];
    for (BufferIdx j = i + 1; j < problem.buffers.size(); ++j) {
      const Buffer& buffer_j = problem.buffers[j];
      const Offset offset_j = solution.offsets[j];
      if (offset_i + buffer_i.size <= offset_j ||
          offset_j + buffer_j.size <= offset_i) {
        continue;
      }
      if (Overlaps(buffer_i, buffer_j)) return kBadOverlap;
    }
  }
  return kGood;
}

}  // namespace minimalloc
