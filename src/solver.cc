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

#include "solver.h"

#include <limits.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "minimalloc.h"
#include "sweeper.h"

namespace minimalloc {
namespace {

using PreorderIdx = int64_t;  // An index into a preordered buffer list.

constexpr int64_t kNoOffset = -1;

// Used to incrementally maintain data about sections during search.
struct SectionData {
  Offset floor = 0;  // The lowest viable offset for any buffer in this section.
  int64_t total = 0;  // A sum of the total unallocated buffer sizes in the section.
};

// Data used to help establish a dynamic ordering of buffers.
struct OrderData {
  Offset offset = 0;
  PreorderIdx preorder_idx = 0;
};

// A record of a buffer's minimum offset value prior to a change during search.
struct OffsetChange {
  BufferIdx buffer_idx;
  Offset min_offset;
};

// A record of a section's floor value prior to a change during search.
struct SectionChange {
  SectionIdx section_idx;
  Offset floor;
};

// Dynamically orders buffers by minimum offset, followed by preorder index.
const auto kDynamicComparator =
    [](const OrderData& a, const OrderData& b) {
      if (a.offset != b.offset) return a.offset < b.offset;
      return a.preorder_idx < b.preorder_idx;
    };

class SolverImpl {
 public:
  SolverImpl(const SolverParams& params, const absl::Time start_time,
      const Problem& problem, const SweepResult& sweep_result,
      int64_t* backtracks, std::atomic<bool>& cancelled) : params_(params),
      start_time_(start_time), problem_(problem), sweep_result_(sweep_result),
      backtracks_(*backtracks), cancelled_(cancelled) {}

  absl::StatusOr<Solution> Solve() {
    if (problem_.buffers.empty()) return solution_;
    const auto num_buffers = problem_.buffers.size();
    assignment_.offsets.resize(num_buffers, kNoOffset);
    solution_.offsets.resize(num_buffers, kNoOffset);
    min_offsets_.resize(num_buffers);
    section_data_.resize(sweep_result_.sections.size());
    for (BufferIdx buffer_idx = 0; buffer_idx < num_buffers; ++buffer_idx) {
      const BufferData& buffer_data = sweep_result_.buffer_data[buffer_idx];
      for (const SectionSpan& section_span : buffer_data.section_spans) {
        const SectionRange& section_range = section_span.section_range;
        const Window& window = section_span.window;
        for (SectionIdx s_idx = section_range.lower();
            s_idx < section_range.upper(); ++s_idx) {
          section_data_[s_idx].total += window.upper() - window.lower();
        }
      }
      if (const Buffer& buffer = problem_.buffers[buffer_idx]; buffer.offset) {
        min_offsets_[buffer_idx] = *buffer.offset;
      }
    }
    cuts_ = sweep_result_.CalculateCuts();
    // If multiple heuristics were specified, use round robin to try them all.
    if (params_.preordering_heuristics.size() > 1) return RoundRobin();
    PreorderingComparator preordering_comparator(
        params_.preordering_heuristics.back());
    DLOG(INFO) << __func__ << " (Start) " << preordering_comparator
        << ", node limit   " << nodes_remaining_;
    for (const Partition& partition : sweep_result_.partitions) {
      absl::Status status = SubSolve(partition, preordering_comparator);
      if (!status.ok()) {
        DLOG(INFO) << __func__ << " (End)   " << preordering_comparator
            << ", node visited "
            << std::numeric_limits<int64_t>::max() - nodes_remaining_
            << ", status " << absl::StatusCodeToString(status.code());
        return status;
      }
    }
    DLOG(INFO) << __func__ << " (End)   " << preordering_comparator
        << ", node visited "
        << std::numeric_limits<int64_t>::max() - nodes_remaining_
        << ", status " << absl::StatusCodeToString(absl::StatusCode::kOk);
    UpdateSolutionHeight();
    return solution_;
  }

