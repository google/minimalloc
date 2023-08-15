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

#ifndef MINIMALLOC_CONVERTER_H_
#define MINIMALLOC_CONVERTER_H_

#include <string>

#include "minimalloc.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace minimalloc {

// Converts a Problem, along with an optional Solution, into a CSV like this:
//
//      buffer,start,end,size,alignment
//      0,10,19,1,1
//      1,20,39,2,1
//      2,10,39,3,2
//
// Notably, these 'end' column values are inclusive!  If a solution is provided,
// an additional "offset" column will be created.
std::string ToCsv(const Problem& problem, Solution* solution = nullptr);

// Given a CSV like the one below (with buffers listed in any order), converts
// it into a Problem instance or returns a status if the problem is malformed:
//
//      buffer,start,end,size,alignment
//      1,20,19,2,1
//      0,10,19,1,1
//      2,10,39,3,2
//
// Notably, these 'end' column values are inclusive!  If an offset column is
// provided, these values will be stored into each buffer's offset member field.
absl::StatusOr<Problem> FromCsv(absl::string_view input);

}  // namespace minimalloc

#endif  // MINIMALLOC_CONVERTER_H_
