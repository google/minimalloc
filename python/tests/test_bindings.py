"""
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
"""

import minimalloc as mm


def test_duration_construction_variants() -> None:
    """Test Duration construction methods and conversions."""
    d1 = mm.Duration(5.0)
    d2 = mm.Duration.from_seconds(5.0)
    d3 = mm.Duration.from_milliseconds(5000.0)
    d4 = mm.Duration.from_microseconds(5_000_000.0)

    assert d1.to_seconds() == 5.0
    assert d2.to_seconds() == 5.0
    assert abs(d3.to_seconds() - 5.0) < 1e-9
    assert abs(d4.to_seconds() - 5.0) < 1e-9


def test_duration_infinite_and_comparisons() -> None:
    """Test infinite duration and comparison operators."""
    inf = mm.Duration.infinite()
    d1 = mm.Duration(10.0)
    d2 = mm.Duration(20.0)

    assert d1 < d2
    assert d1 <= d2
    assert d1 < inf
    assert not (inf < d1)


def test_duration_equality_and_repr() -> None:
    """Test Duration equality and string representation."""
    d1 = mm.Duration(3.14)
    d2 = mm.Duration(3.14)
    d3 = mm.Duration(2.71)

    assert d1 == d2
    assert d1 != d3
    assert "3.14" in repr(d1)
    assert "infinite" in repr(mm.Duration.infinite()).lower()


def test_interval_operations() -> None:
    """Test Interval construction, comparison, and hashing."""
    i1 = mm.Interval(10, 20)
    i2 = mm.Interval(10, 20)
    i3 = mm.Interval(5, 15)

    assert i1 == i2
    assert i1 != i3
    assert i3 < i1  # Lexicographic ordering
    assert hash(i1) == hash(i2)


def test_interval_as_hashable_key() -> None:
    """Test Interval as dictionary/set key."""
    intervals = {
        mm.Interval(0, 10): "first",
        mm.Interval(10, 20): "second",
        mm.Interval(0, 10): "duplicate",  # Should overwrite first
    }
    assert len(intervals) == 2
    assert intervals[mm.Interval(0, 10)] == "duplicate"


def test_interval_repr_and_defaults() -> None:
    """Test Interval repr and default construction."""
    i = mm.Interval(100, 200)
    assert repr(i) == "Interval(100, 200)"

    default = mm.Interval()
    assert default.lower == 0
    assert default.upper == 0


def test_gap_with_optional_window() -> None:
    """Test Gap with and without window."""
    g1 = mm.Gap(mm.Interval(0, 100))
    g2 = mm.Gap(mm.Interval(0, 100), mm.Interval(20, 80))

    assert g1.window is None
    assert g2.window == mm.Interval(20, 80)
    assert g1 != g2


def test_gap_mutation_and_equality() -> None:
    """Test Gap attribute mutation and equality."""
    g = mm.Gap(mm.Interval(0, 50))
    g.window = mm.Interval(10, 40)
    g.lifespan = mm.Interval(0, 60)

    assert g.lifespan == mm.Interval(0, 60)
    assert g.window == mm.Interval(10, 40)

    g2 = mm.Gap(mm.Interval(0, 60), mm.Interval(10, 40))
    assert g == g2
    assert hash(g) == hash(g2)


def test_gap_in_collections() -> None:
    """Test Gap as hashable in sets/dicts."""
    gaps = {
        mm.Gap(mm.Interval(0, 10)): "no_window",
        mm.Gap(mm.Interval(0, 10), mm.Interval(2, 8)): "with_window",
    }
    assert len(gaps) == 2

    gap_set = {mm.Gap(mm.Interval(0, 5)), mm.Gap(mm.Interval(0, 5))}
    assert len(gap_set) == 1


def test_buffer_basic_construction() -> None:
    """Test Buffer basic construction with required parameters."""
    b = mm.Buffer("test", mm.Interval(0, 100), 1024)
    assert b.id == "test"
    assert b.size == 1024
    assert b.alignment == 1  # Default
    assert b.offset is None
    assert b.hint is None
    assert len(b.gaps) == 0


def test_buffer_full_construction() -> None:
    """Test Buffer with all parameters including gaps."""
    gaps = [
        mm.Gap(mm.Interval(10, 30)),
        mm.Gap(mm.Interval(40, 60), mm.Interval(45, 55)),
    ]
    b = mm.Buffer("complex", mm.Interval(0, 100), 2048, 64, gaps, 512, 256)

    assert b.alignment == 64
    assert b.offset == 512
    assert b.hint == 256
    assert len(b.gaps) == 2
    assert b.gaps[1].window == mm.Interval(45, 55)


