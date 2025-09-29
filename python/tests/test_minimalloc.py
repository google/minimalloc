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


def test_buffer_effective_size_with_overlap() -> None:
    """Test effective size calculation with overlapping buffers."""
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 2), size=4)
    buffer_b = mm.Buffer("b", mm.Lifespan(1, 3), size=5)

    assert buffer_a.effective_size(buffer_b) == 4
    assert buffer_b.effective_size(buffer_a) == 5


def test_buffer_effective_size_without_overlap() -> None:
    """Test effective size calculation with non-overlapping buffers."""
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 2), size=4)
    buffer_b = mm.Buffer("b", mm.Lifespan(3, 5), size=5)

    assert buffer_a.effective_size(buffer_b) is None
    assert buffer_b.effective_size(buffer_a) is None


def test_buffer_effective_size_without_overlap_edge_case() -> None:
    """Test effective size calculation with adjacent non-overlapping buffers."""
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 2), size=4)
    buffer_b = mm.Buffer("b", mm.Lifespan(2, 4), size=5)

    assert buffer_a.effective_size(buffer_b) is None
    assert buffer_b.effective_size(buffer_a) is None


def test_buffer_effective_size_gaps_with_overlap() -> None:
    """Test effective size calculation with gaps and overlapping buffers."""
    gap_a1 = mm.Gap(mm.Lifespan(1, 4))
    gap_a2 = mm.Gap(mm.Lifespan(6, 9))
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 10), size=4, gaps=[gap_a1, gap_a2])

    gap_b1 = mm.Gap(mm.Lifespan(6, 9))
    gap_b2 = mm.Gap(mm.Lifespan(11, 14))
    buffer_b = mm.Buffer("b", mm.Lifespan(5, 15), size=5, gaps=[gap_b1, gap_b2])

    assert buffer_a.effective_size(buffer_b) == 4
    assert buffer_b.effective_size(buffer_a) == 5


def test_buffer_effective_size_gaps_without_overlap() -> None:
    """Test effective size calculation with gaps but no actual overlapping regions."""
    gap_a = mm.Gap(mm.Lifespan(1, 9))
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 10), size=4, gaps=[gap_a])

    gap_b = mm.Gap(mm.Lifespan(6, 14))
    buffer_b = mm.Buffer("b", mm.Lifespan(5, 15), size=5, gaps=[gap_b])

    assert buffer_a.effective_size(buffer_b) is None
    assert buffer_b.effective_size(buffer_a) is None


def test_buffer_effective_size_gaps_without_overlap_edge_case_first() -> None:
    """Test effective size calculation edge case where gap eliminates overlap."""
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 10), size=4)

    gap_b = mm.Gap(mm.Lifespan(5, 10))
    buffer_b = mm.Buffer("b", mm.Lifespan(5, 15), size=5, gaps=[gap_b])

    assert buffer_a.effective_size(buffer_b) is None
    assert buffer_b.effective_size(buffer_a) is None


def test_buffer_effective_size_gaps_without_overlap_edge_case_second() -> None:
    """Test effective size calculation edge case where gap eliminates overlap."""
    gap_a = mm.Gap(mm.Lifespan(5, 10))
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 10), size=4, gaps=[gap_a])

    buffer_b = mm.Buffer("b", mm.Lifespan(5, 15), size=5)

    assert buffer_a.effective_size(buffer_b) is None
    assert buffer_b.effective_size(buffer_a) is None


def test_buffer_effective_size_tetris() -> None:
    """Test effective size calculation with tetris-like spatial gaps."""
    gap_a = mm.Gap(mm.Lifespan(0, 5), mm.Window(0, 1))
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 10), size=2, gaps=[gap_a])

    gap_b = mm.Gap(mm.Lifespan(5, 10), mm.Window(1, 2))
    buffer_b = mm.Buffer("b", mm.Lifespan(0, 10), size=2, gaps=[gap_b])

    assert buffer_a.effective_size(buffer_b) == 1


def test_buffer_effective_size_stairs() -> None:
    """Test effective size calculation with complex stair-like spatial gaps."""
    gap_a1 = mm.Gap(mm.Lifespan(0, 5), mm.Window(0, 1))
    gap_a2 = mm.Gap(mm.Lifespan(5, 10), mm.Window(0, 2))
    buffer_a = mm.Buffer("a", mm.Lifespan(0, 15), size=3, gaps=[gap_a1, gap_a2])

    gap_b1 = mm.Gap(mm.Lifespan(5, 10), mm.Window(1, 3))
    gap_b2 = mm.Gap(mm.Lifespan(10, 15), mm.Window(2, 3))
    buffer_b = mm.Buffer("b", mm.Lifespan(0, 15), size=3, gaps=[gap_b1, gap_b2])

    assert buffer_a.effective_size(buffer_b) == 1


def test_problem_strip_solution_ok() -> None:
    """Test that a problem with all buffers having fixed offsets can be stripped to a solution."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2, offset=3),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=3, offset=4),
    ]
    problem = mm.Problem(buffers, capacity=5)

    solution = problem.strip_solution()
    expected_solution = mm.Solution(offsets=[3, 4])

    assert solution == expected_solution


def test_problem_strip_solution_not_found() -> None:
    """Test that a problem with some buffers missing fixed offsets raises an exception."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2, offset=3),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=3),  # No offset
    ]
    problem = mm.Problem(buffers, capacity=5)

    assert problem.strip_solution() is None