 private:
  absl::StatusOr<Solution> RoundRobin() {
    // We'll start with a conservative node limit (in the hopes that one of
    // them will finish quickly), then progressively increase this threshold.
    int64_t node_limit = problem_.buffers.size();
    while (true) {
      node_limit *= 2;
      absl::Status status = absl::OkStatus();
      for (const auto& heuristic : params_.preordering_heuristics) {
        PreorderingComparator preordering_comparator(heuristic);
        nodes_remaining_ = node_limit;
        status = absl::OkStatus();
        DLOG(INFO) << __func__ << " (Start) " << preordering_comparator
            << ", node limit   " << nodes_remaining_;
        for (const Partition& partition : sweep_result_.partitions) {
          status = SubSolve(partition, preordering_comparator);
          // The 'aborted' code means this strategy exhausted its node limit.
          if (!status.ok()) {
            DLOG(INFO) << __func__ << " (End)   " << preordering_comparator
                << ", node visited "
                << node_limit - nodes_remaining_
                << ", status " << absl::StatusCodeToString(status.code());
            if (absl::IsAborted(status)) break;
            return status;
          }
        }
        if (status.ok()) {
          DLOG(INFO) << __func__ << " (End)   " << preordering_comparator
              << ", node visited "
              << node_limit - nodes_remaining_
              << ", status " << absl::StatusCodeToString(absl::StatusCode::kOk);
          break;
        } 
      }
      if (status.ok()) break;
    }
    UpdateSolutionHeight();
    return solution_;
  }

  // Prepopulates section data for this partition, then kicks into the recursive
  // depth-first search.  Returns 'true' if a feasible solution has been found,
  // otherwise 'false'.
  absl::Status SubSolve(
      const Partition& partition,
      const PreorderingComparator& preordering_comparator) {
    std::vector<PreorderData> preordering;
    preordering.reserve(partition.buffer_idxs.size());
    for (const BufferIdx buffer_idx : partition.buffer_idxs) {
      const Buffer& buffer = problem_.buffers[buffer_idx];
      int64_t total = 0;
      const BufferData& buffer_data = sweep_result_.buffer_data[buffer_idx];
      const std::vector<SectionSpan>& section_spans = buffer_data.section_spans;
      for (const SectionSpan& section_span : section_spans) {
        const SectionRange& section_range = section_span.section_range;
        for (SectionIdx s_idx = section_range.lower();
            s_idx < section_range.upper(); ++s_idx) {
          total = std::max(total, section_data_[s_idx].total);
        }
      }
      int64_t sections = section_spans.back().section_range.upper() -
                     section_spans.front().section_range.lower();
      preordering.push_back({
        .area = buffer.area(),
        .lower = buffer.lifespan.lower(),
        .overlaps = buffer_data.overlaps.size(),
        .sections = sections,
        .size = buffer.size,
        .total = total,
        .upper = buffer.lifespan.upper(),
        .width = buffer.lifespan.upper() - buffer.lifespan.lower(),
        .buffer_idx = buffer_idx});
    }
    if (params_.static_preordering) {
      absl::c_sort(preordering, preordering_comparator);
    }
    std::vector<OrderData> ordering(preordering.size());
    for (PreorderIdx idx = 0; idx < preordering.size(); ++idx) {
      ordering[idx].preorder_idx = idx;
    }
    absl::StatusCode status_code =
        SearchSolutions(partition, preordering_comparator, preordering,
            ordering, /*min_offset=*/0, /*min_preorder_idx=*/0);
    return status_code == absl::StatusCode::kOk ? absl::OkStatus()
        : absl::Status(status_code, "Error encountered during search.");
  }