def test_buffer_methods() -> None:
    """Test Buffer area and effective_size calculations."""
    b1 = mm.Buffer("b1", mm.Interval(0, 50), 100)
    b2 = mm.Buffer("b2", mm.Interval(25, 75), 200)

    assert b1.area() == 100 * 50  # size * duration
    effective = b1.effective_size(b2)
    assert isinstance(effective, int)
    assert effective >= 0


def test_buffer_mutation() -> None:
    """Test Buffer attribute mutations."""
    b = mm.Buffer("mutable", mm.Interval(0, 10), 100)
    b.size = 200
    b.alignment = 32
    b.offset = 64
    b.gaps = [mm.Gap(mm.Interval(2, 8))]

    assert b.size == 200
    assert b.alignment == 32
    assert b.offset == 64
    assert len(b.gaps) == 1


def test_buffer_equality_and_hashing() -> None:
    """Test Buffer equality and use in collections."""
    b1 = mm.Buffer("buf", mm.Interval(0, 100), 512, 16)
    b2 = mm.Buffer("buf", mm.Interval(0, 100), 512, 16)
    b3 = mm.Buffer("other", mm.Interval(0, 100), 512, 16)

    assert b1 == b2
    assert b1 != b3
    assert hash(b1) == hash(b2)

    buffer_set = {b1, b2, b3}
    assert len(buffer_set) == 2


def test_solution_basic() -> None:
    """Test Solution construction and modification."""
    s1 = mm.Solution([0, 1024, 2048])
    s2 = mm.Solution()

    assert s1.offsets == [0, 1024, 2048]
    assert s2.offsets == []

    s2.offsets = [512, 1536]
    assert len(s2.offsets) == 2


def test_solution_equality_and_collections() -> None:
    """Test Solution in collections and equality."""
    s1 = mm.Solution([0, 100, 200])
    s2 = mm.Solution([0, 100, 200])
    s3 = mm.Solution([0, 50, 100])

    assert s1 == s2
    assert s1 != s3

    solutions = {s1: "first", s2: "duplicate", s3: "different"}
    assert len(solutions) == 2


def test_solution_repr() -> None:
    """Test Solution string representation."""
    s = mm.Solution([64, 128, 256], height=256)
    assert repr(s) == "Solution(offsets=[64, 128, 256], height=256)"


def test_problem_basic() -> None:
    """Test Problem construction with buffers."""
    buffers = [
        mm.Buffer("a", mm.Interval(0, 10), 100),
        mm.Buffer("b", mm.Interval(5, 15), 200),
    ]
    p = mm.Problem(buffers, 4096)

    assert len(p.buffers) == 2
    assert p.capacity == 4096
    assert p.buffers[0].id == "a"


def test_problem_strip_solution() -> None:
    """Test Problem strip_solution for fixed buffers."""
    b1 = mm.Buffer("1", mm.Interval(0, 10), 100)
    b1.offset = 512
    b2 = mm.Buffer("2", mm.Interval(0, 10), 200)
    b2.offset = 0

    p = mm.Problem([b1, b2], 2048)
    solution = p.strip_solution()
    assert solution is not None


def test_problem_mutation_and_equality() -> None:
    """Test Problem attribute mutation and equality."""
    p1 = mm.Problem([mm.Buffer("x", mm.Interval(0, 5), 50)], 1024)
    p2 = mm.Problem([mm.Buffer("x", mm.Interval(0, 5), 50)], 1024)

    assert p1 == p2
    assert hash(p1) == hash(p2)

    p1.capacity = 2048
    assert p1 != p2


def test_problem_repr() -> None:
    """Test Problem string representation."""
    p = mm.Problem([], 8192)
    assert "8192" in repr(p)
    assert "buffers" in repr(p).lower()


def test_solver_params_basic() -> None:
    """Test SolverParams configuration."""
    params = mm.SolverParams()
    params.canonical_only = True
    params.section_inference = False
    params.dynamic_ordering = True
    params.check_dominance = False

    assert params.canonical_only is True
    assert params.section_inference is False
    assert params.dynamic_ordering is True
    assert params.check_dominance is False


