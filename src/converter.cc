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
#include <ostream>
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

constexpr absl::string_view kAlignment = "alignment";
constexpr absl::string_view kBegin = "begin";
constexpr absl::string_view kBuffer = "buffer";
constexpr absl::string_view kBufferId = "buffer_id";
constexpr absl::string_view kEnd = "end";
constexpr absl::string_view kGaps = "gaps";
constexpr absl::string_view kHint = "hint";
constexpr absl::string_view kId = "id";
constexpr absl::string_view kLower = "lower";
constexpr absl::string_view kOffset = "offset";
constexpr absl::string_view kSize = "size";
constexpr absl::string_view kStart = "start";
constexpr absl::string_view kUpper = "upper";

bool IncludeAlignment(const Problem& problem) {
  for (const Buffer& buffer : problem.buffers) {
    if (buffer.alignment != 1) return true;
  }
  return false;
}

bool IncludeHint(const Problem& problem) {
  for (const Buffer& buffer : problem.buffers) {
    if (buffer.hint) return true;
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

std::string ToCsv(const Problem& problem, Solution* solution, bool old_format) {
  const bool include_alignment = IncludeAlignment(problem);
  const bool include_hint = IncludeHint(problem);
  const bool include_gaps = IncludeGaps(problem);
  const int64_t addend = old_format ? -1 : 0;
  std::vector<std::string> header = {std::string(kId),
                                     std::string(old_format ? kStart : kLower),
                                     std::string(old_format ? kEnd : kUpper),
                                     std::string(kSize)};
  if (include_alignment) header.push_back(std::string(kAlignment));
  if (include_hint) header.push_back(std::string(kHint));
  if (include_gaps) header.push_back(std::string(kGaps));
  if (solution) header.push_back(std::string(kOffset));
  std::vector<std::vector<std::string>> input = { header };
  for (auto buffer_idx = 0; buffer_idx < problem.buffers.size(); ++buffer_idx) {
    const Buffer& buffer = problem.buffers[buffer_idx];
    const auto& lifespan = buffer.lifespan;
    std::vector<std::string> gaps;
    gaps.reserve(buffer.gaps.size());
    for (const Gap& gap : buffer.gaps) {
      std::string gap_str = absl::StrCat(gap.lifespan.lower(), "-",
                                         gap.lifespan.upper() + addend);
      if (gap.window) {
        gap_str +=
            absl::StrCat("@", gap.window->lower(), ":", gap.window->upper());
      }
      gaps.push_back(gap_str);
    }
    std::vector<std::string> record = {absl::StrCat(buffer.id),
                                       absl::StrCat(lifespan.lower()),
                                       absl::StrCat(lifespan.upper() + addend),
                                       absl::StrCat(buffer.size)};
    if (include_alignment) record.push_back(absl::StrCat(buffer.alignment));
    if (include_hint) record.push_back(absl::StrCat(buffer.hint.value_or(-1)));
    if (include_gaps) record.push_back(absl::StrJoin(gaps, " "));
    if (solution) record.push_back(absl::StrCat(solution->offsets[buffer_idx]));
    input.push_back(record);
  }
  std::ostringstream oss;
  for (const auto& record : input) {
    oss << absl::StrJoin(record, ",") << std::endl;
  }
  return oss.str();
}

absl::StatusOr<Problem> FromCsv(absl::string_view input) {
  int64_t addend = 0;
  Problem problem;
  absl::flat_hash_map<std::string, int64_t> col_map;
  std::vector<absl::string_view> records = absl::StrSplit(input, '\n');
  for (const std::string_view& record : records) {
    if (record.empty()) break;
    std::vector<absl::string_view> fields = absl::StrSplit(record, ',');
    if (col_map.empty()) {  // Need to read header row (to determine columns).
      for (int64_t field_idx = 0; field_idx < fields.size(); ++field_idx) {
        // If column reads 'buffer_id', change it to 'buffer' for consistency.
        absl::string_view col_name = fields[field_idx];
        if (col_name == kBegin) col_name = kLower;
        if (col_name == kBuffer) col_name = kId;
        if (col_name == kBufferId) col_name = kId;
        if (col_name == kEnd) {
          col_name = kUpper;
          addend = 1;  // Values of an "end" column are assumed to be off-by-one
        }
        if (col_name == kStart) col_name = kLower;
        col_map[col_name] = field_idx;
      }
      if (col_map.size() != fields.size()) {
        return absl::InvalidArgumentError("Duplicate column names");
      }
      if (!col_map.contains(kId) || !col_map.contains(kLower) ||
          !col_map.contains(kUpper) || !col_map.contains(kSize)) {
        return absl::NotFoundError("A required column is missing");
      }
      continue;
    }
    if (fields.size() != col_map.size()) {
      return absl::InvalidArgumentError("Too many fields");
    }
    const std::string& id = static_cast<std::string>(fields[col_map[kId]]);
    int64_t lower = -1, upper = -1, size = -1;
    if (!absl::SimpleAtoi(fields[col_map[kLower]], &lower) ||
        !absl::SimpleAtoi(fields[col_map[kUpper]], &upper) ||
        !absl::SimpleAtoi(fields[col_map[kSize]], &size)) {
      return absl::InvalidArgumentError("Improperly formed integer");
    }
    int64_t alignment = 1;
    if (col_map.contains(kAlignment)) {
      if (!absl::SimpleAtoi(fields[col_map[kAlignment]], &alignment)) {
        return absl::InvalidArgumentError(
            absl::StrCat("Improperly formed alignment: ",
                         fields[col_map[kAlignment]]));
      }
    }
    std::optional<Offset> hint;
    if (col_map.contains(kHint)) {
      int64_t hint_val = -1;
      if (!absl::SimpleAtoi(fields[col_map[kHint]], &hint_val)) {
        return absl::InvalidArgumentError("Improperly formed hint");
      }
      if (hint_val >= 0) hint = hint_val;
    }
    std::vector<Gap> gaps;
    if (col_map.contains(kGaps)) {
      absl::string_view gaps_str = fields[col_map[kGaps]];
      std::vector<absl::string_view> gaps_list =
          absl::StrSplit(gaps_str, ' ', absl::SkipEmpty());
      for (absl::string_view gap : gaps_list) {
        std::vector<absl::string_view> at = absl::StrSplit(gap, '@');
        if (at.empty()) {
          return absl::InvalidArgumentError(
              absl::StrCat("Improperly formed gap: ", gap));
        }
        std::vector<absl::string_view> gap_pair = absl::StrSplit(at[0], '-');
        if (gap_pair.size() != 2) {
          return absl::InvalidArgumentError(
              absl::StrCat("Improperly formed gap: ", gap));
        }
        TimeValue gap_lower, gap_upper;
        if (!absl::SimpleAtoi(gap_pair[0], &gap_lower) ||
            !absl::SimpleAtoi(gap_pair[1], &gap_upper)) {
            return absl::InvalidArgumentError(
                absl::StrCat("Improperly formed gap: ", gap));
        }
        std::optional<Window> window;
        if (at.size() > 1) {
          std::vector<absl::string_view> at_pair = absl::StrSplit(at[1], ':');
          if (at_pair.size() != 2) {
            return absl::InvalidArgumentError(
                absl::StrCat("Improperly formed gap: ", gap));
          }
          int64_t window_lower, window_upper;
          if (!absl::SimpleAtoi(at_pair[0], &window_lower) ||
              !absl::SimpleAtoi(at_pair[1], &window_upper)) {
              return absl::InvalidArgumentError(
                  absl::StrCat("Improperly formed gap: ", gap));
          }
          window = {window_lower, window_upper};
        }
        gaps.push_back({.lifespan = {gap_lower, gap_upper + addend},
                        .window = window});
      }
    }
    std::optional<Offset> offset;
    if (col_map.contains(kOffset)) {
      int64_t offset_val = -1;
      if (!absl::SimpleAtoi(fields[col_map[kOffset]], &offset_val)) {
        return absl::InvalidArgumentError("Improperly formed offset");
      }
      offset = offset_val;
    }
    problem.buffers.push_back({.id = id,
                               .lifespan = {lower, upper + addend},
                               .size = size,
                               .alignment = alignment,
                               .gaps = gaps,
                               .offset = offset,
                               .hint = hint});
  }
  return problem;
}

}  // namespace minimalloc
