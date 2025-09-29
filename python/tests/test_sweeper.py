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


def test_create_points_no_overlap() -> None:
    """Test CreatePoints with non-overlapping buffers."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 3), size=1),
    ]
    problem = mm.Problem(buffers)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=0,
            time_value=0,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=1,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=1,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=2,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=2,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=3,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_no_overlap() -> None:
    """Test sweeper with non-overlapping buffers."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 2), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 3), size=1),
    ]
    problem = mm.Problem(buffers)

    result = mm.sweep(problem)

    expected_sections = [{0}, {1}, {2}]
    expected_partitions = [
        mm.Partition([0], mm.SectionRange(0, 1)),
        mm.Partition([1], mm.SectionRange(1, 2)),
        mm.Partition([2], mm.SectionRange(2, 3)),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(2, 3), mm.Window(0, 1))
            ]
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_no_overlap() -> None:
    """Test CalculateCuts with non-overlapping buffers."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(2, 3), mm.Window(0, 1))
            ]
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [[0], [1], [2]]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [0, 0]

    assert cuts == expected_cuts


def test_create_points_with_overlap() -> None:
    """Test CreatePoints with overlapping buffers."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 4), size=1),
    ]
    problem = mm.Problem(buffers)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=0,
            time_value=0,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=1,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=1,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=2,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=3,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=4,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_with_overlap() -> None:
    """Test sweeper with overlapping buffers."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 4), size=1),
    ]
    problem = mm.Problem(buffers)

    result = mm.sweep(problem)

    expected_sections = [{0}, {1, 2}, {2}]
    expected_partitions = [
        mm.Partition([0], mm.SectionRange(0, 1)),
        mm.Partition([1, 2], mm.SectionRange(1, 3)),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 3), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_with_overlap() -> None:
    """Test CalculateCuts with overlapping buffers."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 3), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [{0}, {1, 2}, {2}]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [0, 1]

    assert cuts == expected_cuts


def test_create_points_two_buffers_end_at_same_time() -> None:
    """Test CreatePoints with two buffers ending at the same time."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 3), size=1),
    ]
    problem = mm.Problem(buffers)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=0,
            time_value=0,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=1,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=1,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=2,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=3,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=3,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_two_buffers_end_at_same_time() -> None:
    """Test sweeper with two buffers ending at the same time."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 3), size=1),
    ]
    problem = mm.Problem(buffers)

    result = mm.sweep(problem)

    expected_sections = [{0}, {1, 2}]
    expected_partitions = [
        mm.Partition([0], mm.SectionRange(0, 1)),
        mm.Partition([1, 2], mm.SectionRange(1, 2)),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_two_buffers_end_at_same_time() -> None:
    """Test CalculateCuts with two buffers ending at the same time."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [[0], [1, 2]]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [0]

    assert cuts == expected_cuts


def test_create_points_super_long_buffer_prevents_partitioning() -> None:
    """Test CreatePoints with a long buffer that prevents partitioning."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 4), size=1),
        mm.Buffer("b3", mm.Lifespan(0, 4), size=1),
    ]
    problem = mm.Problem(buffers)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=0,
            time_value=0,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=3,
            time_value=0,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=1,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=1,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=2,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=3,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=4,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=3,
            time_value=4,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_super_long_buffer_prevents_partitioning() -> None:
    """Test sweeper with a long buffer that prevents partitioning."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(0, 1), size=2),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(2, 4), size=1),
        mm.Buffer("b3", mm.Lifespan(0, 4), size=1),
    ]
    problem = mm.Problem(buffers)

    result = mm.sweep(problem)

    expected_sections = [{0, 3}, {1, 2, 3}, {2, 3}]
    expected_partitions = [
        mm.Partition(
            [0, 3, 1, 2],
            section_range=mm.SectionRange(0, 3),
        ),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ],
            overlaps=[mm.Overlap(buffer_idx=3, effective_size=2)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[
                mm.Overlap(buffer_idx=2, effective_size=1),
                mm.Overlap(buffer_idx=3, effective_size=1),
            ],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 3), mm.Window(0, 1))
            ],
            overlaps=[
                mm.Overlap(buffer_idx=1, effective_size=1),
                mm.Overlap(buffer_idx=3, effective_size=1),
            ],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 3), mm.Window(0, 1))
            ],
            overlaps=[
                mm.Overlap(buffer_idx=0, effective_size=1),
                mm.Overlap(buffer_idx=1, effective_size=1),
                mm.Overlap(buffer_idx=2, effective_size=1),
            ],
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_super_long_buffer_prevents_partitioning() -> None:
    """Test CalculateCuts with a long buffer that prevents partitioning."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ],
            overlaps=[mm.Overlap(buffer_idx=3, effective_size=2)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[
                mm.Overlap(buffer_idx=2, effective_size=1),
                mm.Overlap(buffer_idx=3, effective_size=1),
            ],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 3), mm.Window(0, 1))
            ],
            overlaps=[
                mm.Overlap(buffer_idx=1, effective_size=1),
                mm.Overlap(buffer_idx=3, effective_size=1),
            ],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 3), mm.Window(0, 1))
            ],
            overlaps=[
                mm.Overlap(buffer_idx=0, effective_size=1),
                mm.Overlap(buffer_idx=1, effective_size=1),
                mm.Overlap(buffer_idx=2, effective_size=1),
            ],
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [[0, 3], [1, 3, 2], [3, 2]]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [1, 2]

    assert cuts == expected_cuts