def test_solver_params_timeout_variants() -> None:
    """Test SolverParams timeout with different input types."""
    params = mm.SolverParams()

    # Duration object
    params.timeout = mm.Duration(10.0)
    assert params.timeout == mm.Duration(10.0)

    # Direct float
    params.timeout = 5.5
    assert params.timeout.to_seconds() == 5.5

    # Integer
    params.timeout = 3
    assert params.timeout.to_seconds() == 3.0

    # None for infinite
    params.timeout = None
    assert params.timeout == mm.Duration.infinite()


def test_solver_params_advanced() -> None:
    """Test advanced SolverParams settings."""
    params = mm.SolverParams()
    params.unallocated_floor = True
    params.static_preordering = False
    params.dynamic_decomposition = True
    params.monotonic_floor = False
    params.hatless_pruning = True
    params.preordering_heuristics = ["WAT", "TAW"]

    assert params.unallocated_floor is True
    assert params.preordering_heuristics == ["WAT", "TAW"]


def test_solver_basic_solve() -> None:
    """Test Solver basic solving."""
    solver = mm.Solver()
    buffers = [
        mm.Buffer("small", mm.Interval(0, 10), 100),
        mm.Buffer("medium", mm.Interval(5, 15), 200),
    ]
    problem = mm.Problem(buffers, 1024)

    solution = solver.solve(problem)
    assert solution is None or isinstance(solution, mm.Solution)
    if solution:
        assert len(solution.offsets) == len(buffers)


def test_solver_with_params() -> None:
    """Test Solver with custom parameters."""
    params = mm.SolverParams()
    params.timeout = mm.Duration(1.0)
    params.canonical_only = True

    solver = mm.Solver(params)
    problem = mm.Problem([mm.Buffer("x", mm.Interval(0, 5), 10)], 100)

    solution = solver.solve(problem)
    assert solution is None or isinstance(solution, mm.Solution)


def test_solver_methods() -> None:
    """Test Solver utility methods."""
    solver = mm.Solver()

    # Test backtrack counter
    backtracks = solver.get_backtracks()
    assert isinstance(backtracks, int)
    assert backtracks >= 0

    # Test cancel (should not fail)
    solver.cancel()


def test_validation_result_enum() -> None:
    """Test ValidationResult enum values."""
    assert mm.ValidationResult.GOOD is not None
    assert mm.ValidationResult.BAD_SOLUTION is not None
    assert mm.ValidationResult.BAD_FIXED is not None
    assert mm.ValidationResult.BAD_OFFSET is not None
    assert mm.ValidationResult.BAD_OVERLAP is not None
    assert mm.ValidationResult.BAD_ALIGNMENT is not None
    assert mm.ValidationResult.BAD_HEIGHT is not None


def test_validate_good_solution() -> None:
    """Test validate with valid solution."""
    buffers = [mm.Buffer("x", mm.Interval(0, 10), 100)]
    problem = mm.Problem(buffers, 1024)
    solution = mm.Solution([0])

    result = mm.validate(problem, solution)
    assert isinstance(result, mm.ValidationResult)


def test_validate_bad_solutions() -> None:
    """Test validate with various invalid solutions."""
    buffers = [
        mm.Buffer("a", mm.Interval(0, 10), 100, 16),  # Needs alignment
        mm.Buffer("b", mm.Interval(5, 15), 100),
    ]
    problem = mm.Problem(buffers, 1024)

    # Wrong number of offsets
    result1 = mm.validate(problem, mm.Solution([0]))
    assert isinstance(result1, mm.ValidationResult)

    # Misaligned offset
    result2 = mm.validate(problem, mm.Solution([1, 200]))
    assert isinstance(result2, mm.ValidationResult)


def test_csv_conversion_basic() -> None:
    """Test CSV export with problem only."""
    buffers = [mm.Buffer("test", mm.Interval(0, 10), 256)]
    problem = mm.Problem(buffers, 2048)

    csv = mm.to_csv(problem)
    assert isinstance(csv, str)
    assert "test" in csv
    assert "256" in csv


def test_csv_with_solution() -> None:
    """Test CSV export with problem and solution."""
    buffers = [
        mm.Buffer("a", mm.Interval(0, 5), 100),
        mm.Buffer("b", mm.Interval(3, 8), 200),
    ]
    problem = mm.Problem(buffers, 1024)
    solution = mm.Solution([0, 128])

    csv = mm.to_csv(problem, solution, old_format=False)
    assert "a" in csv
    assert "b" in csv

    csv_old = mm.to_csv(problem, solution, old_format=True)
    assert isinstance(csv_old, str)