  // Updates section data given that 'buffer_idx' is the next item to be placed.
  std::vector<SectionChange> UpdateSectionData(
      const absl::flat_hash_set<SectionIdx>& affected_sections,
      BufferIdx buffer_idx) {
    std::vector<SectionChange> section_changes;
    const Offset offset = assignment_.offsets[buffer_idx];
    // For any section this buffer resides in, bump up the floor & drop the sum.
    const BufferData& buffer_data = sweep_result_.buffer_data[buffer_idx];
    for (const SectionSpan& section_span : buffer_data.section_spans) {
      const SectionRange& section_range = section_span.section_range;
      const Window& window = section_span.window;
      const Offset height = offset + window.upper();
      for (SectionIdx s_idx = section_range.lower();
          s_idx < section_range.upper(); ++s_idx) {
        section_changes.push_back(
            {.section_idx = s_idx, .floor = section_data_[s_idx].floor});
        section_data_[s_idx].floor = height;
        section_data_[s_idx].total -= window.upper() - window.lower();
      }
    }
    // The floor of any section cannot be lower than its lowest minimum offset.
    for (const SectionIdx s_idx : affected_sections) {
      Offset min_offset = std::numeric_limits<Offset>::max();
      for (const BufferIdx other_idx : sweep_result_.sections[s_idx]) {
        if (assignment_.offsets[other_idx] == kNoOffset) {
          min_offset = std::min(min_offset, min_offsets_[other_idx]);
        }
      }
      if (
        min_offset != std::numeric_limits<Offset>::max() &&
        section_data_[s_idx].floor < min_offset
      ) {
        section_changes.push_back(
            {.section_idx = s_idx, .floor = section_data_[s_idx].floor});
        section_data_[s_idx].floor = min_offset;
      }
    }
    return section_changes;
  }

  // Restores the section data by reversing any recorded changes.
  void RestoreSectionData(
      const std::vector<SectionChange>& section_changes,
      BufferIdx buffer_idx) {
    for (auto c = section_changes.rbegin(); c != section_changes.rend(); ++c) {
      section_data_[c->section_idx].floor = c->floor;
    }
    // For any section this buffer resides in, increase the sum.
    const BufferData& buffer_data = sweep_result_.buffer_data[buffer_idx];
    for (const SectionSpan& section_span : buffer_data.section_spans) {
      const SectionRange& section_range = section_span.section_range;
      const Window& window = section_span.window;
      for (SectionIdx s_idx = section_range.lower();
          s_idx < section_range.upper(); ++s_idx) {
        section_data_[s_idx].total += window.upper() - window.lower();
      }
    }
  }

  // Updates min offset data, given that 'buffer_idx' is the next to be placed.
  std::optional<std::vector<OffsetChange>> UpdateMinOffsets(
      BufferIdx buffer_idx,
      absl::flat_hash_set<SectionIdx>& affected_sections,
      bool& fixed_offset_failure) {
    bool hatless = true;
    std::vector<OffsetChange> offset_changes;
    const Offset offset = assignment_.offsets[buffer_idx];
    // For any overlap this buffer participates in, bump up its minimum offset.
    const std::vector<BufferData>& buffer_data = sweep_result_.buffer_data;
    for (const Overlap& overlap : buffer_data[buffer_idx].overlaps) {
      const BufferIdx other_idx = overlap.buffer_idx;
      if (assignment_.offsets[other_idx] != kNoOffset) continue;
      hatless = false;
      const Offset height = offset + overlap.effective_size;
      if (min_offsets_[other_idx] >= height) continue;
      offset_changes.push_back(
          {.buffer_idx = other_idx, .min_offset = min_offsets_[other_idx]});
      min_offsets_[other_idx] = height;
      const Buffer& other_buffer = problem_.buffers[other_idx];
      Offset diff = min_offsets_[other_idx] % other_buffer.alignment;
      if (diff > 0) min_offsets_[other_idx] += other_buffer.alignment - diff;
      if (other_buffer.offset &&
          min_offsets_[other_idx] > *other_buffer.offset) {
        fixed_offset_failure = true;
      }
      if (!params_.unallocated_floor) continue;  // Mutation safe.
      const BufferData& buffer_data = sweep_result_.buffer_data[other_idx];
      for (const SectionSpan& section_span : buffer_data.section_spans) {
        const SectionRange& section_range = section_span.section_range;
        for (SectionIdx s_idx = section_range.lower();
             s_idx < section_range.upper(); ++s_idx) {
          affected_sections.insert(s_idx);
        }
      }
    }
    if (hatless) return std::nullopt;
    return offset_changes;
  }

