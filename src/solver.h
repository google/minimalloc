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

#ifndef MINIMALLOC_SRC_SOLVER_H_
#define MINIMALLOC_SRC_SOLVER_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "minimalloc.h"

namespace minimalloc {

using CanonicalOnlyParam = bool;
using SectionInferenceParam = bool;
using DynamicOrderingParam = bool;
using CheckDominanceParam = bool;
using UnallocatedFloorParam = bool;
using StaticPreorderingParam = bool;
using DynamicDecompositionParam = bool;
using MonotonicFloorParam = bool;
using HatlessPruningParam = bool;
using MinimizeCapacity = bool;
using PreorderingHeuristic = std::string;

// Various settings that enable / disable certain advanced search & inference
// techniques (for benchmarking) that are employed by the solver.  Unless
// directed otherwise, users should stick with these defaults.
struct SolverParams {
  // The amount of time before the solver gives up on its search.
  absl::Duration timeout = absl::InfiniteDuration();

  // Requires that partial assignments conform to a "canonical" (i.e., non-
  // redundant) solution structure.
  CanonicalOnlyParam canonical_only = true;

  // Prunes any partial solutions in which the lower bound of some section
  // height eclipses the maximum memory capacity.
  SectionInferenceParam section_inference = true;

  // Prefer blocks with smaller viable offset values, using area to break ties.
  DynamicOrderingParam dynamic_ordering = true;

  // Prunes any partial solutions that result in some gap(s) where unallocated
  // buffers could be easily placed.
  CheckDominanceParam check_dominance = true;

  // Uses the minimum offset of unallocated buffers to establish stronger lower
  // bounds on each section's floor.
  UnallocatedFloorParam unallocated_floor = true;

  // Performs an initial sort by maximum section total, followed by area.
  StaticPreorderingParam static_preordering = true;

  // Performs dynamic temporal decomposition.
  DynamicDecompositionParam dynamic_decomposition = true;

  // Requires that the floor of the entire solution increase monotonically.
  MonotonicFloorParam monotonic_floor = true;

  // Prunes alternate solutions whenever a buffer has nothing overhead.
  HatlessPruningParam hatless_pruning = true;

  // Minimize the allocated space
  MinimizeCapacity minimize_capacity = false;

  // The static preordering heuristics to attempt.
  std::vector<PreorderingHeuristic> preordering_heuristics = {"WAT", "TAW",
                                                              "TWA"};
};

// Data used to help establish a static preordering of buffers.
struct PreorderData {
  Area area;  // The total area (i.e., space x time) consumed by this buffer.
  TimeValue lower;    // When does the buffer start?
  uint64_t overlaps;  // The number of pairwise overlaps with other buffers.
  int64_t sections;   // The number of sections spanned by this buffer.
  int64_t size;       // The size of the buffer.
  int64_t total;    // The (maximum) total sum in any of this buffer's sections.
  TimeValue upper;  // When does the buffer end?
  int64_t width;    // The width of this buffer's lifespan.
  BufferIdx buffer_idx;  // An index into a Problem's list of buffers.
};

class PreorderingComparator {
 public:
  explicit PreorderingComparator(const PreorderingHeuristic& h);
  bool operator()(const PreorderData& a, const PreorderData& b) const;
  friend std::ostream& operator<<(std::ostream& os,
                                  const PreorderingComparator& c);

 private:
  PreorderingHeuristic preordering_heuristic_;
};

class Solver {
 public:
  Solver();
  virtual ~Solver() = default;
  explicit Solver(const SolverParams& params);
  absl::StatusOr<Solution> Solve(const Problem& problem);

  // Returns the number of backtracks in the solver's latest invocation.
  int64_t get_backtracks() const;

  // Cancels search.
  void Cancel();

  // A naïve approach to compute an irreducible infeasible subset of buffers.
  absl::StatusOr<std::vector<BufferIdx>> ComputeIrreducibleInfeasibleSubset(
      const Problem& problem);

 protected:
  virtual absl::StatusOr<Solution> SolveWithStartTime(const Problem& problem,
                                                      absl::Time start_time);

  const SolverParams params_;
  int64_t backtracks_ = 0;  // A counter that maintains backtrack count.
  std::atomic<bool> cancelled_ = false;
};

}  // namespace minimalloc

#endif  // MINIMALLOC_SRC_SOLVER_H_