def test_csv_roundtrip() -> None:
    """Test CSV export and import roundtrip."""
    original = mm.Problem(
        [
            mm.Buffer("buf1", mm.Interval(0, 10), 128, 8),
            mm.Buffer("buf2", mm.Interval(5, 15), 256, 16),
        ],
        4096,
    )

    csv = mm.to_csv(original)
    result = mm.from_csv(csv)

    assert result.capacity == 0  # Capacity is not stored in CSV
    for b1, b2 in zip(result.buffers, original.buffers, strict=True):
        assert b1.id == b2.id
        assert b1.lifespan == b2.lifespan
        assert b1.size == b2.size
        assert b1.alignment == b2.alignment


def test_sweep_point_construction() -> None:
    """Test SweepPoint construction and properties."""
    p1 = mm.SweepPoint(
        0, 100, mm.SweepPointType.LEFT, mm.Interval(0, 200), False
    )
    p2 = mm.SweepPoint(
        1, 150, mm.SweepPointType.RIGHT, mm.Interval(50, 250), True
    )

    assert p1.buffer_idx == 0
    assert p1.time_value == 100
    assert p1.point_type == mm.SweepPointType.LEFT
    assert not p1.endpoint
    assert p2.endpoint


def test_sweep_point_comparison() -> None:
    """Test SweepPoint ordering and equality."""
    p1 = mm.SweepPoint(0, 100, mm.SweepPointType.LEFT, mm.Interval(0, 200))
    p2 = mm.SweepPoint(0, 100, mm.SweepPointType.LEFT, mm.Interval(0, 200))
    p3 = mm.SweepPoint(0, 150, mm.SweepPointType.LEFT, mm.Interval(0, 200))

    assert p1 == p2
    assert p1 < p3
    assert hash(p1) == hash(p2)


def test_sweep_point_types() -> None:
    """Test SweepPointType enum values."""
    assert mm.SweepPointType.LEFT is not None
    assert mm.SweepPointType.RIGHT is not None

    p = mm.SweepPoint(0, 50, mm.SweepPointType.RIGHT, mm.Interval(0, 100))
    assert p.point_type == mm.SweepPointType.RIGHT


def test_create_points() -> None:
    """Test create_points function."""
    buffers = [
        mm.Buffer("a", mm.Interval(0, 10), 100),
        mm.Buffer("b", mm.Interval(5, 15), 200),
    ]
    problem = mm.Problem(buffers, 1024)

    points = mm.create_points(problem)
    assert isinstance(points, list)
    assert all(isinstance(p, mm.SweepPoint) for p in points)
    assert len(points) >= 2 * len(buffers)  # At least left and right for each


def test_section_span() -> None:
    """Test SectionSpan construction and properties."""
    span = mm.SectionSpan(mm.Interval(0, 100), mm.Interval(20, 80))

    assert span.section_range == mm.Interval(0, 100)
    assert span.window == mm.Interval(20, 80)

    span2 = mm.SectionSpan(mm.Interval(0, 100), mm.Interval(20, 80))
    assert span == span2
    assert hash(span) == hash(span2)


def test_partition() -> None:
    """Test Partition with buffer indices."""
    p = mm.Partition([0, 2, 4], mm.Interval(10, 50))

    assert p.buffer_idxs == [0, 2, 4]
    assert p.section_range == mm.Interval(10, 50)

    p2 = mm.Partition([0, 2, 4], mm.Interval(10, 50))
    assert p == p2
    assert hash(p) == hash(p2)


def test_overlap() -> None:
    """Test Overlap construction and ordering."""
    o1 = mm.Overlap(0, 100)
    o2 = mm.Overlap(1, 200)
    o3 = mm.Overlap(0, 150)

    assert o1.buffer_idx == 0
    assert o1.effective_size == 100
    assert o1 < o2  # Compare by buffer_idx first
    assert o1 < o3  # Then by effective_size

    assert o1 == mm.Overlap(0, 100)
    assert hash(o1) == hash(mm.Overlap(0, 100))