  // Restores the minimum offsets by reversing any recorded changes.
  void RestoreMinOffsets(const std::vector<OffsetChange>& offset_changes) {
    for (auto c = offset_changes.rbegin(); c != offset_changes.rend(); ++c) {
      min_offsets_[c->buffer_idx] = c->min_offset;
    }
  }

  // Returns 'true' if this partial solution satisfies consistency & inference
  // checks, otherwise 'false'.
  bool Check(const Partition& partition, Offset offset) {
    for (SectionIdx s_idx = partition.section_range.lower();
        s_idx < partition.section_range.upper(); ++s_idx) {
      // Note: by construction, the given section_data object is guaranteed to
      // have an element for every index in the partition's section_range.
      auto [floor, total] = section_data_[s_idx];
      if (params_.monotonic_floor) floor = std::max(offset, floor);
      if (params_.section_inference) floor += total;
      if (problem_.capacity < floor) return false;
    }
    return true;
  }

  // Orders unallocated buffers by their minimum possible offset values, using
  // buffer areas as a tie-breaker.
  std::vector<OrderData> ComputeOrdering(
      const std::vector<PreorderData>& preordering,
      const std::vector<OrderData>& orig_ordering) {
    std::vector<OrderData> ordering;
    for (const auto [offset, preorder_idx] : orig_ordering) {
      const BufferIdx buffer_idx = preordering[preorder_idx].buffer_idx;
      // If this buffer has already been assigned, keep looking.
      if (assignment_.offsets[buffer_idx] != kNoOffset) continue;
      const Offset new_offset = min_offsets_[buffer_idx];
      ordering.push_back(
          {.offset = new_offset, .preorder_idx = preorder_idx});
    }
    if (params_.dynamic_ordering) absl::c_sort(ordering, kDynamicComparator);
    return ordering;
  }

  // Determines the minimum height of any unallocated buffer ... no other buffer
  // should be assigned to an offset at this value or greater.
  Offset CalcMinHeight(
      const std::vector<PreorderData>& preordering,
      const std::vector<OrderData>& ordering) {
    Offset min_height = std::numeric_limits<Offset>::max();
    for (const auto [offset, preorder_idx] : ordering) {
      const BufferIdx buffer_idx = preordering[preorder_idx].buffer_idx;
      const Buffer& buffer = problem_.buffers[buffer_idx];
      min_height = std::min(min_height, offset + buffer.size);
    }
    return min_height;
  }

