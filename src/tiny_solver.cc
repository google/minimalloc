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

#include "tiny_solver.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "minimalloc.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"
#include "ortools/util/time_limit.h"
#include "solver.h"
#include "sweeper.h"

using ::operations_research::TimeLimit;
using ::operations_research::sat::CpModelBuilder;
using ::operations_research::sat::CpSolverResponse;
using ::operations_research::sat::CpSolverStatus;
using ::operations_research::sat::IntervalVar;
using ::operations_research::sat::IntVar;
using ::operations_research::sat::Model;
using ::operations_research::sat::SatParameters;
using ::operations_research::sat::SolveCpModel;

namespace minimalloc {

namespace {
constexpr int kLinearizationLevel = 0;
constexpr SatParameters::SearchBranching kSearchBranching =
    SatParameters::FIXED_SEARCH;
constexpr bool kCpModelPresolve = false;
constexpr int kSymmetryLevel = 0;
constexpr double kMaxDeterministicTime = 0.01;
}  // namespace

TinySolver::TinySolver() = default;

TinySolver::TinySolver(const SolverParams& params) : Solver(params) {}

absl::StatusOr<Solution> TinySolver::SolveWithStartTime(const Problem& problem,
                                                        absl::Time start_time) {
  if (problem.buffers.empty()) return Solution{};
  const SweepResult sweep_result = Sweep(problem);
  CpModelBuilder cp_model;
  std::vector<IntVar> vars;
  vars.reserve(problem.buffers.size());
  std::vector<std::vector<IntervalVar>> intervals(sweep_result.sections.size());
  for (BufferIdx buffer_idx = 0; buffer_idx < problem.buffers.size();
       ++buffer_idx) {
    const Buffer& buffer = problem.buffers[buffer_idx];
    if (buffer.size < 0 || buffer.alignment <= 0 ||
        buffer.size > problem.capacity) {
      return absl::NotFoundError("No solution found.");
    }
    if (buffer.offset) {
      if (*buffer.offset < 0 ||
          *buffer.offset + buffer.size > problem.capacity ||
          *buffer.offset % buffer.alignment != 0) {
        return absl::NotFoundError("No solution found.");
      }
      const int64_t fixed_offset = *buffer.offset / buffer.alignment;
      vars.push_back(cp_model.NewIntVar({fixed_offset, fixed_offset}));
    } else {
      const int64_t max_offset =
          (problem.capacity - buffer.size) / buffer.alignment;
      vars.push_back(cp_model.NewIntVar({0, max_offset}));
      if (buffer.hint) cp_model.AddHint(vars.back(), *buffer.hint);
    }
    const BufferData& buffer_data = sweep_result.buffer_data[buffer_idx];
    const std::vector<SectionSpan>& section_spans = buffer_data.section_spans;
    for (const SectionSpan& section_span : section_spans) {
      const SectionRange& section_range = section_span.section_range;
      const Window& window = section_span.window;
      for (SectionIdx s = section_range.lower(); s < section_range.upper();
           ++s) {
        const auto start = buffer.alignment * vars.back() + window.lower();
        const int64_t size = window.upper() - window.lower();
        intervals[s].push_back(cp_model.NewFixedSizeIntervalVar(start, size));
      }
    }
  }
  for (auto& interval_vars : intervals) cp_model.AddNoOverlap(interval_vars);
  const absl::Duration elapsed_time = absl::Now() - start_time;
  if (elapsed_time >= params_.timeout) {
    return absl::DeadlineExceededError("Solver timed out.");
  }
  SatParameters parameters;
  parameters.set_max_time_in_seconds(
      absl::ToDoubleSeconds(params_.timeout - elapsed_time));
  parameters.set_max_deterministic_time(kMaxDeterministicTime);
  parameters.set_linearization_level(kLinearizationLevel);
  parameters.set_search_branching(kSearchBranching);
  parameters.set_cp_model_presolve(kCpModelPresolve);
  parameters.set_symmetry_level(kSymmetryLevel);
  Model model;
  model.Add(NewSatParameters(parameters));
  model.GetOrCreate<TimeLimit>()->RegisterExternalBooleanAsLimit(&cancelled_);
  const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);
  cancelled_ = false;  // Clear the bit in case it's been set.
  if (response.status() != CpSolverStatus::OPTIMAL &&
      response.status() != CpSolverStatus::FEASIBLE) {
    return absl::NotFoundError("No solution found.");
  }
  Solution solution;
  solution.offsets.reserve(problem.buffers.size());
  for (BufferIdx buffer_idx = 0; buffer_idx < problem.buffers.size();
       ++buffer_idx) {
    const Buffer& buffer = problem.buffers[buffer_idx];
    const Offset offset =
        buffer.alignment * SolutionIntegerValue(response, vars[buffer_idx]);
    solution.offsets.push_back(offset);
    solution.height = std::max(solution.height, offset + buffer.size);
  }
  return solution;
}

}  // namespace minimalloc
