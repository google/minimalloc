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

#ifndef MINIMALLOC_SRC_VALIDATOR_H_
#define MINIMALLOC_SRC_VALIDATOR_H_

#include "absl/base/attributes.h"
#include "minimalloc.h"

namespace minimalloc {

enum ValidationResult {
  kGood = 0,
  kBadSolution = 1,  // Solution structure is incorrect, eg. wrong # of offsets.
  kBadFixed = 2,     // A buffer w/ a fixed offset is assigned somewhere else.
  kBadOffset = 3,    // The offset is out-of-bounds, ie. negative or beyond cap.
  kBadOverlap = 4,   // At least one pair of buffers overlaps in space and time.
  kBadAlignment = 5,  // At least one buffer was not properly aligned.
  kBadHeight = 6      // Solution height is not buffers' maximum height
};

ValidationResult Validate(const Problem& problem,
                          const Solution& solution) ABSL_MUST_USE_RESULT;

}  // namespace minimalloc

#endif  // MINIMALLOC_SRC_VALIDATOR_H_
