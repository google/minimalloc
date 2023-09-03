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

#include "converter.h"

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "minimalloc.h"

namespace minimalloc {

namespace {

constexpr absl::string_view kBufferId = "buffer_id";
constexpr absl::string_view kBuffer = "buffer";
constexpr absl::string_view kStart = "start";
constexpr absl::string_view kEnd = "end";
constexpr absl::string_view kSize = "size";
constexpr absl::string_view kOffset = "offset";
constexpr absl::string_view kAlignment = "alignment";
constexpr absl::string_view kGaps = "gaps";

bool IncludeAlignment(const Problem& problem) {
  for (const Buffer& buffer : problem.buffers) {
    if (buffer.alignment != 1) return true;
  }
  return false;
}

bool IncludeGaps(const Problem& problem) {
  for (const Buffer& buffer : problem.buffers) {
    if (!buffer.gaps.empty()) return true;
  }
  return false;
}

}  // namespace

std::string ToCsv(const Problem& problem, Solution* solution) {
  const bool include_alignment = IncludeAlignment(problem);
  const bool include_gaps = IncludeGaps(problem);
  std::vector<std::string> header = {std::string(kBuffer),
                                     std::string(kStart),
                                     std::string(kEnd),
                                     std::string(kSize)};
  if (include_alignment) {
    header.push_back(std::string(kAlignment));
  }
  if (include_gaps) {
    header.push_back(std::string(kGaps));
  }
  if (solution) {
    header.push_back(std::string(kOffset));
  }
  std::vector<std::vector<std::string>> input = { header };
  for (auto buffer_idx = 0; buffer_idx < problem.buffers.size(); ++buffer_idx) {
    const Buffer& buffer = problem.buffers[buffer_idx];
    const auto& lifespan = buffer.lifespan;
    std::vector<std::string> gaps;
    gaps.reserve(buffer.gaps.size());
    for (const Gap& gap : buffer.gaps) {
      gaps.push_back(
          absl::StrCat(gap.lifespan.lower(), "-", gap.lifespan.upper()));
    }
    std::vector<std::string> record = {absl::StrCat(buffer.id),
                                       absl::StrCat(lifespan.lower()),
                                       absl::StrCat(lifespan.upper()),
                                       absl::StrCat(buffer.size)};
    if (include_alignment) {
      record.push_back(absl::StrCat(buffer.alignment));
    }
    if (include_gaps) {
      record.push_back(absl::StrJoin(gaps, " "));
    }
    if (solution) {
      record.push_back(std::to_string(solution->offsets[buffer_idx]));
    }
    input.push_back(record);
  }
  std::ostringstream oss;
  for (const auto& record : input) {
    oss << absl::StrJoin(record, ",") << std::endl;
  }
  return oss.str();
}

absl::StatusOr<Problem> FromCsv(absl::string_view input, int64_t addend) {
  Problem problem;
  absl::flat_hash_map<std::string, int> col_map;
  std::vector<absl::string_view> records = absl::StrSplit(input, '\n');
  for (const std::string_view& record : records) {
    if (record.empty()) break;
    std::vector<absl::string_view> fields = absl::StrSplit(record, ',');
    if (col_map.empty()) {  // Need to read header row (to determine columns).
      for (int field_idx = 0; field_idx < fields.size(); ++field_idx) {
        // If column reads 'buffer_id', change it to 'buffer' for consistency.
        absl::string_view col_name = fields[field_idx];
        if (col_name == kBufferId) col_name = kBuffer;
        col_map[col_name] = field_idx;
      }
      if (col_map.size() != fields.size()) {
        return absl::InvalidArgumentError("Duplicate column names");
      }
      if (!col_map.contains(kBuffer) || !col_map.contains(kStart) ||
          !col_map.contains(kEnd) || !col_map.contains(kSize)) {
        return absl::NotFoundError("A required column is missing");
      }
      continue;
    }
    if (fields.size() != col_map.size()) {
      return absl::InvalidArgumentError("Too many fields");
    }
    const std::string& id = static_cast<std::string>(fields[col_map[kBuffer]]);
    int64_t start = -1, end = -1, size = -1, alignment = 1;
    std::vector<Gap> gaps;
    std::optional<Offset> offset;
    if (!absl::SimpleAtoi(fields[col_map[kStart]], &start) ||
        !absl::SimpleAtoi(fields[col_map[kEnd]], &end) ||
        !absl::SimpleAtoi(fields[col_map[kSize]], &size)) {
      return absl::InvalidArgumentError("Improperly formed integer");
    }
    if (col_map.contains(kAlignment)) {
      if (!absl::SimpleAtoi(fields[col_map[kAlignment]], &alignment)) {
        return absl::InvalidArgumentError(
            absl::StrCat("Improperly formed alignment: ",
                         fields[col_map[kAlignment]]));
      }
    }
    if (col_map.contains(kGaps)) {
      absl::string_view gaps_str = fields[col_map[kGaps]];
      std::vector<absl::string_view> gaps_list =
          absl::StrSplit(gaps_str, ' ', absl::SkipEmpty());
      for (absl::string_view gap : gaps_list) {
        std::vector<absl::string_view> gap_pair = absl::StrSplit(gap, '-');
        if (gap_pair.size() != 2) {
          return absl::InvalidArgumentError(
              absl::StrCat("Improperly formed gap: ", gap));
        }
        minimalloc::TimeValue gap_start, gap_end;
        if (!absl::SimpleAtoi(gap_pair[0], &gap_start) ||
            !absl::SimpleAtoi(gap_pair[1], &gap_end)) {
            return absl::InvalidArgumentError(
                absl::StrCat("Improperly formed gap: ", gap));
        }
        gaps.push_back({.lifespan = {gap_start, gap_end + addend}});
      }
    }
    if (col_map.contains(kOffset)) {
      int offset_val = -1;
      if (!absl::SimpleAtoi(fields[col_map[kOffset]], &offset_val)) {
        return absl::InvalidArgumentError("Improperly formed offset");
      }
      offset = offset_val;
    }
    problem.buffers.push_back({.id = id,
                               .lifespan = {start, end + addend},
                               .size = size,
                               .alignment = alignment,
                               .gaps = gaps,
                               .offset = offset});
  }
  return problem;
}

}  // namespace minimalloc
