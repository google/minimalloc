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

#ifndef MINIMALLOC_SRC_TINY_SOLVER_H_
#define MINIMALLOC_SRC_TINY_SOLVER_H_

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "minimalloc.h"
#include "solver.h"

namespace minimalloc {

class TinySolver : public Solver {
 public:
  TinySolver();
  explicit TinySolver(const SolverParams& params);

 protected:
  absl::StatusOr<Solution> SolveWithStartTime(const Problem& problem,
                                              absl::Time start_time) override;
};

}  // namespace minimalloc

#endif  // MINIMALLOC_SRC_TINY_SOLVER_H_