  // A recursive depth-first search for buffer offset assignment.  Returns 'kOk'
  // if a feasible solution has been found, otherwise 'kNotFound' or potentially
  // 'kDeadlineExceeded' / 'kAborted'.
  absl::StatusCode SearchSolutions(
      const Partition& partition,
      const PreorderingComparator& preordering_comparator,
      const std::vector<PreorderData>& preordering,
      const std::vector<OrderData>& orig_ordering,
      const Offset min_offset,
      const PreorderIdx min_preorder_idx) {
    DLOG(INFO) << __func__ << " (Start) " << partition;
    if (nodes_remaining_ <= 0) {
      DLOG(INFO) << __func__ << " (End)   " << partition << ", status "
          << absl::StatusCodeToString(absl::StatusCode::kAborted);
      return absl::StatusCode::kAborted;
    }
    --nodes_remaining_;
    if (absl::Now() - start_time_ > params_.timeout || cancelled_) {
      DLOG(INFO) << __func__ << " (End)   " << partition << ", status "
          << absl::StatusCodeToString(absl::StatusCode::kDeadlineExceeded);
      return absl::StatusCode::kDeadlineExceeded;
    }
    const std::vector<OrderData> ordering =
        ComputeOrdering(preordering, orig_ordering);
    if (ordering.empty()) {
      // Store offsets for all the buffers that participate in this partition.
      for (const BufferIdx buffer_idx : partition.buffer_idxs) {
        solution_.offsets[buffer_idx] = assignment_.offsets[buffer_idx];
      }
      DLOG(INFO) << __func__ << " (End)  " << partition << ", status "
          << absl::StatusCodeToString(absl::StatusCode::kOk);
      return absl::StatusCode::kOk;  // We've reached a leaf node.
    }
    const Offset min_height = CalcMinHeight(preordering, ordering);
    for (const auto [offset, preorder_idx] : ordering) {
      const BufferIdx buffer_idx = preordering[preorder_idx].buffer_idx;
      if (params_.canonical_only) {
        // Buffers should be placed in non-increasing order by area.
        if (offset < min_offset ||
            (offset == min_offset && preorder_idx < min_preorder_idx)) continue;
      }
      if (params_.check_dominance) {
       // Check if this solution would introduce an unnecessary gap.
        if (offset >= min_height) continue;
      }
      if (const Buffer& buffer = problem_.buffers[buffer_idx]; buffer.offset) {
        if (offset > *buffer.offset) continue;
      }
      assignment_.offsets[buffer_idx] = offset;
      absl::flat_hash_set<SectionIdx> affected_sections;
      bool fixed_offset_failure = false;
      auto offset_changes = UpdateMinOffsets(buffer_idx, affected_sections,
		                             fixed_offset_failure);
      std::vector<SectionChange> section_changes =
          UpdateSectionData(affected_sections, buffer_idx);
      absl::StatusCode status_code = absl::StatusCode::kNotFound;
      if (!fixed_offset_failure && Check(partition, offset)) {
        DLOG(INFO) << "DFS (Enter) Depth: " << std::setw(5) << depth_++
            << ", BufferIdx: " << std::setw(5) << buffer_idx;
        status_code =
            params_.dynamic_decomposition
                ? DynamicallyDecompose(partition, preordering_comparator,
                    preordering, ordering, offset, preorder_idx, buffer_idx)
                : SearchSolutions(partition, preordering_comparator,
                    preordering, ordering, offset, preorder_idx);
        DLOG(INFO) << "DFS (Leave) Depth: " << std::setw(5) << --depth_
            << ", BufferIdx: " << std::setw(5) << buffer_idx;
      }
      RestoreSectionData(section_changes, buffer_idx);
      if (offset_changes) RestoreMinOffsets(*offset_changes);
      assignment_.offsets[buffer_idx] = kNoOffset;  // Mark it unallocated.
      // If a feasible solution *or* timeout, abort search.
      if (status_code != absl::StatusCode::kNotFound) {
        DLOG(INFO) << __func__ << " (End)  " << partition << ", status "
            << absl::StatusCodeToString(status_code);
        return status_code;
      }
      if (!offset_changes && params_.hatless_pruning) break;
    }
    ++backtracks_;
    DLOG(INFO) << __func__ << " (End)  " << partition << ", status "
        << absl::StatusCodeToString(absl::StatusCode::kNotFound);
    return absl::StatusCode::kNotFound;  // No feasible solution found.
  }

