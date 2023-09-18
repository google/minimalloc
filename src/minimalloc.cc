
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

#include "minimalloc.h"

#include <optional>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace minimalloc {

namespace {

enum PointType { kRight, kLeft };

using Point = std::pair<TimeValue, PointType>;

}  // namespace

bool Gap::operator==(const Gap& x) const {
  return lifespan == x.lifespan && window == x.window;
}

bool Buffer::operator==(const Buffer& x) const {
  return id == x.id
      && lifespan == x.lifespan
      && size == x.size
      && alignment == x.alignment
      && offset == x.offset
      && gaps == x.gaps;
}

bool Solution::operator==(const Solution& x) const {
  return offsets == x.offsets;
}

bool Problem::operator==(const Problem& x) const {
  return buffers == x.buffers
      && capacity == x.capacity;
}

Area Buffer::area() const {
  return size * (lifespan.upper() - lifespan.lower());
}

std::optional<int64_t> Buffer::effective_size(const Buffer& x) const {
  if (lifespan.upper() <= x.lifespan.lower()) return std::nullopt;
  if (x.lifespan.upper() <= lifespan.lower()) return std::nullopt;
  std::vector<Point> points = {
    {lifespan.lower(), PointType::kLeft},
    {lifespan.upper(), PointType::kRight},
    {x.lifespan.lower(), PointType::kLeft},
    {x.lifespan.upper(), PointType::kRight}};
  for (const Gap& gap : gaps) {
    points.push_back({gap.lifespan.lower(), PointType::kRight});
    points.push_back({gap.lifespan.upper(), PointType::kLeft});
  }
  for (const Gap& gap : x.gaps) {
    points.push_back({gap.lifespan.lower(), PointType::kRight});
    points.push_back({gap.lifespan.upper(), PointType::kLeft});
  }
  std::sort(points.begin(), points.end());
  int actives = 0;
  for (const Point& point : points) {
    actives += (point.second == PointType::kLeft) ? 1 : -1;
    if (actives > 1) return size;
  }
  return std::nullopt;
}

absl::StatusOr<Solution> Problem::strip_solution() {
  Solution solution;
  for (Buffer& buffer : buffers) {
    if (!buffer.offset) {
      return absl::NotFoundError("Buffer found with no offset");
    }
    solution.offsets.push_back(*buffer.offset);
    buffer.offset.reset();
  }
  return solution;
}

}  // namespace minimalloc
