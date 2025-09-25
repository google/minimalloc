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

from itertools import product

import minimalloc as mm
import pytest


def _disabled_params() -> mm.SolverParams:
    """Return solver parameters with all optimizations disabled."""
    params = mm.SolverParams()
    params.canonical_only = False
    params.section_inference = False
    params.dynamic_ordering = False
    params.check_dominance = False
    params.unallocated_floor = False
    params.static_preordering = False
    params.dynamic_decomposition = False
    params.monotonic_floor = False
    params.hatless_pruning = False
    params.minimize_capacity = False
    params.preordering_heuristics = ["TWA"]
    return params


def _make_params(params: tuple[bool, ...]) -> mm.SolverParams:
    """Convert parameter tuple to SolverParams object."""

    param_names = [
        "canonical_only",
        "section_inference",
        "dynamic_ordering",
        "check_dominance",
        "unallocated_floor",
        "static_preordering",
        "dynamic_decomposition",
        "monotonic_floor",
        "minimize_capacity",
    ]

    params_result = _disabled_params()
    for i, name in enumerate(param_names):
        setattr(params_result, name, params[i])
    return params_result


def _make_buffer(
    name: str,
    start: int,
    end: int,
    size: int,
    offset: int | None = None,
    alignment: int = 1,
    gaps: list[mm.Gap] | None = None,
) -> mm.Buffer:
    """Helper to create a buffer with common defaults."""
    return mm.Buffer(
        name,
        mm.Lifespan(start, end),
        size=size,
        offset=offset,
        alignment=alignment,
        gaps=gaps or [],
    )


def _assert_feasible(
    problem: mm.Problem, params: mm.SolverParams
) -> mm.Solution:
    """Assert that a problem is feasible and return the solution."""
    solver = mm.Solver(params)
    solution = solver.solve(problem)
    assert solution is not None, "Expected feasible solution"
    return solution


def _assert_infeasible(problem: mm.Problem, params: mm.SolverParams) -> None:
    """Assert that a problem is infeasible."""
    solver = mm.Solver(params)
    solution = solver.solve(problem)
    assert solution is None, "Expected infeasible problem"
    assert solver.get_backtracks() > 0


# Generate all parameter combinations for comprehensive testing
PARAM_COMBINATIONS: list[tuple[bool, ...]] = list(
    product([True, False], repeat=9)
)