  // Decomposes the problem into partitions and solves each independently.  If
  // any subproblem is found to be infeasible, no further search is performed.
  absl::StatusCode DynamicallyDecompose(
      const Partition& partition,
      const PreorderingComparator& preordering_comparator,
      const std::vector<PreorderData>& preordering,
      const std::vector<OrderData>& orig_ordering,
      const Offset min_offset,
      const PreorderIdx min_preorder_idx,
      const BufferIdx buffer_idx) {
    solution_.offsets[buffer_idx] = assignment_.offsets[buffer_idx];
    // Reduce the cuts between sections spanned by this buffer (and store all
    // zero-cut section indices into 'cutpoints', to be solved separately).
    std::vector<SectionIdx> cutpoints = {partition.section_range.lower()};
    const BufferData& buffer_data = sweep_result_.buffer_data[buffer_idx];
    const std::vector<SectionSpan>& section_spans = buffer_data.section_spans;
    for (SectionIdx s_idx = section_spans.front().section_range.lower();
      s_idx + 1 < section_spans.back().section_range.upper(); ++s_idx) {
      if (--cuts_[s_idx] == 0) cutpoints.push_back(s_idx + 1);
    }
    absl::StatusCode status_code = absl::StatusCode::kOk;
    if (cutpoints.size() == 1) {
      status_code =
          SearchSolutions(partition, preordering_comparator, preordering,
              orig_ordering, min_offset, min_preorder_idx);
    } else {
      cutpoints.push_back(partition.section_range.upper());
      for (int64_t c_idx = 1; c_idx < cutpoints.size(); ++c_idx) {
        // Determine the range of this sub-partition.
        const SectionRange section_range =
            {cutpoints[c_idx - 1], cutpoints[c_idx]};
        // Determine the contents of this sub-partition.
        std::vector<BufferIdx> buffer_idxs;
        for (const BufferIdx other_idx : partition.buffer_idxs) {
          // A minor optimization (mutants ok).
          if (assignment_.offsets[other_idx] != kNoOffset) continue;
          const BufferData& other_data = sweep_result_.buffer_data[other_idx];
          const SectionRange other_range = {
            other_data.section_spans.front().section_range.lower(),
            other_data.section_spans.back().section_range.upper()
          };
          if (!(other_range.upper() <= section_range.lower() ||
                section_range.upper() <= other_range.lower())) {
            buffer_idxs.push_back(other_idx);
          }
        }
        if (buffer_idxs.empty()) continue;
        // Create the sub-partition and solve it.
        const Partition sub_partition =
            {.buffer_idxs = buffer_idxs, .section_range = section_range};
        absl::Status status = SubSolve(sub_partition, preordering_comparator);
        if (!status.ok()) {
          status_code = status.code();
          break;
        }
      }
    }
    // Restore all section cuts to their previous values.
    for (SectionIdx s_idx = section_spans.front().section_range.lower();
        s_idx + 1 < section_spans.back().section_range.upper(); ++s_idx) {
      ++cuts_[s_idx];
    }
    return status_code;
  }

  void UpdateSolutionHeight() {
    const auto num_buffers = problem_.buffers.size();
    for (BufferIdx buffer_idx = 0; buffer_idx < num_buffers; buffer_idx++) {
      const Offset buffer_size = problem_.buffers[buffer_idx].size;
      const Offset buffer_offset = solution_.offsets[buffer_idx];
      solution_.height = std::max(
        solution_.height, buffer_size + buffer_offset);
    }
  }

  const SolverParams& params_;
  const absl::Time start_time_;
  const Problem& problem_;
  const SweepResult& sweep_result_;
  int64_t& backtracks_;
  std::atomic<bool>& cancelled_;

  Solution assignment_;
  Solution solution_;
  std::vector<Offset> min_offsets_;
  std::vector<SectionData> section_data_;
  std::vector<CutCount> cuts_;
  int64_t nodes_remaining_ = std::numeric_limits<int64_t>::max();

