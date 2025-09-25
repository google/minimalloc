/*
Copyright 2023 Google LLC
Copyright 2025 Axelera AI

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

#include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "converter.h"
#include "minimalloc.h"
#include "solver.h"
#include "sweeper.h"
#include "validator.h"

namespace py = pybind11;
using namespace py::literals;
using namespace minimalloc;

namespace {
// Generic hash combiner using FNV-1a inspired mixing
template <typename T, typename... Args>
[[nodiscard]] constexpr size_t make_hash(const T &first,
                                         const Args &...args) noexcept {
  size_t seed = std::hash<T>{}(first);
  if constexpr (sizeof...(args) > 0) {
    seed ^= make_hash(args...) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

// Specialization for optional
template <typename T>
[[nodiscard]] size_t make_hash(const std::optional<T> &opt) noexcept {
  return opt ? std::hash<T>{}(*opt) : 0;
}

// StatusOr unwrapper
template <typename T>
[[nodiscard]] py::object unwrap_or_none(const absl::StatusOr<T> &result) {
  return result.ok() ? py::cast(std::move(*result)) : py::none();
}

}  // namespace

PYBIND11_MODULE(_minimalloc, m) {
  py::class_<absl::Duration>(m, "Duration")
      .def(py::init<>())
      .def(py::init([](double s) { return absl::Seconds(s); }), "seconds"_a)
      .def_static(
          "from_seconds", [](double s) { return absl::Seconds(s); },
          "seconds"_a)
      .def_static(
          "from_milliseconds", [](double ms) { return absl::Milliseconds(ms); },
          "milliseconds"_a)
      .def_static(
          "from_microseconds", [](double us) { return absl::Microseconds(us); },
          "microseconds"_a)
      .def_static("infinite", absl::InfiniteDuration)
      .def("to_seconds",
           [](const absl::Duration &d) { return absl::ToDoubleSeconds(d); })
      .def("__repr__",
           [](const absl::Duration &d) {
             return d == absl::InfiniteDuration()
                        ? py::str("Duration.infinite()")
                        : py::str("Duration({} seconds)")
                              .format(absl::ToDoubleSeconds(d));
           })
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self);

  py::class_<Interval<int64_t>>(m, "Interval")
      .def(py::init<int64_t, int64_t>(), "lower"_a, "upper"_a)
      .def(py::init<>())
      .def_property_readonly("lower", &Interval<int64_t>::lower)
      .def_property_readonly("upper", &Interval<int64_t>::upper)
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def("__hash__",
           [](const Interval<int64_t> &i) {
             return make_hash(i.lower(), i.upper());
           })
      .def("__repr__", [](const Interval<int64_t> &i) {
        return py::str("Interval({}, {})").format(i.lower(), i.upper());
      });

  using IntervalType = Interval<int64_t>;
  m.attr("Lifespan") = py::type::of<IntervalType>();
  m.attr("Window") = py::type::of<IntervalType>();
  m.attr("SectionRange") = py::type::of<IntervalType>();

  py::class_<Gap>(m, "Gap")
      .def(py::init<>())
      .def(py::init<Lifespan, std::optional<Window>>(), "lifespan"_a,
           "window"_a = std::nullopt)
      .def_readwrite("lifespan", &Gap::lifespan)
      .def_readwrite("window", &Gap::window)
      .def(py::self == py::self)
      .def(
          "__hash__",
          [](const Gap &g) {
            return make_hash(
                g.lifespan.lower(), g.lifespan.upper(),
                g.window ? make_hash(g.window->lower(), g.window->upper()) : 0);
          })
      .def("__repr__", [](const Gap &g) {
        return py::str("Gap(lifespan={}, window={})")
            .format(g.lifespan, g.window ? py::cast(*g.window) : py::none());
      });

  py::class_<Buffer>(m, "Buffer")
      .def(py::init<>())
      .def(py::init([](std::string id, Lifespan lifespan, int64_t size,
                       int64_t alignment, std::vector<Gap> gaps,
                       std::optional<Offset> offset,
                       std::optional<Offset> hint) {
             return Buffer{std::move(id),   lifespan, size, alignment,
                           std::move(gaps), offset,   hint};
           }),
           "id"_a, "lifespan"_a, "size"_a, "alignment"_a = 1,
           "gaps"_a = std::vector<Gap>{}, "offset"_a = std::nullopt,
           "hint"_a = std::nullopt)
      .def_readwrite("id", &Buffer::id)
      .def_readwrite("lifespan", &Buffer::lifespan)
      .def_readwrite("size", &Buffer::size)
      .def_readwrite("alignment", &Buffer::alignment)
      .def_readwrite("gaps", &Buffer::gaps)
      .def_readwrite("offset", &Buffer::offset)
      .def_readwrite("hint", &Buffer::hint)
      .def("area", &Buffer::area)
      .def(
          "effective_size",
          [](const Buffer &self, const Buffer &other) {
            return self.effective_size(other);
          },
          "other"_a)
      .def(py::self == py::self)
      .def("__hash__",
           [](const Buffer &b) {
             size_t seed = make_hash(b.id, b.lifespan.lower(),
                                     b.lifespan.upper(), b.size, b.alignment);
             for (const auto &gap : b.gaps) {
               seed ^= make_hash(gap.lifespan.lower(), gap.lifespan.upper());
             }
             return seed ^ make_hash(b.offset) ^ make_hash(b.hint);
           })
      .def("__repr__", [](const Buffer &b) {
        return py::str(
                   "Buffer(id='{}', lifespan={}, size={}, alignment={}, "
                   "gaps={}, offset={}, hint={})")
            .format(b.id, b.lifespan, b.size, b.alignment, b.gaps,
                    b.offset ? py::cast(*b.offset) : py::none(),
                    b.hint ? py::cast(*b.hint) : py::none());
      });

  py::class_<Solution>(m, "Solution")
      .def(py::init<>())
      .def(py::init<std::vector<Offset>>(), "offsets"_a)
      .def(py::init<std::vector<Offset>, Offset>(), "offsets"_a, "height"_a)
      .def_readwrite("offsets", &Solution::offsets)
      .def_readwrite("height", &Solution::height)
      .def(py::self == py::self)
      .def("__hash__",
           [](const Solution &s) {
             size_t seed = 0;
             for (auto off : s.offsets) seed ^= make_hash(off);
             seed ^= std::hash<Offset>{}(s.height);
             return seed;
           })
      .def("__repr__", [](const Solution &s) {
        return py::str("Solution(offsets={}, height={})")
            .format(s.offsets, s.height);
      });

  py::class_<Problem>(m, "Problem")
      .def(py::init<>())
      .def(py::init<std::vector<Buffer>, Capacity>(), "buffers"_a,
           "capacity"_a = 0)
      .def_readwrite("buffers", &Problem::buffers)
      .def_readwrite("capacity", &Problem::capacity)
      .def("strip_solution",
           [](Problem &p) { return unwrap_or_none(p.strip_solution()); })
      .def(py::self == py::self)
      .def("__hash__",
           [](const Problem &p) {
             size_t seed = make_hash(p.capacity);
             for (const auto &buf : p.buffers) {
               seed ^= make_hash(buf.id, buf.size);
             }
             return seed;
           })
      .def("__repr__", [](const Problem &p) {
        return py::str("Problem(buffers={}, capacity={})")
            .format(p.buffers, p.capacity);
      });

  py::class_<SolverParams>(m, "SolverParams")
      .def(py::init<>())
      .def_property(
          "timeout", [](const SolverParams &self) { return self.timeout; },
          [](SolverParams &self, py::object value) {
            if (py::isinstance<absl::Duration>(value)) {
              self.timeout = value.cast<absl::Duration>();
            } else if (py::isinstance<py::float_>(value) ||
                       py::isinstance<py::int_>(value)) {
              self.timeout = absl::Seconds(value.cast<double>());
            } else if (value.is_none()) {
              self.timeout = absl::InfiniteDuration();
            } else {
              throw py::type_error(
                  "timeout must be Duration, numeric, or None");
            }
          })
      .def_readwrite("canonical_only", &SolverParams::canonical_only)
      .def_readwrite("section_inference", &SolverParams::section_inference)
      .def_readwrite("dynamic_ordering", &SolverParams::dynamic_ordering)
      .def_readwrite("check_dominance", &SolverParams::check_dominance)
      .def_readwrite("unallocated_floor", &SolverParams::unallocated_floor)
      .def_readwrite("static_preordering", &SolverParams::static_preordering)
      .def_readwrite("dynamic_decomposition",
                     &SolverParams::dynamic_decomposition)
      .def_readwrite("monotonic_floor", &SolverParams::monotonic_floor)
      .def_readwrite("hatless_pruning", &SolverParams::hatless_pruning)
      .def_readwrite("minimize_capacity", &SolverParams::minimize_capacity)
      .def_readwrite("preordering_heuristics",
                     &SolverParams::preordering_heuristics);

  py::class_<PreorderData>(m, "PreorderData")
      .def(py::init<>())
      .def(py::init([](Area area, TimeValue lower, uint64_t overlaps,
                       int sections, int64_t size, int total, TimeValue upper,
                       int64_t width, BufferIdx buffer_idx) {
             PreorderData data;
             data.area = area;
             data.lower = lower;
             data.overlaps = overlaps;
             data.sections = sections;
             data.size = size;
             data.total = total;
             data.upper = upper;
             data.width = width;
             data.buffer_idx = buffer_idx;
             return data;
           }),
           "area"_a, "lower"_a, "overlaps"_a, "sections"_a, "size"_a, "total"_a,
           "upper"_a, "width"_a, "buffer_idx"_a)
      .def_readwrite("area", &PreorderData::area)
      .def_readwrite("lower", &PreorderData::lower)
      .def_readwrite("overlaps", &PreorderData::overlaps)
      .def_readwrite("sections", &PreorderData::sections)
      .def_readwrite("size", &PreorderData::size)
      .def_readwrite("total", &PreorderData::total)
      .def_readwrite("upper", &PreorderData::upper)
      .def_readwrite("width", &PreorderData::width)
      .def_readwrite("buffer_idx", &PreorderData::buffer_idx);

  py::class_<PreorderingComparator>(m, "PreorderingComparator")
      .def(py::init<const std::string &>(), "heuristic"_a)
      .def("__call__", &PreorderingComparator::operator(), "lhs"_a, "rhs"_a);

  py::class_<Solver>(m, "Solver")
      .def(py::init<>())
      .def(py::init<const SolverParams &>(), "params"_a)
      .def(
          "solve",
          [](Solver &self, const Problem &problem) {
            return unwrap_or_none(self.Solve(problem));
          },
          "problem"_a)
      .def("get_backtracks", &Solver::get_backtracks)
      .def("cancel", &Solver::Cancel)
      .def(
          "compute_irreducible_infeasible_subset",
          [](Solver &self, const Problem &problem) {
            return unwrap_or_none(
                self.ComputeIrreducibleInfeasibleSubset(problem));
          },
          "problem"_a);

  py::enum_<ValidationResult>(m, "ValidationResult")
      .value("GOOD", ValidationResult::kGood)
      .value("BAD_SOLUTION", ValidationResult::kBadSolution)
      .value("BAD_FIXED", ValidationResult::kBadFixed)
      .value("BAD_OFFSET", ValidationResult::kBadOffset)
      .value("BAD_OVERLAP", ValidationResult::kBadOverlap)
      .value("BAD_ALIGNMENT", ValidationResult::kBadAlignment)
      .value("BAD_HEIGHT", ValidationResult::kBadHeight);

  m.def("validate", &Validate, "problem"_a, "solution"_a);

  m.def(
      "to_csv",
      [](const Problem &problem, std::optional<Solution> solution,
         bool old_format) {
        return solution ? ToCsv(problem, &solution.value(), old_format)
                        : ToCsv(problem, nullptr, old_format);
      },
      "problem"_a, "solution"_a = std::nullopt, "old_format"_a = false);

  m.def(
      "from_csv",
      [](std::string_view input) { return unwrap_or_none(FromCsv(input)); },
      "input"_a);

  py::class_<SectionSpan>(m, "SectionSpan")
      .def(py::init<>())
      .def(py::init<SectionRange, Window>(), "section_range"_a, "window"_a)
      .def_readwrite("section_range", &SectionSpan::section_range)
      .def_readwrite("window", &SectionSpan::window)
      .def(py::self == py::self)
      .def("__hash__",
           [](const SectionSpan &s) {
             return make_hash(s.section_range.lower(), s.section_range.upper(),
                              s.window.lower(), s.window.upper());
           })
      .def("__repr__", [](const SectionSpan &s) {
        return py::str("SectionSpan(section_range={}, window={})")
            .format(s.section_range, s.window);
      });

  py::class_<Partition>(m, "Partition")
      .def(py::init<>())
      .def(py::init<std::vector<BufferIdx>, SectionRange>(), "buffer_idxs"_a,
           "section_range"_a)
      .def_readwrite("buffer_idxs", &Partition::buffer_idxs)
      .def_readwrite("section_range", &Partition::section_range)
      .def(py::self == py::self)
      .def("__hash__",
           [](const Partition &p) {
             size_t seed =
                 make_hash(p.section_range.lower(), p.section_range.upper());
             for (auto idx : p.buffer_idxs) seed ^= make_hash(idx);
             return seed;
           })
      .def("__repr__", [](const Partition &p) {
        return py::str("Partition(buffer_idxs={}, section_range={})")
            .format(p.buffer_idxs, p.section_range);
      });

  py::class_<Overlap>(m, "Overlap")
      .def(py::init<>())
      .def(py::init<BufferIdx, int64_t>(), "buffer_idx"_a = -1,
           "effective_size"_a = 0)
      .def_readwrite("buffer_idx", &Overlap::buffer_idx)
      .def_readwrite("effective_size", &Overlap::effective_size)
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def("__hash__",
           [](const Overlap &o) {
             return make_hash(o.buffer_idx, o.effective_size);
           })
      .def("__repr__", [](const Overlap &o) {
        return py::str("Overlap(buffer_idx={}, effective_size={})")
            .format(o.buffer_idx, o.effective_size);
      });

  py::class_<BufferData>(m, "BufferData")
      .def(py::init<>())
      .def(py::init([](std::vector<SectionSpan> section_spans,
                       std::optional<py::iterable> overlaps) {
             BufferData data;
             data.section_spans = std::move(section_spans);
             if (overlaps) {
               for (const auto &item : *overlaps) {
                 data.overlaps.insert(item.cast<Overlap>());
               }
             }
             return data;
           }),
           "section_spans"_a = std::vector<SectionSpan>{},
           "overlaps"_a = std::nullopt)
      .def_readwrite("section_spans", &BufferData::section_spans)
      .def_property(
          "overlaps",
          [](const BufferData &self) {
            py::set py_overlaps;
            for (const auto &overlap : self.overlaps) {
              py_overlaps.add(py::cast(overlap));
            }
            return py_overlaps;
          },
          [](BufferData &self, const py::iterable &overlaps) {
            self.overlaps.clear();
            for (const auto &item : overlaps) {
              self.overlaps.insert(item.cast<Overlap>());
            }
          })
      .def(py::self == py::self)
      .def("__hash__",
           [](const BufferData &b) {
             size_t seed = 0;
             for (const auto &span : b.section_spans) {
               seed ^= make_hash(span.section_range.lower(),
                                 span.section_range.upper());
             }
             for (const auto &overlap : b.overlaps) {
               seed ^= make_hash(overlap.buffer_idx, overlap.effective_size);
             }
             return seed;
           })
      .def("__repr__", [](const BufferData &b) {
        py::set py_overlaps;
        for (const auto &overlap : b.overlaps) {
          py_overlaps.add(py::cast(overlap));
        }
        return py::str("BufferData(section_spans={}, overlaps={})")
            .format(b.section_spans, py_overlaps);
      });

  py::class_<SweepResult>(m, "SweepResult")
      .def(py::init<>())
      .def(py::init([](std::optional<py::list> sections,
                       std::vector<Partition> partitions,
                       std::vector<BufferData> buffer_data) {
             SweepResult result;
             if (sections) {
               result.sections.reserve(sections->size());
               for (const auto &item : *sections) {
                 Section section;
                 for (const auto &idx : item.cast<py::iterable>()) {
                   section.insert(idx.cast<BufferIdx>());
                 }
                 result.sections.push_back(std::move(section));
               }
             }
             result.partitions = std::move(partitions);
             result.buffer_data = std::move(buffer_data);
             return result;
           }),
           "sections"_a = std::nullopt,
           "partitions"_a = std::vector<Partition>{},
           "buffer_data"_a = std::vector<BufferData>{})
      .def_property(
          "sections",
          [](const SweepResult &self) {
            py::list sections;
            for (const auto &section : self.sections) {
              py::set py_section;
              for (const auto &idx : section) py_section.add(py::int_(idx));
              sections.append(py_section);
            }
            return sections;
          },
          [](SweepResult &self, const py::list &sections) {
            self.sections.clear();
            self.sections.reserve(sections.size());
            for (const auto &item : sections) {
              Section section;
              for (const auto &idx : item.cast<py::iterable>()) {
                section.insert(idx.cast<BufferIdx>());
              }
              self.sections.push_back(std::move(section));
            }
          })
      .def_readwrite("partitions", &SweepResult::partitions)
      .def_readwrite("buffer_data", &SweepResult::buffer_data)
      .def("calculate_cuts", &SweepResult::CalculateCuts)
      .def("__eq__", [](const SweepResult &self, const SweepResult &other) {
        return self.sections == other.sections &&
               self.partitions == other.partitions &&
               self.buffer_data == other.buffer_data;
      });

  py::enum_<SweepPointType>(m, "SweepPointType")
      .value("LEFT", SweepPointType::kLeft)
      .value("RIGHT", SweepPointType::kRight);

  py::class_<SweepPoint>(m, "SweepPoint")
      .def(py::init<>())
      .def(py::init<BufferIdx, TimeValue, SweepPointType, Window, bool>(),
           "buffer_idx"_a, "time_value"_a, "point_type"_a, "window"_a,
           "endpoint"_a = false)
      .def_readwrite("buffer_idx", &SweepPoint::buffer_idx)
      .def_readwrite("time_value", &SweepPoint::time_value)
      .def_readwrite("point_type", &SweepPoint::point_type)
      .def_readwrite("window", &SweepPoint::window)
      .def_readwrite("endpoint", &SweepPoint::endpoint)
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def("__hash__",
           [](const SweepPoint &p) {
             return make_hash(p.buffer_idx, p.time_value,
                              static_cast<int>(p.point_type), p.window.lower(),
                              p.window.upper(), p.endpoint);
           })
      .def("__repr__", [](const SweepPoint &p) {
        return py::str(
                   "SweepPoint(buffer_idx={}, time_value={}, point_type={}, "
                   "window={}, endpoint={})")
            .format(p.buffer_idx, p.time_value,
                    p.point_type == SweepPointType::kLeft ? "LEFT" : "RIGHT",
                    p.window, p.endpoint);
      });

  m.def("create_points", &CreatePoints, "problem"_a);
  m.def("sweep", &Sweep, "problem"_a);
}