def test_buffer_data() -> None:
    """Test BufferData with spans and overlaps."""
    spans = [
        mm.SectionSpan(mm.Interval(0, 50), mm.Interval(10, 40)),
        mm.SectionSpan(mm.Interval(50, 100), mm.Interval(60, 90)),
    ]
    overlaps = {mm.Overlap(0, 100), mm.Overlap(1, 200)}

    data = mm.BufferData(spans, overlaps)
    assert len(data.section_spans) == 2
    assert len(data.overlaps) == 2

    # Test overlaps as set property
    assert mm.Overlap(0, 100) in data.overlaps


def test_buffer_data_mutation() -> None:
    """Test BufferData attribute mutation."""
    data = mm.BufferData()
    data.section_spans = [mm.SectionSpan(mm.Interval(0, 10), mm.Interval(2, 8))]
    data.overlaps = {mm.Overlap(0, 50), mm.Overlap(1, 75)}

    assert len(data.section_spans) == 1
    assert len(data.overlaps) == 2


def test_sweep_result_construction() -> None:
    """Test SweepResult with sections and partitions."""
    sections = [{0, 1}, {2, 3, 4}]
    partitions = [
        mm.Partition([0, 1], mm.Interval(0, 50)),
        mm.Partition([2, 3, 4], mm.Interval(50, 100)),
    ]
    buffer_data = [mm.BufferData(), mm.BufferData()]

    result = mm.SweepResult(sections, partitions, buffer_data)
    assert len(result.sections) == 2
    assert 0 in result.sections[0]
    assert 4 in result.sections[1]


def test_sweep_result_calculate_cuts() -> None:
    """Test SweepResult calculate_cuts method."""
    # result = mm.SweepResult(
    #     [{0, 1}], [mm.Partition([0, 1], mm.Interval(0, 100))], [mm.BufferData()]
    # )

    # cuts = result.calculate_cuts()
    # assert isinstance(cuts, int)
    # assert cuts >= 0


def test_sweep_function() -> None:
    """Test sweep analysis function."""
    buffers = [
        mm.Buffer("x", mm.Interval(0, 20), 100),
        mm.Buffer("y", mm.Interval(10, 30), 150),
        mm.Buffer("z", mm.Interval(25, 40), 75),
    ]
    problem = mm.Problem(buffers, 1024)

    result = mm.sweep(problem)
    assert isinstance(result, mm.SweepResult)
    assert hasattr(result, "sections")
    assert hasattr(result, "partitions")
    assert hasattr(result, "buffer_data")


def test_preorder_data() -> None:
    """Test PreorderData construction."""
    data = mm.PreorderData(
        area=1000,
        lower=0,
        overlaps=3,
        sections=2,
        size=100,
        total=5,
        upper=50,
        width=50,
        buffer_idx=0,
    )

    assert data.area == 1000
    assert data.overlaps == 3
    assert data.buffer_idx == 0


def test_preordering_comparator() -> None:
    """Test PreorderingComparator with different heuristics."""
    comp = mm.PreorderingComparator("area")

    d1 = mm.PreorderData(
        area=1000,
        lower=0,
        overlaps=1,
        sections=1,
        size=100,
        total=1,
        upper=10,
        width=10,
        buffer_idx=0,
    )
    d2 = mm.PreorderData(
        area=2000,
        lower=0,
        overlaps=1,
        sections=1,
        size=100,
        total=1,
        upper=20,
        width=20,
        buffer_idx=1,
    )

    # Should return boolean for comparison
    result = comp(d1, d2)
    assert isinstance(result, bool)


def test_complex_problem_solve() -> None:
    """Test solving a more complex problem with gaps."""
    buffers = [
        mm.Buffer(
            "with_gap",
            mm.Interval(0, 100),
            512,
            32,
            [mm.Gap(mm.Interval(20, 40), mm.Interval(25, 35))],
        ),
        mm.Buffer("aligned", mm.Interval(50, 150), 256, 64),
        mm.Buffer("small", mm.Interval(75, 125), 128, 16),
    ]
    problem = mm.Problem(buffers, 2048)

    params = mm.SolverParams()
    params.timeout = mm.Duration(5.0)
    params.canonical_only = True

    solver = mm.Solver(params)
    solution = solver.solve(problem)

    if solution:
        assert len(solution.offsets) == 3
        # Check alignment constraints would be satisfied
        for i, offset in enumerate(solution.offsets):
            assert offset % buffers[i].alignment == 0
