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

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "converter.h"
#include "minimalloc.h"
#include "solver.h"
#include "validator.h"

ABSL_FLAG(int64_t, capacity, 0, "The maximum memory capacity.");
ABSL_FLAG(std::string, input, "", "The path to the input CSV file.");
ABSL_FLAG(std::string, output, "", "The path to the output CSV file.");
ABSL_FLAG(absl::Duration, timeout, absl::InfiniteDuration(),
          "The time limit enforced for the MiniMalloc solver.");
ABSL_FLAG(bool, validate, false, "Validates the solver's output.");

ABSL_FLAG(bool, canonical_only, true, "Explores canonical solutions only.");
ABSL_FLAG(bool, section_inference, true, "Performs advanced inference.");
ABSL_FLAG(bool, dynamic_ordering, true, "Dynamically orders buffers.");
ABSL_FLAG(bool, check_dominance, true,
          "Checks for dominated solutions that leave gaps in the allocation.");
ABSL_FLAG(bool, unallocated_floor, true,
          "Uses min offsets to establish lower bounds on section floors.");
ABSL_FLAG(bool, static_preordering, true, "Statically preorders buffers.");
ABSL_FLAG(bool, dynamic_decomposition, true, "Dynamically decomposes buffers.");
ABSL_FLAG(bool, monotonic_floor, true,
          "Requires the solution floor to increase monotonically.");
ABSL_FLAG(bool, hatless_pruning, true,
          "Prunes alternate solutions whenever a buffer has nothing overhead.");

ABSL_FLAG(std::string, preordering_heuristics, "WAT,TAW,TWA",
          "Static preordering heuristics to attempt.");

ABSL_FLAG(bool, print_solution, false, "Prints the solution in LaTeX");

// Found using trial-and-error with the LaTeX 'tikzpicture' package.
const float kWidth = 17;
const float kHeight = 8.5;

void PrintSolution(const minimalloc::Problem& problem,
                   const minimalloc::Solution& solution) {
  std::ostream& os = std::cout;
  os << std::endl;
  os << "\\documentclass[tikz]{standalone}" << std::endl;
  os << "\\usepackage{tikz}" << std::endl;
  os << "\\usepackage{pgfplots}" << std::endl;
  os << "\\begin{document}" << std::endl;
  os << "\\begin{tikzpicture}" << std::endl;
  minimalloc::TimeValue min_time = INT_MAX;
  minimalloc::TimeValue max_time = 0;
  for (const minimalloc::Buffer& buffer : problem.buffers) {
    min_time = std::min(min_time, buffer.lifespan.lower());
    max_time = std::max(max_time, buffer.lifespan.upper());
  }
  const float scale_x = kWidth / (max_time - min_time);
  const float scale_y = kHeight / problem.capacity;
  for (int buffer_idx = 0; buffer_idx < problem.buffers.size(); ++buffer_idx) {
    const minimalloc::Buffer& buffer = problem.buffers[buffer_idx];
    for (int i = 0; i <= buffer.gaps.size(); ++i) {
      auto left = (i == 0) ? buffer.lifespan.lower()
                           : buffer.gaps[i - 1].lifespan.upper();
      auto right = (i == buffer.gaps.size()) ? buffer.lifespan.upper()
                                             : buffer.gaps[i].lifespan.lower();
      if (left == right) continue;
      const float x = scale_x * (left - min_time);
      const float y = scale_y * solution.offsets[buffer_idx];
      const float w = scale_x * (right - left);
      const float h = scale_y * buffer.size;
      const std::string color = "lightgray";
      os << "\\fill[" << color << ",draw=darkgray] (" << x << "," << y
         << ")" << " rectangle (" << x + w << "," << y + h << ");  % height = "
         << h << ", ID = " << buffer.id << std::endl;
    }
  }
  const float w = scale_x * (max_time - min_time);
  const float h = scale_y * problem.capacity;
  os << "\\fill[draw=black,fill opacity=0,thick] (0,0) rectangle "
     << "(" << w << "," << h << ");" << std::endl;
  os << "\\end{tikzpicture}" << std::endl;
  os << "\\end{document}" << std::endl;
}

// Solves a given problem using the Solver.
int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  minimalloc::SolverParams params = {
      .timeout = absl::GetFlag(FLAGS_timeout),
      .canonical_only = absl::GetFlag(FLAGS_canonical_only),
      .section_inference = absl::GetFlag(FLAGS_section_inference),
      .dynamic_ordering = absl::GetFlag(FLAGS_dynamic_ordering),
      .check_dominance = absl::GetFlag(FLAGS_check_dominance),
      .unallocated_floor = absl::GetFlag(FLAGS_unallocated_floor),
      .static_preordering = absl::GetFlag(FLAGS_static_preordering),
      .dynamic_decomposition = absl::GetFlag(FLAGS_dynamic_decomposition),
      .monotonic_floor = absl::GetFlag(FLAGS_monotonic_floor),
      .hatless_pruning = absl::GetFlag(FLAGS_hatless_pruning),
      .preordering_heuristics = absl::StrSplit(
          absl::GetFlag(FLAGS_preordering_heuristics), ',', absl::SkipEmpty()),
  };
  std::ifstream ifs(absl::GetFlag(FLAGS_input));
  std::string csv((std::istreambuf_iterator<char>(ifs)),
                  (std::istreambuf_iterator<char>()   ));
  absl::StatusOr<minimalloc::Problem> problem = minimalloc::FromCsv(csv);
  if (!problem.ok()) return 1;
  problem->capacity = absl::GetFlag(FLAGS_capacity);
  minimalloc::Solver solver(params);
  const absl::Time start_time = absl::Now();
  absl::StatusOr<minimalloc::Solution> solution = solver.Solve(*problem);
  const absl::Time end_time = absl::Now();
  std::cerr << std::fixed << std::setprecision(3)
      << absl::ToDoubleSeconds(end_time - start_time);
  if (!solution.ok()) return 1;
  if (absl::GetFlag(FLAGS_validate)) {
    minimalloc::ValidationResult validation_result =
        minimalloc::Validate(*problem, *solution);
    std::cerr << (validation_result == minimalloc::ValidationResult::kGood
        ? "PASS" : "FAIL") << std::endl;
  }
  if (absl::GetFlag(FLAGS_print_solution)) PrintSolution(*problem, *solution);
  std::string contents = minimalloc::ToCsv(*problem, &(*solution));
  std::ofstream ofs(absl::GetFlag(FLAGS_output));
  ofs << contents;
  ofs.close();
  return 0;
}
