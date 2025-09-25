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


def test_validates_good_solution() -> None:
    """Test that a valid solution is accepted."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 4), size=1),
        mm.Buffer("b3", mm.Lifespan(3, 5), size=1),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # right # of offsets, in range, no overlaps
    solution = mm.Solution(offsets=[0, 0, 1, 0], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.GOOD


def test_validates_good_solution_with_gaps() -> None:
    """Test that a valid solution with gaps is accepted."""
    gap0 = mm.Gap(mm.Lifespan(1, 9))
    gap1 = mm.Gap(mm.Lifespan(6, 14))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 10), size=2, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(5, 15), size=2, gaps=[gap1]),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # right # of offsets, in range, no overlaps
    solution = mm.Solution(offsets=[0, 0], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.GOOD


def test_validates_good_solution_with_gaps_edge_case() -> None:
    """Test that a valid solution with gaps at edge case is accepted."""
    gap0 = mm.Gap(mm.Lifespan(1, 8))
    gap1 = mm.Gap(mm.Lifespan(8, 14))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 10), size=2, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(5, 15), size=2, gaps=[gap1]),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # right # of offsets, in range, no overlaps
    solution = mm.Solution(offsets=[0, 0], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.GOOD


def test_validates_tetris() -> None:
    """Test validation with tetris-like gaps."""
    gap0 = mm.Gap(mm.Lifespan(0, 5), mm.Window(0, 1))
    gap1 = mm.Gap(mm.Lifespan(5, 10), mm.Window(1, 2))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 10), size=2, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(0, 10), size=2, gaps=[gap1]),
    ]
    problem = mm.Problem(buffers, capacity=3)

    # right # of offsets, in range, no overlaps
    solution = mm.Solution(offsets=[0, 1], height=3)

    assert mm.validate(problem, solution) == mm.ValidationResult.GOOD


def test_validates_stairs() -> None:
    """Test validation with stairs-like complex gaps."""
    # Buffer 0 gaps
    gap0_0 = mm.Gap(mm.Lifespan(36, 72), mm.Window(10, 30))
    gap0_1 = mm.Gap(mm.Lifespan(72, 108), mm.Window(20, 30))

    # Buffer 1 gaps
    gap1_0 = mm.Gap(mm.Lifespan(36, 72), mm.Window(20, 30))
    gap1_1 = mm.Gap(mm.Lifespan(72, 108), mm.Window(10, 40))

    # Buffer 2 gaps
    gap2_0 = mm.Gap(mm.Lifespan(114, 129), mm.Window(0, 28))
    gap2_1 = mm.Gap(mm.Lifespan(129, 144), mm.Window(0, 14))

    # Buffer 3 gaps
    gap3_0 = mm.Gap(mm.Lifespan(99, 114), mm.Window(14, 42))
    gap3_1 = mm.Gap(mm.Lifespan(114, 129), mm.Window(28, 42))

    # Buffer 4 gaps
    gap4_0 = mm.Gap(mm.Lifespan(99, 114), mm.Window(28, 42))
    gap4_1 = mm.Gap(mm.Lifespan(114, 129), mm.Window(14, 56))

    # Buffer 5 gaps
    gap5_0 = mm.Gap(mm.Lifespan(72, 108), mm.Window(0, 20))
    gap5_1 = mm.Gap(mm.Lifespan(108, 144), mm.Window(0, 10))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 108), size=30, gaps=[gap0_0, gap0_1]),
        mm.Buffer("b1", mm.Lifespan(36, 144), size=50, gaps=[gap1_0, gap1_1]),
        mm.Buffer("b2", mm.Lifespan(84, 144), size=42, gaps=[gap2_0, gap2_1]),
        mm.Buffer("b3", mm.Lifespan(84, 129), size=42, gaps=[gap3_0, gap3_1]),
        mm.Buffer("b4", mm.Lifespan(99, 144), size=70, gaps=[gap4_0, gap4_1]),
        mm.Buffer("b5", mm.Lifespan(0, 144), size=30, gaps=[gap5_0, gap5_1]),
    ]
    problem = mm.Problem(buffers, capacity=144)

    solution = mm.Solution(offsets=[30, 10, 60, 102, 74, 0], height=144)

    assert mm.validate(problem, solution) == mm.ValidationResult.GOOD


def test_invalidates_bad_solution() -> None:
    """Test that a solution with wrong number of offsets is rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(1, 2), size=1),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # wrong # of offsets
    solution = mm.Solution(offsets=[0, 0], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_SOLUTION


def test_invalidates_fixed_buffer() -> None:
    """Test that a fixed buffer with wrong offset is rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(1, 2), size=1, offset=0),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # last buffer needs offset @ 0
    solution = mm.Solution(offsets=[0, 0, 1], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_FIXED


def test_invalidates_negative_offset() -> None:
    """Test that a negative offset is rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(1, 2), size=1),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # offset is negative
    solution = mm.Solution(offsets=[0, 0, -1], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_OFFSET


def test_invalidates_out_of_range_offset() -> None:
    """Test that an out of range offset is rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(1, 2), size=1),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # buffer lies outside range
    solution = mm.Solution(offsets=[0, 0, 2], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_OFFSET


def test_invalidates_out_of_height_range_offset() -> None:
    """Test that an offset exceeding height is rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(1, 2), size=1),
    ]
    problem = mm.Problem(buffers, capacity=3)

    # height too small
    solution = mm.Solution(offsets=[0, 0, 2], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_HEIGHT


def test_invalidates_with_extra_height() -> None:
    """Test that unnecessary height is rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(1, 2), size=1),
    ]
    problem = mm.Problem(buffers, capacity=4)

    # height too large
    solution = mm.Solution(offsets=[0, 0, 2], height=4)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_HEIGHT


def test_invalidates_overlap() -> None:
    """Test that overlapping buffers are rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(1, 2), size=1),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # final two buffers overlap
    solution = mm.Solution(offsets=[0, 0, 0], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_OVERLAP


def test_invalidates_misalignment() -> None:
    """Test that misaligned buffers are rejected."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1, alignment=2),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # offset of 1 isn't a multiple of 2
    solution = mm.Solution(offsets=[0, 1], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_ALIGNMENT


def test_invalidates_gap_overlap() -> None:
    """Test that buffers with overlapping gaps are rejected."""
    gap0 = mm.Gap(mm.Lifespan(1, 7))
    gap1 = mm.Gap(mm.Lifespan(8, 14))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 10), size=2, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(5, 15), size=2, gaps=[gap1]),
    ]
    problem = mm.Problem(buffers, capacity=2)

    # right # of offsets, in range, but overlaps
    solution = mm.Solution(offsets=[0, 0], height=2)

    assert mm.validate(problem, solution) == mm.ValidationResult.BAD_OVERLAP
