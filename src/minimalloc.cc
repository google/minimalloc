
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

enum PointType { kLeft, kRightGap, kLeftGap, kRight };

struct Point {
  BufferIdx buffer_idx;
  TimeValue time_value;
  PointType point_type;
  std::optional<Window> window;
  bool operator<(const Point& x) const {
    if (time_value != x.time_value) return time_value < x.time_value;
    if (point_type != x.point_type) return point_type < x.point_type;
    return buffer_idx < x.buffer_idx;
  }
};

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
  const Window window = {0, size};
  const Window x_window = {0, x.size};
  std::vector<Point> points = {{0, lifespan.lower(), kLeft, window},
                               {0, lifespan.upper(), kRight, std::nullopt},
                               {1, x.lifespan.lower(), kLeft, x_window},
                               {1, x.lifespan.upper(), kRight, std::nullopt}};
  for (const Gap& gap : gaps) {
    points.push_back({0, gap.lifespan.lower(), kRightGap, gap.window});
    points.push_back({0, gap.lifespan.upper(), kLeftGap, window});
  }
  for (const Gap& gap : x.gaps) {
    points.push_back({1, gap.lifespan.lower(), kRightGap, gap.window});
    points.push_back({1, gap.lifespan.upper(), kLeftGap, x_window});
  }
  std::sort(points.begin(), points.end());
  std::optional<Window> windows[2];
  std::optional<int64_t> effective_size;
  std::optional<TimeValue> last_time;
  for (const Point& point : points) {
    if (last_time && point.time_value > *last_time) {  // We've moved right
      if (windows[0] && windows[1]) {  // Both buffers are active, let's check
        const int64_t diff = windows[0]->upper() - windows[1]->lower();
        if (!effective_size || *effective_size < diff) effective_size = diff;
      }
    }
    last_time = point.time_value;
    windows[point.buffer_idx] = point.window;
  }
  return effective_size;
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
