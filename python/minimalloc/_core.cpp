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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include <pybind11/operators.h>

#include <optional>
#include <string>
#include <vector>

#include "../../src/minimalloc.h"
#include "../../src/solver.h"
#include "../../src/converter.h"
#include "../../src/validator.h"

namespace py = pybind11;
using namespace py::literals; // Enable the _a suffix for py::arg
using namespace minimalloc;

PYBIND11_MODULE(_core, m)
{
    m.doc() = "MiniMalloc Python bindings";

    // // Use a module-local holder to prevent type registration conflicts
    // // Interval types - use unique names to avoid conflicts
    // py::class_<Interval<TimeValue>>(m, "TimeInterval")
    //     .def(py::init<TimeValue, TimeValue>(), "lower"_a, "upper"_a)
    //     .def_property_readonly("lower", &Interval<TimeValue>::lower)
    //     .def_property_readonly("upper", &Interval<TimeValue>::upper)
    //     .def("__eq__", &Interval<TimeValue>::operator==)
    //     .def("__lt__", &Interval<TimeValue>::operator<)
    //     .def("__repr__", [](const Interval<TimeValue> &i)
    //          { return "TimeInterval(" + std::to_string(i.lower()) + ", " + std::to_string(i.upper()) + ")"; });

    // py::class_<Interval<Offset>>(m, "OffsetInterval")
    //     .def(py::init<Offset, Offset>(), "lower"_a, "upper"_a)
    //     .def_property_readonly("lower", &Interval<Offset>::lower)
    //     .def_property_readonly("upper", &Interval<Offset>::upper)
    //     .def("__eq__", &Interval<Offset>::operator==)
    //     .def("__lt__", &Interval<Offset>::operator<)
    //     .def("__repr__", [](const Interval<Offset> &i)
    //          { return "OffsetInterval(" + std::to_string(i.lower()) + ", " + std::to_string(i.upper()) + ")"; });

    // // Gap
    // py::class_<Gap>(m, "Gap")
    //     .def(py::init<>())
    //     .def(py::init([](TimeValue lower, TimeValue upper,
    //                      std::optional<Offset> window_lower = std::nullopt,
    //                      std::optional<Offset> window_upper = std::nullopt)
    //                   {
    //         Gap gap;
    //         gap.lifespan = {lower, upper};
    //         if (window_lower && window_upper) {
    //             gap.window = Interval<Offset>{*window_lower, *window_upper};
    //         }
    //         return gap; }),
    //          "lower"_a, "upper"_a, "window_lower"_a = py::none(), "window_upper"_a = py::none())
    //     .def_readwrite("lifespan", &Gap::lifespan)
    //     .def_readwrite("window", &Gap::window)
    //     .def("__eq__", &Gap::operator==)
    //     .def("__repr__", [](const Gap &g)
    //          {
    //         std::string repr = "Gap(lifespan=" + std::to_string(g.lifespan.lower()) +
    //                           "-" + std::to_string(g.lifespan.upper());
    //         if (g.window) {
    //             repr += ", window=" + std::to_string(g.window->lower()) +
    //                    "-" + std::to_string(g.window->upper());
    //         }
    //         return repr + ")"; });

    // Buffer
    py::class_<Buffer>(m, "Buffer")
        .def(py::init([](const std::string &id, TimeValue lower, TimeValue upper, int64_t size,
                         int64_t alignment = 1, std::optional<Offset> offset = std::nullopt,
                         std::optional<Offset> hint = std::nullopt,
                         const std::vector<Gap> &gaps = {})
                      {
            Buffer buffer;
            buffer.id = id;
            buffer.lifespan = {lower, upper};
            buffer.size = size;
            buffer.alignment = alignment;
            buffer.offset = offset;
            buffer.hint = hint;
            buffer.gaps = gaps;
            return buffer; }),
             "id"_a, "lower"_a, "upper"_a, "size"_a, "alignment"_a = 1,
             "offset"_a = py::none(), "hint"_a = py::none(), "gaps"_a = std::vector<Gap>{})
        .def_readwrite("id", &Buffer::id)
        .def_readwrite("lifespan", &Buffer::lifespan)
        .def_readwrite("size", &Buffer::size)
        .def_readwrite("alignment", &Buffer::alignment)
        .def_readwrite("offset", &Buffer::offset)
        .def_readwrite("hint", &Buffer::hint)
        .def_readwrite("gaps", &Buffer::gaps)
        .def("area", &Buffer::area)
        .def("effective_size", &Buffer::effective_size)
        .def("__eq__", &Buffer::operator==)
        .def("__repr__", [](const Buffer &b)
             { return "Buffer(id='" + b.id + "', lifespan=" +
                      std::to_string(b.lifespan.lower()) + "-" + std::to_string(b.lifespan.upper()) +
                      ", size=" + std::to_string(b.size) + ")"; });

    // // Solution
    // py::class_<Solution>(m, "Solution")
    //     .def(py::init<>())
    //     .def(py::init<const std::vector<Offset> &>(), "offsets"_a)
    //     .def_readwrite("offsets", &Solution::offsets)
    //     .def("__eq__", &Solution::operator==)
    //     .def("__len__", [](const Solution &s)
    //          { return s.offsets.size(); })
    //     .def("__getitem__", [](const Solution &s, size_t i)
    //          {
    //         if (i >= s.offsets.size()) throw py::index_error();
    //         return s.offsets[i]; })
    //     .def("__repr__", [](const Solution &s)
    //          {
    //         std::string repr = "Solution([";
    //         for (size_t i = 0; i < s.offsets.size(); ++i) {
    //             if (i > 0) repr += ", ";
    //             repr += std::to_string(s.offsets[i]);
    //         }
    //         return repr + "])"; });

    // // Problem
    // py::class_<Problem>(m, "Problem")
    //     .def(py::init<>())
    //     .def(py::init<const std::vector<Buffer> &, Capacity>(), "buffers"_a, "capacity"_a)
    //     .def_readwrite("buffers", &Problem::buffers)
    //     .def_readwrite("capacity", &Problem::capacity)
    //     .def("strip_solution", &Problem::strip_solution)
    //     .def("__eq__", &Problem::operator==)
    //     .def("__len__", [](const Problem &p)
    //          { return p.buffers.size(); })
    //     .def("__getitem__", [](const Problem &p, size_t i)
    //          {
    //         if (i >= p.buffers.size()) throw py::index_error();
    //         return p.buffers[i]; })
    //     .def_static("from_csv", [](const std::string &csv_content, Capacity capacity)
    //                 {
    //         auto result = FromCsv(csv_content);
    //         if (!result.ok()) {
    //             throw std::runtime_error("Failed to parse CSV: " + std::string(result.status().message()));
    //         }
    //         result->capacity = capacity;
    //         return *result; }, "csv_content"_a, "capacity"_a)
    //     .def("to_csv", [](const Problem &problem, const Solution &solution)
    //          {
    //         // Create a mutable copy to work with ToCsv
    //         Problem mutable_problem = problem;
    //         Solution mutable_solution = solution;
    //         return ToCsv(mutable_problem, &mutable_solution); }, "solution"_a)
    //     .def("__repr__", [](const Problem &p)
    //          { return "Problem(buffers=" + std::to_string(p.buffers.size()) +
    //                   ", capacity=" + std::to_string(p.capacity) + ")"; });

    // // SolverParams
    // py::class_<SolverParams>(m, "SolverParams")
    //     .def(py::init([](std::optional<double> timeout_seconds = std::nullopt,
    //                      bool canonical_only = true,
    //                      bool section_inference = true,
    //                      bool dynamic_ordering = true,
    //                      bool check_dominance = true,
    //                      bool unallocated_floor = true,
    //                      bool static_preordering = true,
    //                      bool dynamic_decomposition = true,
    //                      bool monotonic_floor = true,
    //                      bool hatless_pruning = true,
    //                      const std::vector<std::string> &preordering_heuristics = {"WAT", "TAW", "TWA"})
    //                   {
    //         SolverParams params;
    //         if (timeout_seconds) {
    //             params.timeout = absl::Seconds(*timeout_seconds);
    //         }
    //         params.canonical_only = canonical_only;
    //         params.section_inference = section_inference;
    //         params.dynamic_ordering = dynamic_ordering;
    //         params.check_dominance = check_dominance;
    //         params.unallocated_floor = unallocated_floor;
    //         params.static_preordering = static_preordering;
    //         params.dynamic_decomposition = dynamic_decomposition;
    //         params.monotonic_floor = monotonic_floor;
    //         params.hatless_pruning = hatless_pruning;
    //         params.preordering_heuristics = preordering_heuristics;
    //         return params; }),
    //          "timeout_seconds"_a = py::none(), "canonical_only"_a = true,
    //          "section_inference"_a = true, "dynamic_ordering"_a = true,
    //          "check_dominance"_a = true, "unallocated_floor"_a = true,
    //          "static_preordering"_a = true, "dynamic_decomposition"_a = true,
    //          "monotonic_floor"_a = true, "hatless_pruning"_a = true,
    //          "preordering_heuristics"_a = std::vector<std::string>{"WAT", "TAW", "TWA"})
    //     .def_readwrite("canonical_only", &SolverParams::canonical_only)
    //     .def_readwrite("section_inference", &SolverParams::section_inference)
    //     .def_readwrite("dynamic_ordering", &SolverParams::dynamic_ordering)
    //     .def_readwrite("check_dominance", &SolverParams::check_dominance)
    //     .def_readwrite("unallocated_floor", &SolverParams::unallocated_floor)
    //     .def_readwrite("static_preordering", &SolverParams::static_preordering)
    //     .def_readwrite("dynamic_decomposition", &SolverParams::dynamic_decomposition)
    //     .def_readwrite("monotonic_floor", &SolverParams::monotonic_floor)
    //     .def_readwrite("hatless_pruning", &SolverParams::hatless_pruning)
    //     .def_readwrite("preordering_heuristics", &SolverParams::preordering_heuristics);

    // // Solver
    // py::class_<Solver>(m, "Solver")
    //     .def(py::init<>())
    //     .def(py::init<const SolverParams &>(), "params"_a)
    //     .def("solve", [](Solver &solver, const Problem &problem)
    //          {
    //         auto result = solver.Solve(problem);
    //         if (!result.ok()) {
    //             throw std::runtime_error("Solve failed: " + std::string(result.status().message()));
    //         }
    //         return *result; }, "problem"_a)
    //     .def_property_readonly("backtracks", &Solver::get_backtracks)
    //     .def("cancel", &Solver::Cancel);

    // // ValidationResult enum
    // py::enum_<ValidationResult>(m, "ValidationResult")
    //     .value("GOOD", ValidationResult::kGood)
    //     .value("BAD_SOLUTION", ValidationResult::kBadSolution)
    //     .value("BAD_FIXED", ValidationResult::kBadFixed)
    //     .value("BAD_OFFSET", ValidationResult::kBadOffset)
    //     .value("BAD_OVERLAP", ValidationResult::kBadOverlap)
    //     .value("BAD_ALIGNMENT", ValidationResult::kBadAlignment);

    // // Validation function
    // m.def("validate_solution", &Validate, "problem"_a, "solution"_a,
    //       "Validates that a solution is correct for the given problem");
}