def test_preordering_comparator() -> None:
    """Test that preordering comparator works correctly."""

    def make_data(
        area: int, total: int, width: int, idx: int = 0
    ) -> mm.PreorderData:
        data = mm.PreorderData()
        data.area = area
        data.total = total
        data.width = width
        data.buffer_idx = idx
        return data

    data_a = make_data(1, 3, 2, 0)
    data_b = make_data(0, 4, 1, 0)
    data_c = make_data(0, 3, 3, 0)
    data_d = make_data(2, 3, 2, 0)
    data_e = make_data(1, 3, 2, 1)

    comparator = mm.PreorderingComparator("TWA")

    assert comparator(data_b, data_a)
    assert comparator(data_c, data_a)
    assert comparator(data_d, data_a)
    assert comparator(data_a, data_e)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_buffer_too_big(params: tuple[bool, ...]) -> None:
    """Single buffer exceeds capacity."""
    params = _make_params(params)
    problem = mm.Problem([_make_buffer("b0", 0, 2, size=3)], capacity=2)
    _assert_infeasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_trivial_conflict(params: tuple[bool, ...]) -> None:
    """Two overlapping buffers exceed capacity."""
    params = _make_params(params)
    problem = mm.Problem(
        [_make_buffer("b0", 0, 2, 2), _make_buffer("b1", 0, 2, 2)], capacity=3
    )
    _assert_infeasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_complex_conflict(params: tuple[bool, ...]) -> None:
    """Complex infeasible scenario."""
    params = _make_params(params)
    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 1, 3),
            _make_buffer("b1", 0, 3, 1),
            _make_buffer("b2", 4, 5, 3),
            _make_buffer("b3", 2, 5, 1),
            _make_buffer("b4", 1, 2, 2),
            _make_buffer("b5", 3, 4, 2),
            _make_buffer("b6", 1, 4, 1),
        ],
        capacity=4,
    )
    _assert_infeasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_fixed_buffer_conflict(params: tuple[bool, ...]) -> None:
    """Fixed buffer at offset 0 creates infeasibility."""
    params = _make_params(params)
    problem = mm.Problem(
        [
            _make_buffer("b0", 1, 2, 1, offset=0),
            _make_buffer("b1", 0, 2, 1),
            _make_buffer("b2", 2, 3, 2),
            _make_buffer("b3", 1, 3, 1),
            _make_buffer("b4", 0, 1, 2),
        ],
        capacity=3,
    )
    _assert_infeasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_empty_problem(params: tuple[bool, ...]) -> None:
    """Empty problem is always feasible."""
    params = _make_params(params)
    problem = mm.Problem([], capacity=0)
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_single_buffer(params: tuple[bool, ...]) -> None:
    """Single buffer fitting exactly."""
    params = _make_params(params)
    problem = mm.Problem([_make_buffer("b0", 0, 2, 2)], capacity=2)
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_two_overlapping_buffers(params: tuple[bool, ...]) -> None:
    """Two buffers with overlapping lifespans."""
    params = _make_params(params)
    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 2, 2),
            _make_buffer("b1", 1, 3, 2),
        ],
        capacity=4,
    )
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_five_buffers_tight_fit(params: tuple[bool, ...]) -> None:
    """Five buffers with tight capacity constraint."""
    params = _make_params(params)
    problem = mm.Problem(
        [
            _make_buffer("b0", 1, 2, 1),
            _make_buffer("b1", 0, 2, 1),
            _make_buffer("b2", 2, 3, 2),
            _make_buffer("b3", 1, 3, 1),
            _make_buffer("b4", 0, 1, 2),
        ],
        capacity=3,
    )
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_fixed_buffer_feasible(params: tuple[bool, ...]) -> None:
    """Fixed buffer at offset 1 is feasible."""
    params = _make_params(params)
    problem = mm.Problem(
        [
            _make_buffer("b0", 1, 2, 1),
            _make_buffer("b1", 0, 2, 1),
            _make_buffer("b2", 2, 3, 2, offset=1),
            _make_buffer("b3", 1, 3, 1),
            _make_buffer("b4", 0, 1, 2),
        ],
        capacity=3,
    )

    solution = _assert_feasible(problem, params)
    assert solution.offsets[2] == 1


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_two_partitions(params: tuple[bool, ...]) -> None:
    """Buffers naturally partition into two groups."""
    params = _make_params(params)

    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 2, 2),
            _make_buffer("b1", 1, 3, 2),
            _make_buffer("b2", 3, 5, 2),
            _make_buffer("b3", 4, 6, 2),
        ],
        capacity=4,
    )
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_alignment_constraints(params: tuple[bool, ...]) -> None:
    """Buffers with even alignment requirements."""
    params = _make_params(params)
    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 2, 1, alignment=2),
            _make_buffer("b1", 0, 2, 1, alignment=2),
        ],
        capacity=4,
    )
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_simple_temporal_gap(params: tuple[bool, ...]) -> None:
    """Buffer with temporal gap allows reuse."""
    params = _make_params(params)
    gap = mm.Gap(mm.Lifespan(1, 3))
    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 4, 2, gaps=[gap]),
            _make_buffer("b1", 1, 3, 2),
        ],
        capacity=2,
    )
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_tetris_spatial_gaps(params: tuple[bool, ...]) -> None:
    """Tetris-like interlocking spatial gaps."""
    params = _make_params(params)
    gap_a = mm.Gap(mm.Lifespan(0, 5), mm.Window(0, 1))
    gap_b = mm.Gap(mm.Lifespan(5, 10), mm.Window(1, 2))

    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 10, 2, gaps=[gap_a]),
            _make_buffer("b1", 0, 10, 2, gaps=[gap_b]),
        ],
        capacity=3,
    )
    _assert_feasible(problem, params)