def test_create_points_buffers_out_of_order() -> None:
    """Test CreatePoints with buffers defined out of order."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(2, 3), size=1),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(0, 1), size=2),
    ]
    problem = mm.Problem(buffers)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=2,
            time_value=0,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=1,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=1,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=2,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=3,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=3,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_buffers_out_of_order() -> None:
    """Test sweeper with buffers defined out of order."""
    buffers = [
        mm.Buffer("b0", mm.Lifespan(2, 3), size=1),
        mm.Buffer("b1", mm.Lifespan(1, 3), size=1),
        mm.Buffer("b2", mm.Lifespan(0, 1), size=2),
    ]
    problem = mm.Problem(buffers)

    result = mm.sweep(problem)

    expected_sections = [{2}, {1, 0}]
    expected_partitions = [
        mm.Partition([2], mm.SectionRange(0, 1)),
        mm.Partition([1, 0], mm.SectionRange(1, 2)),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=0, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_buffers_out_of_order() -> None:
    """Test CalculateCuts with buffers defined out of order."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1))
            ],
            overlaps=[mm.Overlap(buffer_idx=0, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2))
            ]
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [[2], [1, 0]]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [0]

    assert cuts == expected_cuts


def test_create_points_with_gaps() -> None:
    """Test CreatePoints with buffers having gaps."""
    gap0 = mm.Gap(mm.Lifespan(5, 6))
    gap1 = mm.Gap(mm.Lifespan(6, 7))
    gap2 = mm.Gap(mm.Lifespan(5, 7))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(4, 7), size=1, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(5, 8), size=1, gaps=[gap1]),
        mm.Buffer("b2", mm.Lifespan(4, 8), size=1, gaps=[gap2]),
    ]
    problem = mm.Problem(buffers)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=0,
            time_value=4,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=4,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=5,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=5,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=5,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=6,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=6,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=7,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=7,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=7,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=8,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=2,
            time_value=8,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_with_gaps() -> None:
    """Test sweeper with buffers having gaps."""
    gap0 = mm.Gap(mm.Lifespan(5, 6))
    gap1 = mm.Gap(mm.Lifespan(6, 7))
    gap2 = mm.Gap(mm.Lifespan(5, 7))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(4, 7), size=1, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(5, 8), size=1, gaps=[gap1]),
        mm.Buffer("b2", mm.Lifespan(4, 8), size=1, gaps=[gap2]),
    ]
    problem = mm.Problem(buffers)

    result = mm.sweep(problem)

    expected_sections = [{0, 2}, {1}, {0}, {1, 2}]
    expected_partitions = [
        mm.Partition([0, 2, 1], mm.SectionRange(0, 4)),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(2, 3), mm.Window(0, 1)),
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(3, 4), mm.Window(0, 1)),
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(3, 4), mm.Window(0, 1)),
            ],
            overlaps=[
                mm.Overlap(buffer_idx=0, effective_size=1),
                mm.Overlap(buffer_idx=1, effective_size=1),
            ],
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_with_gaps() -> None:
    """Test CalculateCuts with buffers having gaps."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(2, 3), mm.Window(0, 1)),
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(3, 4), mm.Window(0, 1)),
            ],
            overlaps=[mm.Overlap(buffer_idx=2, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(3, 4), mm.Window(0, 1)),
            ],
            overlaps=[
                mm.Overlap(buffer_idx=0, effective_size=1),
                mm.Overlap(buffer_idx=1, effective_size=1),
            ],
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [[0, 2], [1], [0], [1, 2]]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [2, 3, 2]

    assert cuts == expected_cuts


def test_create_points_tetris() -> None:
    """Test CreatePoints with tetris-like spatial gaps."""
    gap0 = mm.Gap(mm.Lifespan(4, 6), mm.Window(0, 1))
    gap1 = mm.Gap(mm.Lifespan(6, 8), mm.Window(1, 2))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(4, 8), size=2, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(4, 8), size=2, gaps=[gap1]),
    ]
    problem = mm.Problem(buffers, capacity=3)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=0,
            time_value=4,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=4,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=6,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=6,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=6,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=6,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(1, 2),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=8,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=1,
            time_value=8,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(1, 2),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_tetris() -> None:
    """Test sweeper with tetris-like spatial gaps."""
    gap0 = mm.Gap(mm.Lifespan(4, 6), mm.Window(0, 1))
    gap1 = mm.Gap(mm.Lifespan(6, 8), mm.Window(1, 2))

    buffers = [
        mm.Buffer("b0", mm.Lifespan(4, 8), size=2, gaps=[gap0]),
        mm.Buffer("b1", mm.Lifespan(4, 8), size=2, gaps=[gap1]),
    ]
    problem = mm.Problem(buffers, capacity=3)

    result = mm.sweep(problem)

    expected_sections = [{0, 1}, {0, 1}]
    expected_partitions = [
        mm.Partition([0, 1], mm.SectionRange(0, 2)),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 2)),
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2)),
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(1, 2)),
            ],
            overlaps=[mm.Overlap(buffer_idx=0, effective_size=2)],
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_tetris() -> None:
    """Test CalculateCuts with tetris-like spatial gaps."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 2)),
            ],
            overlaps=[mm.Overlap(buffer_idx=1, effective_size=1)],
        ),
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 2)),
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(1, 2)),
            ],
            overlaps=[mm.Overlap(buffer_idx=0, effective_size=2)],
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [[0, 1], [0, 1]]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [2]

    assert cuts == expected_cuts


