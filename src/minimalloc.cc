
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