@pytest.mark.parametrize("params", PARAM_COMBINATIONS)
def test_complex_stairs_pattern(params: tuple[bool, ...]) -> None:
    """Complex stair-like gap pattern."""
    params = _make_params(params)

    def make_gap(
        start: int, end: int, window_start: int, window_end: int
    ) -> mm.Gap:
        return mm.Gap(
            mm.Lifespan(start, end),
            mm.Window(window_start, window_end),
        )

    problem = mm.Problem(
        [
            _make_buffer(
                "b0",
                0,
                108,
                30,
                gaps=[make_gap(36, 72, 10, 30), make_gap(72, 108, 20, 30)],
            ),
            _make_buffer(
                "b1",
                36,
                144,
                50,
                gaps=[make_gap(36, 72, 20, 30), make_gap(72, 108, 10, 40)],
            ),
            _make_buffer(
                "b2",
                84,
                144,
                42,
                gaps=[make_gap(114, 129, 0, 28), make_gap(129, 144, 0, 14)],
            ),
            _make_buffer(
                "b3",
                84,
                129,
                42,
                gaps=[make_gap(99, 114, 14, 42), make_gap(114, 129, 28, 42)],
            ),
            _make_buffer(
                "b4",
                99,
                144,
                70,
                gaps=[make_gap(99, 114, 28, 42), make_gap(114, 129, 14, 56)],
            ),
            _make_buffer(
                "b5",
                0,
                144,
                30,
                gaps=[make_gap(72, 108, 0, 20), make_gap(108, 144, 0, 10)],
            ),
        ],
        capacity=144,
    )
    _assert_feasible(problem, params)


def test_counts_backtracks_correctly() -> None:
    """Verify backtrack counter works and resets properly."""
    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 2, 2),
            _make_buffer("b1", 0, 2, 2),
        ],
        capacity=3,
    )

    solver = mm.Solver(_disabled_params())

    # First solve
    solution = solver.solve(problem)
    assert solution is None
    assert solver.get_backtracks() == 3

    # Second solve - counter should reset
    solution = solver.solve(problem)
    assert solution is None
    assert solver.get_backtracks() == 3


@pytest.mark.parametrize(
    ("optimization", "flag"),
    [
        ("canonical_only", True),
        ("section_inference", True),
        ("dynamic_ordering", True),
        ("check_dominance", True),
        ("static_preordering", True),
        ("dynamic_decomposition", True),
    ],
)
def test_optimization_reduces_backtracks(optimization: str, flag: bool) -> None:
    """Verify each optimization reduces backtrack count."""
    problem = mm.Problem(
        [
            _make_buffer("b0", 2, 3, 2),
            _make_buffer("b1", 0, 1, 2),
            _make_buffer("b2", 1, 2, 1),
            _make_buffer("b3", 0, 2, 1),
            _make_buffer("b4", 1, 3, 1),
        ],
        capacity=3,
    )

    # Test with optimization enabled
    optimized_params = _disabled_params()
    setattr(optimized_params, optimization, flag)
    optimized_solver = mm.Solver(optimized_params)
    optimized_solution = optimized_solver.solve(problem)
    assert optimized_solution is not None

    # Test with all optimizations disabled
    baseline_solver = mm.Solver(_disabled_params())
    baseline_solution = baseline_solver.solve(problem)
    assert baseline_solution is not None

    # Optimization should reduce backtracks
    assert optimized_solver.get_backtracks() < baseline_solver.get_backtracks()


def test_compute_iis() -> None:
    """Verify IIS identifies minimal conflicting subset."""
    problem = mm.Problem(
        [
            _make_buffer("b0", 0, 2, 2),  # Not in IIS
            _make_buffer("b1", 0, 2, 2),  # Not in IIS
            _make_buffer("b2", 2, 5, 2),  # In IIS
            _make_buffer("b3", 3, 6, 2),  # In IIS
            _make_buffer("b4", 4, 7, 2),  # In IIS
        ],
        capacity=4,
    )

    solver = mm.Solver()
    subset = solver.compute_irreducible_infeasible_subset(problem)
    assert subset == [2, 3, 4]
