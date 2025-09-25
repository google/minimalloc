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

ValidationResult Validate(const Problem& problem, const Solution& solution) {
  // Check that the number of buffers matches the number of offsets.
  if (problem.buffers.size() != solution.offsets.size()) return kBadSolution;
  // Check fixed buffers & check that offsets are within the allowable range.
  Offset max_height = 0;
  for (auto buffer_idx = 0; buffer_idx < problem.buffers.size(); ++buffer_idx) {
    const Buffer& buffer = problem.buffers[buffer_idx];
    const Offset offset = solution.offsets[buffer_idx];
    const Offset height = offset + buffer.size;
    max_height = std::max(max_height, height);
    if (buffer.offset && *buffer.offset != offset) return kBadFixed;
    if (offset < 0) return kBadOffset;
    if (height > problem.capacity) return kBadOffset;
    if (height > solution.height) return kBadHeight;
    if (offset % buffer.alignment != 0) return kBadAlignment;
  }
  if (max_height != solution.height) return kBadHeight;
  // Check that no two buffers overlap in both space and time, the O(n^2) way.
  for (BufferIdx i = 0; i < problem.buffers.size(); ++i) {
    const Buffer& buffer_i = problem.buffers[i];
    const Offset offset_i = solution.offsets[i];
    for (BufferIdx j = i + 1; j < problem.buffers.size(); ++j) {
      const Buffer& buffer_j = problem.buffers[j];
      const Offset offset_j = solution.offsets[j];
      const auto buffer_i_size = buffer_i.effective_size(buffer_j);
      const auto buffer_j_size = buffer_j.effective_size(buffer_i);
      if (!buffer_i_size || offset_i + *buffer_i_size <= offset_j) continue;
      if (!buffer_j_size || offset_j + *buffer_j_size <= offset_i) continue;
      return kBadOverlap;
    }
  }
  return kGood;
}

}  // namespace minimalloc