def test_create_points_mixed_gaps() -> None:
    """Test CreatePoints with mixed gap types (spatial and temporal)."""
    gap0 = mm.Gap(mm.Lifespan(4, 5), mm.Window(0, 1))
    gap1 = mm.Gap(mm.Lifespan(5, 6), mm.Window(0, 2))
    gap2 = mm.Gap(mm.Lifespan(6, 7))  # Full temporal gap
    gap3 = mm.Gap(mm.Lifespan(7, 8), mm.Window(0, 2))

    buffers = [
        mm.Buffer(
            "b0",
            mm.Lifespan(4, 8),
            size=2,
            gaps=[gap0, gap1, gap2, gap3],
        ),
    ]
    problem = mm.Problem(buffers, capacity=3)

    points = mm.create_points(problem)

    expected_points = [
        mm.SweepPoint(
            buffer_idx=0,
            time_value=4,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 1),
            endpoint=True,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=5,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 1),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=5,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=6,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=7,
            point_type=mm.SweepPointType.LEFT,
            window=mm.Window(0, 2),
            endpoint=False,
        ),
        mm.SweepPoint(
            buffer_idx=0,
            time_value=8,
            point_type=mm.SweepPointType.RIGHT,
            window=mm.Window(0, 2),
            endpoint=True,
        ),
    ]

    assert points == expected_points


def test_sweeper_mixed_gaps() -> None:
    """Test sweeper with mixed gap types (spatial and temporal)."""
    gap0 = mm.Gap(mm.Lifespan(4, 5), mm.Window(0, 1))
    gap1 = mm.Gap(mm.Lifespan(5, 6), mm.Window(0, 2))
    gap2 = mm.Gap(mm.Lifespan(6, 7))  # Full temporal gap
    gap3 = mm.Gap(mm.Lifespan(7, 8), mm.Window(0, 2))

    buffers = [
        mm.Buffer(
            "b0",
            mm.Lifespan(4, 8),
            size=2,
            gaps=[gap0, gap1, gap2, gap3],
        ),
    ]
    problem = mm.Problem(buffers, capacity=3)

    result = mm.sweep(problem)

    expected_sections = [{0}, {0}, {0}]
    expected_partitions = [
        mm.Partition([0], mm.SectionRange(0, 3)),
    ]
    expected_buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 2)),
                mm.SectionSpan(mm.SectionRange(2, 3), mm.Window(0, 2)),
            ]
        ),
    ]

    assert result.sections == expected_sections
    assert result.partitions == expected_partitions
    assert result.buffer_data == expected_buffer_data


def test_calculate_cuts_mixed_gaps() -> None:
    """Test CalculateCuts with mixed gap types (spatial and temporal)."""
    buffer_data = [
        mm.BufferData(
            section_spans=[
                mm.SectionSpan(mm.SectionRange(0, 1), mm.Window(0, 1)),
                mm.SectionSpan(mm.SectionRange(1, 2), mm.Window(0, 2)),
                mm.SectionSpan(mm.SectionRange(2, 3), mm.Window(0, 2)),
            ]
        ),
    ]
    sweep_result = mm.SweepResult()
    sweep_result.sections = [[0], [0], [0]]
    sweep_result.buffer_data = buffer_data

    cuts = sweep_result.calculate_cuts()
    expected_cuts = [1, 1]

    assert cuts == expected_cuts