  // debug
  int64_t depth_ = 0;
};  // class SolverImpl

}  // namespace

PreorderingComparator::PreorderingComparator(const PreorderingHeuristic& h) :
    preordering_heuristic_(h) {}

bool PreorderingComparator::operator()(
    const PreorderData& a, const PreorderData& b) const {
  for (const char &c : preordering_heuristic_) {
    if (c == 'A' && a.area != b.area) return a.area > b.area;
    if (c == 'C' && a.sections != b.sections) return a.sections > b.sections;
    if (c == 'L' && a.lower != b.lower) return a.lower > b.lower;
    if (c == 'O' && a.overlaps != b.overlaps) return a.overlaps > b.overlaps;
    if (c == 'T' && a.total != b.total) return a.total > b.total;
    if (c == 'U' && a.upper != b.upper) return a.upper > b.upper;
    if (c == 'W' && a.width != b.width) return a.width > b.width;
    if (c == 'Z' && a.size != b.size) return a.size > b.size;
  }
  return a.buffer_idx < b.buffer_idx;
}

std::ostream& operator<<(std::ostream& os, const PreorderingComparator& c) {
  os << "preorder heuristic " << c.preordering_heuristic_;
  return os;
}

Solver::Solver() {}

Solver::Solver(const SolverParams& params) : params_(params) {}

// Calculates partitions, and then solves each subproblem independently.  If
// any subproblem is found to be infeasible, no further search is performed.
absl::StatusOr<Solution> Solver::Solve(const Problem& problem) {
  backtracks_ = 0;  // Reset the backtrack counter.
  cancelled_ = false;
  return SolveWithStartTime(problem, absl::Now());
}

absl::StatusOr<Solution> Solver::SolveWithStartTime(const Problem& problem,
                                                    absl::Time start_time) {
  const SweepResult sweep_result = Sweep(problem);
  if (!params_.minimize_capacity) {
    SolverImpl solver_impl(
        params_, start_time, problem, sweep_result, &backtracks_, cancelled_);
    return solver_impl.Solve();
  }

  // binary search the minimum viable capacity
  Problem problem_bs = problem;
  absl::StatusOr<Solution> solution(absl::Status(
    absl::StatusCode::kNotFound, "Not found any valid capacity."));
  Capacity lo_capacity = 0, hi_capacity = problem.capacity;
  while (lo_capacity <= hi_capacity) {
    Capacity mid_capacity = (lo_capacity + hi_capacity) / 2;
    problem_bs.capacity = mid_capacity;
    SolverImpl solver_impl(
        params_, start_time, problem_bs, sweep_result, &backtracks_, cancelled_);
    const absl::Time start_time = absl::Now();
    auto solution_bs = solver_impl.Solve();
    const absl::Time end_time = absl::Now();
    if (!solution_bs.ok()) {
      DLOG(INFO) << __func__ << "Binary search, capacity " << mid_capacity
          << ", elapsed time " << std::fixed << std::setprecision(3)
          << absl::ToDoubleSeconds(end_time - start_time)
          << ", status " << absl::StatusCodeToString(solution_bs.status().code());
      lo_capacity = mid_capacity + 1;
    } else {
      DLOG(INFO) << __func__ << "Binary search, capacity " << mid_capacity
          << ", elapsed time " << std::fixed << std::setprecision(3)
          << absl::ToDoubleSeconds(end_time - start_time)
          << ", status " << absl::StatusCodeToString(absl::StatusCode::kOk);
      solution = solution_bs;
      hi_capacity = solution->height - 1; // mid_capacity - 1;
    }
  }
  return solution;
}

int64_t Solver::get_backtracks() const { return backtracks_; }

void Solver::Cancel() { cancelled_ = true; }

absl::StatusOr<std::vector<BufferIdx>>
    Solver::ComputeIrreducibleInfeasibleSubset(const Problem& problem) {
  backtracks_ = 0;  // Reset the backtrack counter.
  cancelled_ = false;
  const absl::Time start_time = absl::Now();
  std::vector<bool> include(problem.buffers.size(), true);
  std::vector<BufferIdx> subset;
  for (auto buffer_idx = 0; buffer_idx < problem.buffers.size(); ++buffer_idx) {
    include[buffer_idx] = false;  // Try removing this buffer from the problem.
    Problem subproblem = {.capacity = problem.capacity};
    for (BufferIdx idx = 0; idx < problem.buffers.size(); ++idx) {
      if (include[idx]) subproblem.buffers.push_back(problem.buffers[idx]);
    }
    auto solution = SolveWithStartTime(subproblem, start_time);
    if (absl::IsDeadlineExceeded(solution.status())) return solution.status();
    if ((include[buffer_idx] = solution.ok())) subset.push_back(buffer_idx);
  }
  return subset;
}

}  // namespace minimalloc
